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
    , _ack_no(0)
    , elapsed_time(0)
    , _rto(0)
    , timer_run(false)
    , num_cons_trans(0)
    , _window_size(1)
    , fin_sent(false) {}

uint64_t TCPSender::bytes_in_flight() const { return _next_seqno - _ack_no; }

void TCPSender::fill_window() {
    while ((_window_size >= bytes_in_flight()) &&
           !fin_sent)  // check availability about sending packet and whether fin packet was made or not
    {
        TCPSegment segment;
        size_t payload_size = _window_size - bytes_in_flight();  // for flow control

        if (_window_size == 0 && bytes_in_flight() == 0)  // treat 0 window size as 1
        {
            payload_size = 1;
        }

        if (_next_seqno == 0 &&
            payload_size != 0)  // make syn flag and syn flag also occupy the byte of string, so reduce payload size
        {
            segment.header().syn = true;
            payload_size--;
        }

        if (payload_size > TCPConfig::MAX_PAYLOAD_SIZE)  // limitation by MSS
        {
            payload_size = TCPConfig::MAX_PAYLOAD_SIZE;
        }

        segment.header().seqno = wrap(_next_seqno, _isn);  // make sequence number of segment
        if (payload_size != 0 && !_stream.eof())           // check get something or not
        {
            segment.payload() = _stream.read(payload_size);
        }

        if (_stream.eof() && ((_window_size - bytes_in_flight()) > segment.length_in_sequence_space() ||
                              (_window_size == 0 && bytes_in_flight() == 0 &&
                               segment.length_in_sequence_space() ==
                                   0)))  // eof, okay to add fin to flow control or empty packet->set fin flag
        {
            segment.header().fin = true;
            fin_sent = true;
        }

        if (segment.length_in_sequence_space() == 0)  // nothing be sent, then do nothing
        {
            return;
        }

        _segments_out.push(segment);                            // store to send
        _segments_unacked.push(unacked{_next_seqno, segment});  // store to retransmit if negative acknowledgement
        _next_seqno += segment.length_in_sequence_space();      // update sequence number

        if (!timer_run)  // if no timer run, then start timer
        {
            elapsed_time = 0;
            _rto = _initial_retransmission_timeout;
            timer_run = true;
        }
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    uint64_t new_ackno = unwrap(ackno, _isn, _ack_no);  // get acknowledgement number
    _window_size = window_size;                         // check receiver's window size for flow control

    if ((new_ackno > _ack_no) && (new_ackno <= _next_seqno))  // if ackno is not in sender's window, ignore
    {
        timer_run = false;  // stop timer

        _ack_no = new_ackno;  // store accumulate ack

        num_cons_trans = 0;  // reset number of consecutive number because ack is received

        while ((!_segments_unacked.empty()) &&
               ((_segments_unacked.front().seqno + _segments_unacked.front().packet.length_in_sequence_space()) <=
                new_ackno))  // check packet which is acked
        {
            _segments_unacked.pop();
        }

        if (!_segments_unacked.empty())  // if there is unacked packet, start timer
        {
            elapsed_time = 0;
            _rto = _initial_retransmission_timeout;
            timer_run = true;
        }
    }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    if (timer_run)  // only run when timer is running
    {
        elapsed_time += ms_since_last_tick;  // update elapsed time

        if (elapsed_time >= _rto)  // timeout
        {
            _segments_out.push(_segments_unacked.front().packet);  // retransmit first unacked packet

            elapsed_time = 0;

            if (_window_size != 0)  // if window size is not 0, then exponential backoff
            {
                _rto *= 2;
                num_cons_trans++;
            }
        }
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return num_cons_trans; }

void TCPSender::send_empty_segment() {  // just set seqno and send
    TCPSegment segment;
    segment.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.push(segment);
}
