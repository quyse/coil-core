#pragma once

#include <vector>
#include <cstddef>
#include <cstdint>

// platform definitions

#if defined(_WIN32) || defined(__WIN32__)
  #define ___COIL_PLATFORM_WINDOWS
#elif defined(__APPLE__) && defined(__MACH__)
  #define ___COIL_PLATFORM_MACOS
  #define ___COIL_PLATFORM_POSIX
#else
  #define ___COIL_PLATFORM_LINUX
  #define ___COIL_PLATFORM_POSIX
#endif

namespace Coil
{
  // Object is something which can be registered in a Book.
  class Object
  {
  public:
    virtual ~Object() {};
  };

  // Book is a container for objects to remove them later.
  class Book : public Object
  {
  public:
    Book() = default;
    Book(Book const&) = delete;
    Book(Book&&);
    ~Book();

    Book& operator=(Book const&) = delete;
    Book& operator=(Book&&);

    // Allocate new object in pool.
    template <typename T, typename... Args>
    T* Allocate(Args&&... args)
    {
      static_assert(sizeof(T) <= _ChunkSize, "too big object to allocate in a book");
      T* object = new (_AllocateFromPool(sizeof(T))) T(std::forward<Args...>(args...));
      _Register(object);
      return object;
    }
    // Delete all objects.
    void Free();

  private:
    void* _AllocateFromPool(size_t size);
    void _Register(Object* object);

    std::vector<Object*> _objects;
    struct _Chunk;
    _Chunk* _lastChunk = nullptr;
    size_t _lastChunkAllocated = 0;

    static constexpr size_t _ChunkSize = 0x1000;
  };

  // Piece of allocated or mapped memory.
  struct Buffer
  {
    Buffer(void* data = nullptr, size_t size = 0)
    : data(data), size(size) {}
    Buffer(void const* data, size_t size)
    : data(const_cast<void*>(data)), size(size) {}

    Buffer slice(size_t sliceOffset, size_t sliceSize)
    {
      return Buffer(static_cast<uint8_t* const>(data) + sliceOffset, sliceSize);
    }

    void* data;
    size_t size;
  };

  // Memory allocating with new uint8_t[].
  class Memory : public Object
  {
  public:
    Memory(uint8_t* const data);
    ~Memory();

    static Buffer Allocate(Book& book, size_t size);

  private:
    uint8_t* const _data;
  };

  class Exception
  {
  public:
    Exception(char const* message)
    : _message(message) {}

    char const* GetMessage() const
    {
      return _message;
    }

  private:
    char const* const _message;
  };

  // Big size, unlike size_t not depending on size of addressable memory.
  // Should be used for things like file/stream sizes.
  using bigsize_t = uint64_t;

  // Output stream.
  class OutputStream
  {
  public:
    virtual void Write(Buffer const& buffer) = 0;
  };

  // Input stream.
  class InputStream
  {
  public:
    // Read some data, up to size of the buffer.
    // Always fills the buffer fully, unless there's not enough data.
    virtual size_t Read(Buffer const& buffer) = 0;
  };

  // Output stream writing into buffer.
  class BufferOutputStream : public OutputStream
  {
  public:
    BufferOutputStream(Buffer const& buffer);

    void Write(Buffer const& buffer) override;

  private:
    Buffer _buffer;
  };

  // Input stream reading from buffer.
  class BufferInputStream : public InputStream
  {
  public:
    BufferInputStream(Buffer const& buffer);

    size_t Read(Buffer const& buffer) override;

  private:
    Buffer _buffer;
  };
}
