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

void TCPConnection::segment_received(const TCPSegment &seg) { DUMMY_CODE(seg); }

bool TCPConnection::active() const {
    if (this->_error) return false;

    if (!this->_check() || this->_linger_after_streams_finish) return false;

    return true;
}

size_t TCPConnection::write(const string &data) {
    DUMMY_CODE(data);
    return {};
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) { DUMMY_CODE(ms_since_last_tick); }

void TCPConnection::end_input_stream() {}

void TCPConnection::connect() {
    this->_sender.fill_window();
    this->_send();
}

bool TCPConnection::_check() const {
    if (this->_sender.bytes_in_flight() != 0 || !(this->_fin && this->_sender.stream_in().eof())) return false;

    if (this->_receiver.stream_out().eof()) return false;

    return true;
}

void TCPConnection::_report() {
    this->_error = true;
    this->_sender.stream_in().set_error();
    this->_receiver.stream_out().set_error();
}

void TCPConnection::_send() {

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
