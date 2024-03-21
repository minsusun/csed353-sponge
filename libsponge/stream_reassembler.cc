#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity)
    : _output(capacity)
    , _capacity(capacity)
    , _unassembled_bytes(0)
    , _eof(false)
    , _buffer(capacity, ' ')
    , _state(capacity, false) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const uint64_t index, const bool eof) {
    const uint64_t first_unread = _output.bytes_read();
    const uint64_t first_unassmbled = _output.bytes_written();

    // Determine we reach the end of data and if it is fetch the eof
    if (index + data.length() <= first_unread + this->_capacity && eof)
        this->_eof = true;

    // Write and update data into buffer
    for (uint64_t data_index = 0; data_index < data.length(); data_index++) {
        const uint64_t buffer_index = data_index + index - first_unassmbled;

        if (data_index + index < first_unassmbled)
            continue;

        if (buffer_index >= this->_output.remaining_capacity())
            break;

        if (!this->_state[buffer_index]) {
            this->_state[buffer_index] = true;
            this->_unassembled_bytes++;
        }

        this->_buffer[buffer_index] = data[data_index];
    }

    // Retrieve the data from buffer
    string ret;
    while (this->_state.front() && ret.length() < this->_output.remaining_capacity()) {
        ret += this->_buffer.front();

        this->_buffer.pop_front();
        this->_state.pop_front();

        this->_buffer.push_back(' ');
        this->_state.push_back(false);
    }

    // Write data to bytestream
    this->_output.write(ret);
    this->_unassembled_bytes -= ret.length();

    // Determine end_input
    if (this->_eof && this->empty())
        this->_output.end_input();
}

size_t StreamReassembler::unassembled_bytes() const { return this->_unassembled_bytes; }

bool StreamReassembler::empty() const { return this->unassembled_bytes() == 0; }
