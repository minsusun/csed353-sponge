#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _time; }

void TCPConnection::segment_received(const TCPSegment &seg) {
    _time = 0;

    if (seg.header().rst) {
        _sender.stream_in().set_error();
        _receiver.stream_out().set_error();
        _active = false;
        return;
    }

    _receiver.segment_received(seg);

    if (seg.header().ack) {
        _sender.ack_received(seg.header().ackno, seg.header().win);
        sending_segments();
    }

    if (seg.length_in_sequence_space() > 0) {
        _sender.fill_window();
        if (_sender.segments_out().size() == 0) {
            _sender.send_empty_segment();
            TCPSegment segment = _sender.segments_out().front();
            if (_receiver.ackno().has_value()) {
                segment.header().ack = true;
                segment.header().ackno = _receiver.ackno().value();
                segment.header().win = static_cast<uint16_t>(_receiver.window_size());
            }
            _sender.segments_out().pop();
            _segments_out.push(segment);
        } else {
            sending_segments();
        }
    }
    return;
}

bool TCPConnection::active() const { return _active; }

size_t TCPConnection::write(const string &data) {
    if (!data.size()) {
        return 0;
    }
    size_t written_data_size = _sender.stream_in().write(data);
    _sender.fill_window();
    sending_segments();
    return written_data_size;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    _time += ms_since_last_tick;
    _sender.tick(ms_since_last_tick);

    if (_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS) {
        reset();
    }

    sending_segments();
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    _sender.fill_window();
    sending_segments();
}

void TCPConnection::connect() {
    _sender.fill_window();
    sending_segments();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
            _sender.send_empty_segment();
            reset();
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}

void TCPConnection::sending_segments() {
    TCPSegment segment;

    while (!_sender.segments_out().empty()) {
        segment = _sender.segments_out().front();

        if (_receiver.ackno().has_value()) {
            segment.header().ack = true;
            segment.header().ackno = _receiver.ackno().value();
            segment.header().win = static_cast<uint16_t>(_receiver.window_size());
        }

        _segments_out.push(segment);
        _sender.segments_out().pop();
    }

    if (_receiver.stream_out().input_ended() && !_sender.stream_in().eof()) {
        _linger_after_streams_finish = false;
    }
    if (_receiver.stream_out().input_ended() && _sender.bytes_in_flight() == 0 && _sender.stream_in().eof()) {
        if (time_since_last_segment_received() >= 10 * _cfg.rt_timeout || !_linger_after_streams_finish) {
            _active = false;
        }
    }
}

void TCPConnection::reset() {
    _receiver.stream_out().set_error();
    _sender.stream_in().set_error();
    _active = false;
    TCPSegment seg = _sender.segments_out().front();

    seg.header().rst = true;
    if (_receiver.ackno().has_value()) {
        seg.header().ack = true;
        seg.header().ackno = _receiver.ackno().value();
        seg.header().win = static_cast<uint16_t>(_receiver.window_size());
    }
    _sender.segments_out().pop();
    _segments_out.push(seg);
}
