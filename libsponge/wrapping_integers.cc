#include "wrapping_integers.hh"

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) { return WrappingInt32(isn + n); }

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    // Retrieve lower 32 bits of total 64 bits. Automatic rounding.
    const uint64_t lower_32 = n.raw_value() - isn.raw_value();
    // Retrieve upper 32 bits of total 64 bits.
    // By now (upper_32 << 32) | lower_32 < checkpoint holds unless upper_32 is 0xFFFFFFFF
    const uint64_t upper_32 = (checkpoint - lower_32) >> 32;

    // Edge case
    if (checkpoint <= lower_32)
        return lower_32;

    // Candidates for unwrapped result
    const uint64_t low = (upper_32 << 32) | lower_32;
    const uint64_t high = ((upper_32 + 1) << 32) | lower_32;

    // Determine which candidate to return
    if (checkpoint - low >= high - checkpoint)
        return high;
    else
        return low;
}
