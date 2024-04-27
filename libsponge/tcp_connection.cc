#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const {
    return this->active() ? this->_sender.stream_in().remaining_capacity() : 0;
}

size_t TCPConnection::bytes_in_flight() const { return this->active() ? this->_sender.bytes_in_flight() : 0; }

size_t TCPConnection::unassembled_bytes() const { return this->active() ? this->_receiver.unassembled_bytes() : 0; }

size_t TCPConnection::time_since_last_segment_received() const {
    return this->active() ? this->_time_since_last_segment_received : 0;
}

void TCPConnection::segment_received(const TCPSegment &seg) {
    // Initiate _time_since_last_segment_received
    this->_time_since_last_segment_received = 0;

    const TCPHeader &header = seg.header();

    // Segment reported to be error
    if (header.rst)
        this->_report();

    // Ignore when inactive
    if (!this->active())
        return;

    const optional<WrappingInt32> ackno = this->_receiver.ackno();
    // Keep-alive segment
    if (seg.length_in_sequence_space() == 0 && ackno.has_value() && header.seqno == ackno.value() - 1)
        this->_sender.send_empty_segment();
    // Ordinary segments
    else {
        // Alarm receiver about segment
        this->_receiver.segment_received(seg);

        // Alarm sender about ack or fill window
        if (header.ack)
            this->_sender.ack_received(header.ackno, header.win);
        else
            this->_sender.fill_window();

        // When received data segments
        if (seg.length_in_sequence_space() != 0 && this->_sender.segments_out().empty())
            this->_sender.send_empty_segment();

        // lingering condition check
        if (this->_receiver.stream_out().eof() && !(this->_sender.stream_in().eof() && this->_fin))
            this->_linger_after_streams_finish = false;
    }

    // Try to send segments
    this->_send();
}

bool TCPConnection::active() const {
    // When error occrued, the connection is inactive
    if (this->_error)
        return false;

    // When connection should not be closed or
    // When lingering is activated
    if (!this->_check_prereqs() || this->_linger_after_streams_finish)
        return true;

    return false;
}

size_t TCPConnection::write(const string &data) {
    // Ignore when inactive
    if (!this->active())
        return 0;

    const size_t size = this->_sender.stream_in().write(data);

    this->_sender.fill_window();
    this->_send();

    return size;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    // Ignore when inactive
    if (!this->active())
        return;

    // Update _time_since_last_segment_received
    // Propagate tick to sender
    this->_time_since_last_segment_received += ms_since_last_tick;
    this->_sender.tick(ms_since_last_tick);

    // When too many retransmissions
    if (this->_sender.consecutive_retransmissions() > this->_cfg.MAX_RETX_ATTEMPTS)
        this->_report();

    // Stop lingering
    if (this->_check_prereqs() && _time_since_last_segment_received >= 10 * this->_cfg.rt_timeout)
        this->_linger_after_streams_finish = false;

    this->_send();
}

void TCPConnection::end_input_stream() {
    // Ignore when inactive
    if (!this->active())
        return;

    this->_sender.stream_in().end_input();
    this->_sender.fill_window();
    this->_send();
}

void TCPConnection::connect() {
    this->_sender.fill_window();
    this->_send();
}

bool TCPConnection::_check_prereqs() const {
    // Checking about the prerequisite conditions
    // returns true: when the connection can be closed
    // returns false: when the connection should not be closed

    // Sender side
    if (!(this->_fin && this->_sender.stream_in().eof()) || this->_sender.bytes_in_flight() != 0)
        return false;

    // Receiver side
    if (!this->_receiver.stream_out().eof())
        return false;

    return true;
}

void TCPConnection::_report() {
    // Mark as error occured and set errors to streams of sender and receiver
    this->_error = true;
    this->_sender.stream_in().set_error();
    this->_receiver.stream_out().set_error();
}

void TCPConnection::_send() {
    queue<TCPSegment> &waiting_queue = this->_sender.segments_out();

    // Until sender's segments_out queue is empty
    while (!waiting_queue.empty()) {
        TCPSegment segment = waiting_queue.front();
        waiting_queue.pop();

        TCPHeader &header = segment.header();

        // Mark fin when fin of segment is set
        if (header.fin)
            this->_fin = true;

        // Mark ack and ackno
        const optional<WrappingInt32> ackno = this->_receiver.ackno();
        if (ackno.has_value())
            header.ack = true, header.ackno = ackno.value();

        // Mark win and rst
        header.win = min(this->_receiver.window_size(), static_cast<size_t>(0xFFFF));
        header.rst = this->_error;

        // Push segment to outgoing segments
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
