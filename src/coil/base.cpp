#include "base.hpp"

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


#if defined(__cpp_lib_source_location) && !defined(NDEBUG)
  Exception::Exception(std::source_location location)
  {
    *this << location.file_name() << ':' << location.line() << ' ' << location.function_name() << ": ";
  }
#endif

  std::string Exception::GetMessage() const
  {
    return _message.str();
  }

  Exception& operator<<(Exception& e, Exception const& inner)
  {
    e._message << '\n' << inner._message.str();
    return e;
  }
  Exception operator<<(Exception&& e, Exception const& inner)
  {
    e._message << '\n' << inner._message.str();
    return std::move(e);
  }


  size_t InputStream::Skip(size_t size)
  {
    uint8_t bufferData[0x1000];
    size_t totalSkippedSize = 0;
    while(size)
    {
      size_t sizeToSkip = std::min(size, sizeof(bufferData));
      Buffer buffer(bufferData, sizeToSkip);
      size_t skippedSize = Read(buffer);
      if(!skippedSize) break;
      totalSkippedSize += skippedSize;
      size -= skippedSize;
    }
    return totalSkippedSize;
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
}
