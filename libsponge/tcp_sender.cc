#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , rto{retx_timeout} {}

uint64_t TCPSender::bytes_in_flight() const { return _bytes_in_flight; }

void TCPSender::fill_window() {
    if (!_next_seqno) {
        TCPSegment segment;

        segment.header().syn = true;
        segment.header().seqno = wrap(_next_seqno, _isn);

        _next_seqno += segment.length_in_sequence_space();
        _bytes_in_flight += segment.length_in_sequence_space();

        _segments_out.push(segment);
        outstanding_seg.push(segment);

        if (!timer_started) {
            timer_started = true;
            time = 0;
        }

        return;
    }

    uint16_t free_window = 0;

    free_window = (_window_size > 0 ? _window_size : 1) + absolute_ack - _next_seqno;

    while (free_window) {
        TCPSegment segment;

        if (fin) {
            break;
        }

        size_t payload_length = min({static_cast<size_t>(free_window),
                                     static_cast<size_t>(TCPConfig::MAX_PAYLOAD_SIZE),
                                     _stream.buffer_size()});
        segment.payload() = Buffer{_stream.read(payload_length)};

        if (static_cast<size_t>(free_window) > payload_length && _stream.eof()) {
            segment.header().fin = true;
            fin = true;
        }

        if (!segment.length_in_sequence_space()) {
            return;
        }

        free_window -= static_cast<uint64_t>(segment.length_in_sequence_space());

        segment.header().seqno = wrap(_next_seqno, _isn);

        _next_seqno += segment.length_in_sequence_space();
        _bytes_in_flight += segment.length_in_sequence_space();

        _segments_out.push(segment);
        outstanding_seg.push(segment);

        if (!timer_started) {
            timer_started = true;
            time = 0;
        }
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    absolute_ack = unwrap(ackno, _isn, _next_seqno);
    _window_size = window_size;

    if (absolute_ack > _next_seqno) {
        return;
    }

    while (!outstanding_seg.empty()) {
        if (absolute_ack >= unwrap(outstanding_seg.front().header().seqno, _isn, _next_seqno) +
                                outstanding_seg.front().length_in_sequence_space()) {
            _bytes_in_flight -= outstanding_seg.front().length_in_sequence_space();
            outstanding_seg.pop();

            time = 0;
            rto = _initial_retransmission_timeout;
            consecutive_retransmission = 0;
        } else {
            break;
        }
    }

    if (!outstanding_seg.size()) {
        timer_started = false;
    }

    fill_window();
    return;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    time += ms_since_last_tick;

    if (time >= rto && outstanding_seg.size()) {
        _segments_out.push(outstanding_seg.front());
        consecutive_retransmission++;

        if (_window_size || outstanding_seg.front().header().syn) {
            rto *= 2;
        }

        time = 0;
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return consecutive_retransmission; }

void TCPSender::send_empty_segment() {
    TCPSegment segment;
    segment.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.push(segment);
}
