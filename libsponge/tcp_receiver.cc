#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    const TCPHeader &header = seg.header();

    const bool syn = header.syn;
    const bool fin = header.fin;
    const WrappingInt32 seqno = header.seqno;

    if (!this->_is_syn && syn) {
        this->_is_syn = true;
        this->_isn = seqno;
    }

    if (!this->_is_syn) return;

    if (fin) this->_is_fin = true;

    const string data = seg.payload().copy();
    const uint64_t stream_index = unwrap(WrappingInt32(seqno + syn), this->_isn, this->_first_unassembled()) - 1;

    this->_reassembler.push_substring(data, stream_index, fin);
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (!this->_is_syn)
        return nullopt;
    
    const uint64_t abs_seqno = this->_is_syn + this->_first_unassembled() + (this->_is_fin && this->_reassembler.empty());

    return wrap(abs_seqno, this->_isn);
}

size_t TCPReceiver::window_size() const { return this->_capacity - this->stream_out().buffer_size(); }
