#include "base.hpp"
#include <cstring>

namespace Coil
{
  Book::Book(Book&& book)
  {
    *this = std::move(book);
  }

  Book& Book::operator=(Book&& book)
  {
    std::swap(_lastChunk, book._lastChunk);
    std::swap(_lastChunkAllocated, book._lastChunkAllocated);
    std::swap(_lastObjectHeader, book._lastObjectHeader);
    return *this;
  }

  Book::~Book()
  {
    Free();
  }

  void Book::Free()
  {
    while(_lastObjectHeader)
    {
      ObjectHeader* prev = _lastObjectHeader->prev;
      _lastObjectHeader->~ObjectHeader();
      _lastObjectHeader = prev;
    }
    _lastChunk = nullptr;
    _lastChunkAllocated = 0;
    _lastObjectHeader = nullptr;
  }

  void* Book::_AllocateFromPool(size_t size)
  {
    if(!_lastChunk || _lastChunkAllocated + size > _ChunkSize)
    {
      _lastChunk = new uint8_t[_ChunkSize];
      _lastChunkAllocated = 0;
      InitObject<_Chunk>(_lastChunk);
    }
    void* data = _lastChunk + _lastChunkAllocated;
    _lastChunkAllocated += size;
    return data;
  }

  Memory::Memory(uint8_t* const data)
  : _data(data) {}

  Memory::~Memory()
  {
    delete [] _data;
  }

  Buffer Memory::Allocate(Book& book, size_t size)
  {
    uint8_t* data = new uint8_t[size];
    book.Allocate<Memory>(data);
    return Buffer(data, size);
  }

  BufferOutputStream::BufferOutputStream(Buffer const& buffer)
  : _buffer(buffer) {}

  void BufferOutputStream::Write(Buffer const& buffer)
  {
    if(_buffer.size < buffer.size)
      throw Exception("BufferOutputStream: end of dest buffer");
    memcpy(_buffer.data, buffer.data, buffer.size);
    _buffer.data = (uint8_t*)_buffer.data + buffer.size;
    _buffer.size -= buffer.size;
  }

  BufferInputStream::BufferInputStream(Buffer const& buffer)
  : _buffer(buffer) {}

  size_t BufferInputStream::Read(Buffer const& buffer)
  {
    size_t size = std::min(buffer.size, _buffer.size);
    memcpy(buffer.data, _buffer.data, size);
    _buffer.data = (uint8_t*)_buffer.data + size;
    _buffer.size -= size;
    return size;
  }
}
