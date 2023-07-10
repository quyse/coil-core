#pragma once

#include <vector>
#include <sstream>
#include <version>
#if defined(__cpp_lib_source_location)
#include <source_location>
#endif
#include <cstddef>
#include <cstdint>

// platform definitions

#if defined(_WIN32) || defined(__WIN32__)
  #define COIL_PLATFORM_WINDOWS
#elif defined(__APPLE__) && defined(__MACH__)
  #define COIL_PLATFORM_MACOS
  #define COIL_PLATFORM_POSIX
#else
  #define COIL_PLATFORM_LINUX
  #define COIL_PLATFORM_POSIX
#endif

namespace Coil
{
  // Book is a container for objects to remove them later.
  class Book
  {
  public:
    Book() = default;
    Book(Book const&) = delete;
    Book(Book&&) noexcept;
    ~Book();

    Book& operator=(Book const&) = delete;
    Book& operator=(Book&&) noexcept;

    // Allocate new object in pool.
    template <typename T, typename... Args>
    T& Allocate(Args&&... args)
    {
      static_assert(sizeof(TemplObjectHeader<T>) + sizeof(T) <= _MaxObjectSize, "too big object to allocate in a book");
      static_assert(alignof(T) <= _MaxAlignment, "unsupported object alignment to allocate in a book");
      return InitObject<T, Args...>(
        _AllocateFromPool(sizeof(TemplObjectHeader<T>) + sizeof(T)),
        std::forward<Args>(args)...
      );
    }
    // Delete all objects.
    void Free();

  private:
    // Init object using provided memory.
    template <typename T, typename... Args>
    T& InitObject(void* memory, Args&&... args)
    {
      ObjectHeader* header = new (memory) TemplObjectHeader<T>(_lastObjectHeader);
      _lastObjectHeader = header;
      return *new (header + 1) T(std::forward<Args>(args)...);
    }

    struct ObjectHeader
    {
      ObjectHeader(ObjectHeader* prev)
      : prev(prev) {}
      virtual ~ObjectHeader() {}

      ObjectHeader* prev;
    };
    template <typename T>
    struct TemplObjectHeader;

    void* _AllocateFromPool(size_t size);
    void _Register(ObjectHeader* objectHeader);

    struct _Chunk {};
    uint8_t* _lastChunk = nullptr;
    size_t _lastChunkAllocated = 0;
    ObjectHeader* _lastObjectHeader = nullptr;

    static constexpr size_t _ChunkSize = 0x1000 - 128; // try to (over)estimate heap's overhead
    static constexpr size_t _MaxObjectSize = _ChunkSize - sizeof(ObjectHeader);
    static constexpr size_t _MaxAlignment = 16;
  };

  template <typename T>
  struct alignas(Book::_MaxAlignment) Book::TemplObjectHeader final : public Book::ObjectHeader
  {
    TemplObjectHeader(ObjectHeader* prev)
    : ObjectHeader(prev) {}
    ~TemplObjectHeader()
    {
      reinterpret_cast<T*>(this + 1)->~T();
    }
  };
  // specialization for chunks
  template <>
  struct Book::TemplObjectHeader<Book::_Chunk> final : public Book::ObjectHeader
  {
    TemplObjectHeader(ObjectHeader* prev)
    : ObjectHeader(prev) {}
    ~TemplObjectHeader()
    {
      delete [] reinterpret_cast<uint8_t*>(this);
    }
  };

  // Piece of allocated or mapped memory.
  struct Buffer
  {
    Buffer()
    : data(nullptr), size(0) {}
    Buffer(void* data, size_t size)
    : data(data), size(size) {}
    Buffer(void const* data, size_t size)
    : data(const_cast<void*>(data)), size(size) {}
    Buffer(size_t size)
    : data(nullptr), size(size) {}

    template <typename T>
    Buffer(std::vector<T>& v)
    : data(v.data()), size(v.size() * sizeof(T)) {}
    template <typename T>
    Buffer(std::vector<T> const& v)
    : data(const_cast<void*>(static_cast<void const*>(v.data()))), size(v.size() * sizeof(T)) {}

    template <typename T, size_t n>
    Buffer(T (&init)[n])
    : data(init), size(n * sizeof(T)) {}
    template <typename T, size_t n>
    Buffer(T const (&init)[n])
    : data(const_cast<void*>(static_cast<void const*>(init))), size(n * sizeof(T)) {}

    operator bool() const
    {
      return data && size;
    }

    Buffer slice(size_t sliceOffset, size_t sliceSize)
    {
      return Buffer(static_cast<uint8_t* const>(data) + sliceOffset, sliceSize);
    }

    void* data;
    size_t size;
  };

  // Memory allocating with new uint8_t[].
  class Memory
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
#if defined(__cpp_lib_source_location)
    Exception(std::source_location location = std::source_location::current());
#else
    Exception() = default;
#endif

    template <typename T>
    explicit Exception(T const& value
#if defined(__cpp_lib_source_location)
      , std::source_location location = std::source_location::current()
#endif
    )
    {
      std::move(*this)
#if defined(__cpp_lib_source_location)
        << location.file_name() << ':' << location.line() << ' ' << location.function_name() << ": "
#endif
        << value;
    }

    Exception(Exception const&) = delete;
    Exception(Exception&&) = default;

    std::string GetMessage() const;

    friend Exception&& operator<<(Exception&& e, Exception const& inner);

    template <typename T>
    friend Exception&& operator<<(Exception&& e, T const& value)
    {
      e._message << value;
      return std::move(e);
    }

  private:
    std::ostringstream _message;
  };


  // Input stream.
  class InputStream
  {
  public:
    // Read some data, up to size of the buffer.
    // Always fills the buffer fully, unless there's not enough data.
    virtual size_t Read(Buffer const& buffer) = 0;
  };

  // Output stream.
  class OutputStream
  {
  public:
    virtual void Write(Buffer const& buffer) = 0;
    virtual void End() {};

    void WriteAllFrom(InputStream& inputStream);
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

  // Output stream writing into buffer.
  class BufferOutputStream : public OutputStream
  {
  public:
    BufferOutputStream(Buffer const& buffer);

    void Write(Buffer const& buffer) override;

  private:
    Buffer _buffer;
  };

  class MemoryStream : public OutputStream
  {
  public:
    void Write(Buffer const& buffer) override;

    Buffer ToBuffer() const;

  private:
    std::vector<uint8_t> _data;
  };
}
