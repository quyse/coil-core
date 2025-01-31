module;

#include <algorithm>
#include <concepts>
#include <sstream>
#include <string_view>
#include <vector>
#include <array>
#include <version>
// do not store source locations in release builds
// dev source paths end up in executables and bloat Nix closure
#if defined(__cpp_lib_source_location) && !defined(NDEBUG)
#include <source_location>
#endif
#include <cstddef>
#include <cstdint>
#include "base.hpp"

export module coil.core.base;

export namespace Coil
{
  // Book is a container for objects to remove them later.
  class Book
  {
  public:
    Book() = default;
    Book(Book const&) = delete;
    Book(Book&& other) noexcept
    {
      *this = std::move(other);
    }
    ~Book()
    {
      Free();
    }

    Book& operator=(Book const&) = delete;
    Book& operator=(Book&& other) noexcept
    {
      std::swap(_lastChunk, other._lastChunk);
      std::swap(_lastChunkAllocated, other._lastChunkAllocated);
      std::swap(_lastObjectHeader, other._lastObjectHeader);
      return *this;
    }

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
    void Free()
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
      virtual ~ObjectHeader() = default;

      ObjectHeader* prev;
    };

    struct _Chunk {};

    static constexpr size_t _MaxAlignment = 16;

    template <typename T>
    struct alignas(_MaxAlignment) TemplObjectHeader final : public ObjectHeader
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
    struct TemplObjectHeader<_Chunk> final : public ObjectHeader
    {
      TemplObjectHeader(ObjectHeader* prev)
      : ObjectHeader(prev) {}
      ~TemplObjectHeader()
      {
        delete [] reinterpret_cast<uint8_t*>(this);
      }
    };

    void* _AllocateFromPool(size_t size)
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

    void _Register(ObjectHeader* objectHeader);

    uint8_t* _lastChunk = nullptr;
    size_t _lastChunkAllocated = 0;
    ObjectHeader* _lastObjectHeader = nullptr;

    static constexpr size_t _ChunkSize = 0x1000 - 128; // try to (over)estimate heap's overhead
    static constexpr size_t _MaxObjectSize = _ChunkSize - sizeof(ObjectHeader);
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
    explicit Buffer(size_t size)
    : data(nullptr), size(size) {}

    template <typename T>
    Buffer(std::vector<T>& v)
    : data(v.data()), size(v.size() * sizeof(T)) {}
    template <typename T>
    Buffer(std::vector<T> const& v)
    : data(const_cast<void*>(static_cast<void const*>(v.data()))), size(v.size() * sizeof(T)) {}

    Buffer(std::string& s)
    : data(s.data()), size(s.length()) {}
    Buffer(std::string const& s)
    : data(const_cast<void*>(static_cast<void const*>(s.data()))), size(s.length()) {}

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
    Memory(uint8_t* const data)
    : _data(data) {}
    ~Memory()
    {
      delete [] _data;
    }

    static Buffer Allocate(Book& book, size_t size)
    {
      uint8_t* data = new uint8_t[size];
      book.Allocate<Memory>(data);
      return { data, size };
    }

  private:
    uint8_t* const _data;
  };

  class Exception
  {
  public:
#if defined(__cpp_lib_source_location) && !defined(NDEBUG)
    Exception(std::source_location location = std::source_location::current())
    {
      *this << location.file_name() << ':' << location.line() << ' ' << location.function_name() << ": ";
    }
#else
    Exception() = default;
#endif

    template <typename T>
    explicit Exception(T&& value
#if defined(__cpp_lib_source_location) && !defined(NDEBUG)
      , std::source_location location = std::source_location::current()
#endif
    )
    {
      *this
#if defined(__cpp_lib_source_location) && !defined(NDEBUG)
        << location.file_name() << ':' << location.line() << ' ' << location.function_name() << ": "
#endif
        << std::forward<T>(value);
    }

    Exception(Exception const&) = delete;
    Exception(Exception&&) = default;

    std::string GetMessage() const
    {
      return _message.str();
    }

    friend Exception& operator<<(Exception& e, Exception const& inner)
    {
      e._message << '\n' << inner._message.str();
      return e;
    }
    friend Exception operator<<(Exception&& e, Exception const& inner)
    {
      e._message << '\n' << inner._message.str();
      return std::move(e);
    }

    template <typename T>
    friend Exception& operator<<(Exception& e, T const& value)
    {
      e._message << value;
      return e;
    }
    template <typename T>
    friend Exception operator<<(Exception&& e, T const& value)
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
    // Skip some data.
    // Always skips requested amount, unless there's not enough data.
    // Default implementation just reads the data.
    virtual size_t Skip(size_t size)
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
  };

  // Source of input streams.
  // Allows to create streams multiple times.
  class InputStreamSource
  {
  public:
    virtual InputStream& CreateStream(Book& book) = 0;
  };

  // Output stream.
  class OutputStream
  {
  public:
    virtual void Write(Buffer const& buffer) = 0;
    virtual void End() {};

    void WriteAllFrom(InputStream& inputStream)
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
  };

  // Packetized input stream.
  class PacketInputStream
  {
  public:
    // read one packet from stream, empty buffer if EOF
    // buffer is valid only until next read
    virtual Buffer ReadPacket() = 0;
  };

  // Source of packetized input stream.
  // Allows creating stream multiple times.
  class PacketInputStreamSource
  {
  public:
    virtual PacketInputStream& CreateStream(Book& book) = 0;
  };

  class ReadableStorage
  {
  public:
    virtual uint64_t GetSize() const = 0;
    virtual size_t Read(uint64_t offset, Buffer const& buffer) const = 0;
  };

  class WritableStorage
  {
  public:
    virtual void Write(uint64_t offset, Buffer const& buffer) = 0;
  };


  // text literal
  // can be used as non-type template argument
  template <size_t N>
  struct Literal
  {
    template <size_t M>
    constexpr Literal(char const(&s)[M]) requires (N <= M)
    {
      std::copy_n(s, N, this->s);
    }
    constexpr Literal(char const* s)
    {
      std::copy_n(s, N, this->s);
    }

    friend std::ostream& operator<<(std::ostream& s, Literal const& l)
    {
      return s << std::string_view(l.s, l.n);
    }

    char s[N];
    static constexpr size_t n = N;
  };
  template <size_t M>
  Literal(char const(&s)[M]) -> Literal<M - 1>;

  // custom literal
  template <Literal l>
  consteval auto operator""_l()
  {
    return l;
  }

  // compile-time multicharacter literal
  // (simple 'abcd' has implementation-defined value and produces compile warnings)
  template <Literal l>
  consteval auto operator""_c()
  {
    uint32_t r = 0;
    for(size_t i = 0; i < l.n; ++i)
      r = (r << 8) | l.s[i];
    return r;
  }
  static_assert("ABCD"_c == 0x41424344);

  // compile-time hex byte array literal
  template <Literal l>
  consteval std::array<uint8_t, l.n / 2> operator""_hex()
  {
    static_assert(l.n % 2 == 0);
    std::array<uint8_t, l.n / 2> r = {};
    for(size_t i = 0; i < l.n; ++i)
    {
      uint8_t c;
      if(l.s[i] >= '0' && l.s[i] <= '9') c = l.s[i] - '0';
      else if(l.s[i] >= 'a' && l.s[i] <= 'f') c = l.s[i] - 'a' + 10;
      else if(l.s[i] >= 'A' && l.s[i] <= 'F') c = l.s[i] - 'A' + 10;
      else throw "literal is not hex";
      r[i / 2] |= (i % 2) ? c : (c << 4);
    }
    return r;
  }


  // serialization to/from string, to be specialized
  template <typename T>
  std::string ToString(T const& value);
  template <typename T>
  T FromString(std::string_view str);

  // asset traits
  // specialization must be created for every asset type
  template <typename Asset>
  struct AssetTraits
  {
    // default implementation is empty, so arbitrary types are not assets

    /* what should be defined for asset types:

    // asset type name
    static constexpr std::string_view assetTypeName = "";

    */
  };

  // whether type is asset
  template <typename Asset>
  concept IsAsset = std::copyable<Asset> && requires
  {
    { AssetTraits<Asset>::assetTypeName } -> std::convertible_to<std::string_view>;
  };

  // whether type is asset loader
  template <typename AssetLoader>
  concept IsAssetLoader = requires
  {
    { AssetLoader::assetLoaderName } -> std::convertible_to<std::string_view>;
  };

  template <>
  struct AssetTraits<Buffer>
  {
    static constexpr std::string_view assetTypeName = "buffer";
  };
  static_assert(IsAsset<Buffer>);

  template <>
  struct AssetTraits<InputStreamSource*>
  {
    static constexpr std::string_view assetTypeName = "input_stream_source";
  };
  static_assert(IsAsset<InputStreamSource*>);

  template <>
  struct AssetTraits<PacketInputStreamSource*>
  {
    static constexpr std::string_view assetTypeName = "packet_input_stream_source";
  };
  static_assert(IsAsset<PacketInputStreamSource*>);
}
