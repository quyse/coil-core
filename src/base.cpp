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
    std::swap(_objects, book._objects);
    std::swap(_lastChunk, book._lastChunk);
    std::swap(_lastChunkAllocated, book._lastChunkAllocated);
    return *this;
  }

  Book::~Book()
  {
    Free();
  }

  void Book::Free()
  {
    while(!_objects.empty())
    {
      _objects.back()->~Object();
      _objects.pop_back();
    }
    _lastChunk = nullptr;
    _lastChunkAllocated = 0;
  }

  struct Book::_Chunk : public Object
  {
    _Chunk()
    : data(new uint8_t[_ChunkSize]) {}
    ~_Chunk()
    {
      delete [] data;
    }

    uint8_t* const data;
  };

  void* Book::_AllocateFromPool(size_t size)
  {
    if(!_lastChunk || _lastChunkAllocated + size > _ChunkSize)
    {
      _lastChunk = new _Chunk();
      _lastChunkAllocated = 0;
      _Register(_lastChunk);
    }
    void* data = _lastChunk->data + _lastChunkAllocated;
    _lastChunkAllocated += size;
    return data;
  }

  void Book::_Register(Object* object)
  {
    _objects.push_back(object);
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
