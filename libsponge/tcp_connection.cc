#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return this->active() ? this->_sender.stream_in().remaining_capacity() : 0; }

size_t TCPConnection::bytes_in_flight() const { return this->active() ? this->_sender.bytes_in_flight() : 0; }

size_t TCPConnection::unassembled_bytes() const { return this->active() ? this->_receiver.unassembled_bytes() : 0; }

size_t TCPConnection::time_since_last_segment_received() const { return this->active() ? this->_time_since_last_segment_received : 0; }

void TCPConnection::segment_received(const TCPSegment &seg) {
    this->_time_since_last_segment_received = 0;

    const TCPHeader &header = seg.header();

    if (header.rst)
        this->_report();

    if (!this->active())
        return;

    const optional<WrappingInt32> ackno = this->_receiver.ackno();
    if (seg.length_in_sequence_space() == 0 && ackno.has_value() && header.seqno == ackno.value() - 1)
        this->_sender.send_empty_segment();
    else {
        this->_receiver.segment_received(seg);

        if (header.ack)
            this->_sender.ack_received(header.ackno, header.win);
        else
            this->_sender.fill_window();

        if (seg.length_in_sequence_space() != 0 && this->_sender.segments_out().empty())
            this->_sender.send_empty_segment();

        if (this->_receiver.stream_out().eof() && !(this->_sender.stream_in().eof() && this->_fin))
            this->_linger_after_streams_finish = false;
    }

    this->_send();
}

bool TCPConnection::active() const {
    if (this->_error)
        return false;

    if (!this->_check() || this->_linger_after_streams_finish)
        return true;

    return false;
}

size_t TCPConnection::write(const string &data) {
    if (!this->active()) return 0;

    const size_t size = this->_sender.stream_in().write(data);
    
    this->_sender.fill_window();
    this->_send();
    
    return size;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    if (!this->active()) return;

    this->_time_since_last_segment_received += ms_since_last_tick;
    this->_sender.tick(ms_since_last_tick);

    if (this->_sender.consecutive_retransmissions() > this->_cfg.MAX_RETX_ATTEMPTS)
        this->_report();

    if (this->_check() && _time_since_last_segment_received >= 10 * this->_cfg.rt_timeout)
        this->_linger_after_streams_finish = false;

    this->_send();
}

void TCPConnection::end_input_stream() {
    if (!this->active()) return;

    this->_sender.stream_in().end_input();
    this->_sender.fill_window();
    this->_send();
}

void TCPConnection::connect() {
    this->_sender.fill_window();
    this->_send();
}

bool TCPConnection::_check() const {
    if (!(this->_fin && this->_sender.stream_in().eof()) || this->_sender.bytes_in_flight() != 0) return false;

    if (!this->_receiver.stream_out().eof()) return false;

    return true;
}

void TCPConnection::_report() {
    this->_error = true;
    this->_sender.stream_in().set_error();
    this->_receiver.stream_out().set_error();
}

void TCPConnection::_send() {
    queue<TCPSegment> &waiting_queue = this->_sender.segments_out();

    while (!waiting_queue.empty()) {
        TCPSegment segment = waiting_queue.front();
        waiting_queue.pop();
        
        TCPHeader &header = segment.header();

        if (header.fin) this->_fin = true;

        const optional<WrappingInt32> ackno = this->_receiver.ackno();
        if (ackno.has_value()) header.ack = true, header.ackno = ackno.value();

        header.win = min(this->_receiver.window_size(), static_cast<size_t>(0xFFFF));
        header.rst = this->_error;

        this->_segments_out.push(segment);
    }
}


TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
            this->_report();
            this->_sender.send_empty_segment();
            this->_send();
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
