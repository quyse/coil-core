#include "base.hpp"
#include <cstring>

namespace Coil
{
  Book::Book(Book&& book) noexcept
  {
    *this = std::move(book);
  }

  Book& Book::operator=(Book&& book) noexcept
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
      _lastChunkAllocated = sizeof(TemplObjectHeader<_Chunk>);
      InitObject<_Chunk>(_lastChunk);
    }
    void* data = _lastChunk + _lastChunkAllocated;
    _lastChunkAllocated += (size + _MaxAlignment - 1) & ~(_MaxAlignment - 1);
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
    return { data, size };
  }

#if defined(__cpp_lib_source_location)
  Exception::Exception(std::source_location location)
  {
    std::move(*this) << location.file_name() << ':' << location.line() << ' ' << location.function_name() << ": ";
  }
#endif

  std::string Exception::GetMessage() const
  {
    return _message.str();
  }

  Exception&& operator<<(Exception&& e, Exception const& inner)
  {
    e._message << '\n' << inner._message.str();
    return std::move(e);
  }

  void OutputStream::WriteAllFrom(InputStream& inputStream)
  {
    uint8_t bufferData[0x1000];
    Buffer buffer(bufferData, sizeof(bufferData));
    for(;;)
    {
      size_t size = inputStream.Read(buffer);
      if(!size) break;
      Write(Buffer(buffer.data, size));
    }
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

  void MemoryStream::Write(Buffer const& buffer)
  {
    size_t initialSize = _data.size();
    _data.resize(initialSize + buffer.size);
    memcpy(_data.data() + initialSize, buffer.data, buffer.size);
  }

  Buffer MemoryStream::ToBuffer() const
  {
    return _data;
  }
}
