#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

inline void StreamReassembler::_fetchAssembled() {
    for (; _assembled_iterator != _buffer.end() && _assembled_cursor == (*_assembled_iterator).first;
         _assembled_iterator++, _assembled_cursor++)
        _unassembled_bytes--;
}

inline void StreamReassembler::_pushAssembled() {
    for (auto it = _buffer.begin(); it != _assembled_iterator;) {
        if (_output.buffer_size() >= _capacity)
            break;
        else {
            _output.write((*it).second);
            it = _buffer.erase(it);
            _buffer_size--;
        }
    }
}

StreamReassembler::StreamReassembler(const size_t capacity)
    : _buffer()
    , _assembled_iterator(_buffer.begin())
    , _buffer_size(0)
    , _assembled_cursor(0)
    , _unassembled_bytes(0)
    , _eof(false)
    , _output(capacity)
    , _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const uint64_t index, const bool eof) {
    uint64_t pos = index;
    for (auto &elem : data) {
        // ignore already assembled bytes
        if (pos < _assembled_cursor) {
            pos++;
            continue;
        }

        // try to push assembled data
        _pushAssembled();

        // buffer is full
        if (_buffer_size >= _capacity)
            break;

        // map.insert.second will be true when new data is inserted, false for duplicated data
        if (_buffer.insert({pos++, to_string(elem)}).second)
            _buffer_size++, _unassembled_bytes++;

        // fetch assembled iterator to new assembled data and try to push assembled data
        _fetchAssembled();
        _pushAssembled();

        // fetch eof value when loop reach the end of data
        if (elem == data[data.size() - 1])
            _eof = eof;
    }

    if (_eof && empty())
        _output.end_input();
}

size_t StreamReassembler::unassembled_bytes() const { return _unassembled_bytes; }

bool StreamReassembler::empty() const { return _buffer_size == 0; }
