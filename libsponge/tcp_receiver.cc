#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    const TCPHeader &header = seg.header();

    // Retrieve SYN, FIN flag and seqno from header
    const bool syn = header.syn;
    const bool fin = header.fin;
    const WrappingInt32 seqno = header.seqno;

    // Update _is_syn & _isn when SYN received
    if (!this->_is_syn && syn) {
        this->_is_syn = true;
        this->_isn = seqno;
    }

    // Abort when no SYN received
    if (!this->_is_syn) return;

    // Update _is_fin when FIN recevied
    if (fin) this->_is_fin = true;

    // Retreive data from payload
    const string data = seg.payload().copy();
    // Calculate stream index by unwrapping from seqno
    // Unwrap based on _isn with first unassembled index as checkpoint
    // seqno + syn considers the SYN within data
    const uint64_t stream_index = unwrap(WrappingInt32(seqno + syn), this->_isn, this->_first_unassembled()) - 1;

    // Push substring to reassembler
    this->_reassembler.push_substring(data, stream_index, fin);
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    // When no connection opened
    if (!this->_is_syn)
        return nullopt;
    
    // Calculate absolute seqno
    // Consider SYN at first
    // Consider FIN at last only when reassembler ended its job
    const uint64_t abs_seqno = this->_is_syn + this->_first_unassembled() + (this->_is_fin && this->_reassembler.empty());

    // Return wrapped absolute seqno as WrappingInt32
    return wrap(abs_seqno, this->_isn);
}

size_t TCPReceiver::window_size() const { return this->_capacity - this->stream_out().buffer_size(); }
