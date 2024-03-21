#include "stream_reassembler.hh"

#include <iostream>

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity)
    : _output(capacity)
    , _capacity(capacity)
    , _unassembled_bytes(0)
    , _bytes_read(0)
    , _buffer(capacity, make_pair('_', false))
    , _eof(0)
    , _eof_index(0) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    // update buffer size when read bytes are changed
    auto first_unread = _output.bytes_read();
    if (_bytes_read != first_unread) {
        for (size_t i = _bytes_read; i < first_unread; i++)
            _buffer.push_back(make_pair('_', false));
        _bytes_read = first_unread;
    }

    // upload input data to buffer only acceptable
    auto first_unassembled = _output.bytes_written();
    for (size_t i = 0; i < data.size(); i++) {
        if (i + index < first_unassembled)
            continue;
        auto buffer_index = i + index - first_unassembled;
        if (buffer_index >= _buffer.size())
            continue;
        if (!_buffer[buffer_index].second) {
            _buffer[buffer_index].first = data[i];
            _buffer[buffer_index].second = true;
            _unassembled_bytes++;
        }
    }

    // write assembled data to _output
    string assembled;
    while (!_buffer.empty() && _buffer[0].second) {
        assembled += _buffer[0].first;
        _buffer.pop_front();
    }
    _output.write(assembled);
    _unassembled_bytes -= assembled.size();

    // notify end input to _output when achieving eof
    if (eof) {
        _eof = eof;
        _eof_index = index + data.size();
    }
    if (_output.bytes_written() == _eof_index && _eof) {
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const { return _unassembled_bytes; }

bool StreamReassembler::empty() const { return unassembled_bytes() == 0; }
