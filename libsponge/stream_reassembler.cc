#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

inline void StreamReassembler::_fetchAssembled() {
    for (_assembled_iterator = _buffer.begin(); _assembled_iterator != _buffer.end() && _assembled_cursor > (*_assembled_iterator).first; _assembled_iterator++);
    for (; _assembled_iterator != _buffer.end() && _assembled_cursor == (*_assembled_iterator).first;
         _assembled_iterator++, _assembled_cursor++, _unassembled_bytes--);
}

inline void StreamReassembler::_pushAssembled() {
    for (auto it = _buffer.begin(); it != _assembled_iterator;) {
        if (_output.buffer_size() >= _capacity)
            break;
        else {
            cout << "!" << (*it).first << " " << (*it).second << endl;
            _output.write((*it).second);
            it = _buffer.erase(it);
            _buffer_size--;
            break;
        }
    }
}

inline void StreamReassembler::_pushAssembledAll() {
    for (auto it = _buffer.begin(); it != _assembled_iterator;) {
        if (_output.buffer_size() >= _capacity)
            break;
        else {
            cout << "!" << (*it).first << " " << (*it).second << endl;
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
    cout << data.size() << " " << index << " " << eof << endl;
    cout << _assembled_cursor << endl;
    cout << _eof << endl;
    
    if(data.empty() && eof) _eof = true;

    uint64_t pos = index;
    for (auto it = data.begin(); it != data.end(); it++) {
        // try to push assembled data
        // _pushAssembled();

        if (pos < _output.bytes_written()) {
            pos++;
            continue;
        }

        if (_buffer.find(pos) == _buffer.end()) {
            if (_buffer_size >= _capacity) break;

            if (_buffer_size == _capacity - 1 && pos != _assembled_cursor) break;

            _buffer_size++, _unassembled_bytes++;
        }
        
        if(next(it) == data.end() && eof) _eof = true;

        _buffer[pos++] = string(it, next(it));

        // when buffer is full
        // if (_buffer_size >= _capacity) break;

        // if (_buffer_size == _capacity - 1 && pos != _assembled_cursor) break; 

        // if(next(it) == data.end() && eof) _eof = true;

        // // ignore already assembled bytes
        // if (pos < _assembled_cursor) {
        //     if(_buffer.find(pos) != _buffer.end()) _buffer[pos] = string(it, next(it));
        //     pos++;
        //     continue;
        // }
            
        // // index not existed
        // if (_buffer.find(pos) == _buffer.end()) _buffer_size++, _unassembled_bytes++;
        // _buffer[pos++] = string(it, next(it));

        // fetch assembled iterator to new assembled data and try to push assembled data
        _fetchAssembled();
        _pushAssembled();
    }

    _pushAssembledAll();

    if (_eof && empty())
        _output.end_input();
    cout << _buffer_size << endl;
    cout << _output.input_ended() << endl;
}

size_t StreamReassembler::unassembled_bytes() const { return _unassembled_bytes; }

bool StreamReassembler::empty() const { return _buffer_size == 0; }
