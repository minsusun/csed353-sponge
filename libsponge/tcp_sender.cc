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

size_t TCPSender::bytes_in_flight() const { return {}; }

void TCPSender::fill_window() {}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { DUMMY_CODE(ackno, window_size); }

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
        if (this->_window_size > 0) {
            this->_consecutive_retransmission++;    // Keep track of consecutive retransmission
            this->_retransmission_timeout <<= 1;
        }
        
        this->_timer_elapsed = 0;
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return this->_consecutive_retransmission; }

void TCPSender::send_empty_segment() {}
