#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void ByteStream::_pop(const size_t size) {
    this->_buffer = this->_buffer.substr(size, this->buffer_size());
    this->_bytes_read += size;
}

string ByteStream::_peek(const size_t size) const { return this->_buffer.substr(0, size); }

ByteStream::ByteStream(const size_t capacity)
    : _capacity(capacity), _bytes_written(0), _bytes_read(0), _input_ended(false), _buffer(""), _error(false) {}

size_t ByteStream::write(const string &data) {
    size_t size = data.size() > this->remaining_capacity() ? this->remaining_capacity() : data.size();
    this->_buffer += data.substr(0, size);
    this->_bytes_written += size;
    return size;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    size_t size = len > this->buffer_size() ? this->buffer_size() : len;
    return this->_peek(size);
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    size_t size = len > this->buffer_size() ? this->buffer_size() : len;
    this->_pop(size);
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    size_t size = len > this->buffer_size() ? this->buffer_size() : len;
    const string data = this->_peek(size);
    this->_pop(size);
    return data;
}

void ByteStream::end_input() { _input_ended = true; }

bool ByteStream::input_ended() const { return this->_input_ended; }

size_t ByteStream::buffer_size() const { return this->_buffer.size(); }

bool ByteStream::buffer_empty() const { return this->_buffer.empty(); }

bool ByteStream::eof() const { return this->buffer_empty() && this->input_ended(); }

size_t ByteStream::bytes_written() const { return this->_bytes_written; }

size_t ByteStream::bytes_read() const { return this->_bytes_read; }

size_t ByteStream::remaining_capacity() const { return _capacity - this->buffer_size(); }
