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
    , _retransmission_timeout(retx_timeout)
    , _stream(capacity) {}

size_t TCPSender::bytes_in_flight() const { return this->_next_seqno - this->_current_seqno; }

void TCPSender::fill_window() {}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    // Retreive absolute ack number from ackno
    const uint64_t absolute_ackno = unwrap(ackno, this->_isn, this->_next_seqno);

    // Update for only available absolute ackno
    if (absolute_ackno <= this->_next_seqno) {
        // Update _current_seqno & _window_size
        this->_current_seqno = max(this->_current_seqno, absolute_ackno);
        this->_receiver_window_size = window_size;

        // Ignore when timer is off
        if (!this->_is_timer_on)
            return;

        // Pop acked segments from outstanding segments queue
        while (!this->_outstanding_segments.empty()) {
            const TCPSegment segment = this->_outstanding_segments.front();
            
            if (unwrap(segment.header().seqno, this->_isn, this->_next_seqno) > absolute_ackno)
                break;

            // Reset timer when the first outstanding segment is acked, which is about to be pop
            this->_retransmission_timeout = this->_initial_retransmission_timeout;
            this->_consecutive_retransmissions = 0;
            this->_timer_elapsed = 0;

            this->_outstanding_segments.pop();
        }
    }

    this->fill_window();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    // Ignore when timer is not activated
    if (!this->_is_timer_on)
        return;
    
    // Update elapsed time
    this->_timer_elapsed += ms_since_last_tick;

    if (this->_timer_elapsed >= this->_retransmission_timeout) {
        const TCPSegment segment = this->_outstanding_segments.front();
        
        this->_segments_out.push(segment);
        
        // Exponential Backoff
        // Do only when window size of stream is not zero
        if (this->_receiver_window_size > 0) {
            this->_consecutive_retransmissions++;    // Keep track of consecutive retransmission
            this->_retransmission_timeout <<= 1;
        }
        
        this->_timer_elapsed = 0;
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return this->_consecutive_retransmissions; }

void TCPSender::send_empty_segment() {
    TCPSegment segment;
    segment.header().seqno = wrap(this->_next_seqno, this->_isn);
    this->_segments_out.push(segment);
}
