#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

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

void TCPSender::fill_window() {
    // Shortcut for TCPConfig::MAX_PAYLOAD_SIZE
    const size_t &MAX_PAYLOAD_SIZE = TCPConfig::MAX_PAYLOAD_SIZE;

    // Current state of SYN & FIN flag which is needed to be transfered
    const bool has_syn = this->_next_seqno == 0;
    const bool has_fin = this->_stream.input_ended() && !this->_fin;

    // Window_size considering SYN & FIN flags
    const size_t total_window_size = has_syn + this->stream_in().buffer_size() + has_fin;

    // FIN flag can be sent only when last segment is able to be sent
    const bool reach_fin = has_fin && this->_window_size >= total_window_size;

    // Actual available window size
    const size_t actual_window_size = min(total_window_size, static_cast<size_t>(this->_window_size));
    // Actual payload size with actual window size
    // Payload size does not consider SYN & FIN
    const size_t actual_payload_size = actual_window_size - has_syn - reach_fin;

    // Number of segments, divided by MAX_PAYLOAD_SIZE
    // When actual payload size is not enough for create a payload(equals 0),
    // Number of segments is 1, which is open for flags(only when flag needed to be sent)
    const size_t num_segments =
        max((actual_payload_size + MAX_PAYLOAD_SIZE - 1) / MAX_PAYLOAD_SIZE, static_cast<size_t>(has_syn || reach_fin));

    for (size_t index = 0; index < num_segments; index++) {
        TCPSegment segment;

        TCPHeader &header = segment.header();

        header.syn = index == 0 && has_syn;
        header.fin = index == num_segments - 1 && reach_fin;
        header.seqno = wrap(this->_next_seqno, this->_isn);

        const size_t size =
            (index == num_segments - 1) ? actual_payload_size - MAX_PAYLOAD_SIZE * index : MAX_PAYLOAD_SIZE;
        segment.payload() = Buffer(this->stream_in().read(size));

        this->_next_seqno += segment.length_in_sequence_space();
        this->_window_size -= segment.length_in_sequence_space();

        this->_segments_out.push(segment);
        this->_outstanding_segments.push(segment);

        this->_fin |= header.fin;

        // Timer initiation, timer needs to be turned on upon sending segments
        // No need to be resetted when already timer is running
        if (!this->_is_timer_on) {
            this->_is_timer_on = true;
            this->_retransmission_timeout = this->_initial_retransmission_timeout;
            this->_timer_elapsed = 0;
            this->_consecutive_retransmissions = 0;
        }
    }
}

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

        // Update estimated receiver window size
        // 1. lower bound of available window should be 1 which is for flags
        // 2. estimated receiver window size should be considered with in_flight segments
        // 3. lower bound of estimated receiver window size is 0
        this->_window_size =
            max(max(window_size, static_cast<uint16_t>(1)) - this->bytes_in_flight(), static_cast<size_t>(0));

        // Ignore when timer is off
        if (!this->_is_timer_on)
            return;

        // Pop acked segments from outstanding segments queue
        while (!this->_outstanding_segments.empty()) {
            const TCPSegment segment = this->_outstanding_segments.front();

            if (unwrap(segment.header().seqno, this->_isn, this->_next_seqno) + segment.length_in_sequence_space() >
                this->_current_seqno)
                break;

            // Reset timer when the first outstanding segment is acked, which is about to be pop
            this->_retransmission_timeout = this->_initial_retransmission_timeout;
            this->_consecutive_retransmissions = 0;
            this->_timer_elapsed = 0;

            this->_outstanding_segments.pop();
        }

        // try to fill window
        this->fill_window();
    }

    // timer should be turned off when there is no outstanding segment
    if (this->_outstanding_segments.empty())
        this->_is_timer_on = false;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    // Ignore when timer is not activated
    if (!this->_is_timer_on)
        return;

    // Update elapsed time
    this->_timer_elapsed += ms_since_last_tick;

    if (this->_timer_elapsed >= this->_retransmission_timeout) {
        // Retransmit the first unacked segment
        this->_segments_out.push(this->_outstanding_segments.front());

        // Exponential Backoff
        // Do only when window size of stream is not zero
        if (this->_receiver_window_size > 0) {
            this->_consecutive_retransmissions++;  // Keep track of consecutive retransmission
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
