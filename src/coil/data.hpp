#pragma once

#include "base.hpp"
#include <string>
#include <string_view>
#include <istream>
#include <ostream>
#include <bit>

namespace Coil
{
  static_assert((std::endian::native == std::endian::big) != (std::endian::native == std::endian::little));

  void EndianSwap(uint32_t& value);

  template <typename T>
  concept EndianSwappable = requires(T value)
  {
    EndianSwap(value);
  };

  // Input stream reading from buffer.
  class BufferInputStream final : public InputStream
  {
  public:
    BufferInputStream(Buffer const& buffer);

    size_t Read(Buffer const& buffer) override;
    size_t Skip(size_t size) override;

    Buffer const& GetBuffer() const;

  private:
    Buffer _buffer;
  };

  class BufferInputStreamSource final : public InputStreamSource
  {
  public:
    BufferInputStreamSource(Buffer const& buffer);

    BufferInputStream& CreateStream(Book& book) override;

  private:
    Buffer const _buffer;
  };

  // Output stream writing into buffer.
  class BufferOutputStream final : public OutputStream
  {
  public:
    BufferOutputStream(Buffer const& buffer);

    void Write(Buffer const& buffer) override;

  private:
    Buffer _buffer;
  };

  class MemoryStream final : public OutputStream
  {
  public:
    void Write(Buffer const& buffer) override;

    Buffer ToBuffer() const;

  private:
    std::vector<uint8_t> _data;
  };

  class BufferStorage final : public ReadableStorage, public WritableStorage
  {
  public:
    BufferStorage(Buffer const& buffer);

    uint64_t GetSize() const override;
    size_t Read(uint64_t offset, Buffer const& buffer) const override;
    void Write(uint64_t offset, Buffer const& buffer) override;

  private:
    Buffer const _buffer;
  };

  class ReadableStorageStream : public InputStream
  {
  public:
    ReadableStorageStream(ReadableStorage const& storage, uint64_t offset, uint64_t size);

    size_t Read(Buffer const& buffer) override;
    size_t Skip(size_t size) override;

  private:
    ReadableStorage const& _storage;
    uint64_t _offset, _size;
  };

  // Writer of various data types.
  class StreamWriter
  {
  public:
    StreamWriter(OutputStream& stream);

    // Write any data.
    void Write(void const* data, size_t size);

    // Write primitive data.
    template <typename T>
    void Write(T const& value)
    {
      Write(&value, sizeof(value));
    }

    // Write number in little-endian.
    template <EndianSwappable T>
    void WriteLE(T value)
    {
      if constexpr(std::endian::native == std::endian::big) EndianSwap(value);
      Write<T>(value);
    }
    // Write number in big-endian.
    template <EndianSwappable T>
    void WriteBE(T value)
    {
      if constexpr(std::endian::native == std::endian::little) EndianSwap(value);
      Write<T>(value);
    }

    // Write number in shortened format.
    void WriteNumber(uint64_t value);
    // Write string.
    void WriteString(std::string_view value);

    // Write gap - align to specified boundary.
    template <size_t alignment>
    void WriteGap()
    {
      static_assert(!(alignment & (alignment - 1)), "alignment must be power of two");
      _WriteGap(alignment);
    }

    uint64_t GetWrittenSize() const;

  private:
    void _WriteGap(size_t alignment);

    OutputStream& _stream;
    uint64_t _written = 0;
  };

  class StreamReader
  {
  public:
    StreamReader(InputStream& stream);

    void Read(void* data, size_t size);
    void Skip(size_t size);

    // Read primitive data.
    template <typename T>
    T Read()
    {
      T value;
      Read(&value, sizeof(value));
      return value;
    }

    // Read number in little-endian.
    template <EndianSwappable T>
    T ReadLE()
    {
      T value = Read<T>();
      if constexpr(std::endian::native == std::endian::big) EndianSwap(value);
      return value;
    }
    // Read number in big-endian.
    template <EndianSwappable T>
    T ReadBE()
    {
      T value = Read<T>();
      if constexpr(std::endian::native == std::endian::little) EndianSwap(value);
      return value;
    }

    // Read number in shortened format.
    uint64_t ReadNumber();
    // Read string.
    std::string ReadString();

    // Read gap - align to specified boundary.
    template <size_t alignment>
    void ReadGap()
    {
      static_assert(!(alignment & (alignment - 1)), "alignment must be power of two");
      _ReadGap(alignment);
    }

    // Ensure stream has ended.
    void ReadEnd();

    uint64_t GetReadSize() const;

  private:
    void _ReadGap(size_t alignment);

    InputStream& _stream;
    uint64_t _read = 0;
  };

  class StdStreamOutputStream : public OutputStream
  {
  public:
    StdStreamOutputStream(std::ostream& stream);

    void Write(Buffer const& buffer) override;

  private:
    std::ostream& _stream;
  };

  class StdStreamInputStream : public InputStream
  {
  public:
    StdStreamInputStream(std::istream& stream);

    size_t Read(Buffer const& buffer) override;

  private:
    std::istream& _stream;
  };

  // circular byte queue
  class CircularMemoryBuffer
  {
  public:
    // actual data size
    size_t GetDataSize() const;
    // buffer size
    size_t GetBufferSize() const;
    // consume data from circular buffer
    // returns size actually read
    size_t Read(Buffer const& buffer);
    // push data into circular buffer
    // auto-expands if necessary
    void Write(Buffer const& buffer);

  private:
    std::vector<uint8_t> _buffer;
    size_t _start = 0;
    size_t _size = 0;
  };

  // input stream which reads not more than specified number of bytes
  // from source stream
  class LimitedInputStream final : public InputStream
  {
  public:
    LimitedInputStream(InputStream& inputStream, uint64_t limit);

    size_t Read(Buffer const& buffer) override;
    size_t Skip(size_t size) override;

  private:
    InputStream& _inputStream;
    uint64_t _remaining;
  };
}
