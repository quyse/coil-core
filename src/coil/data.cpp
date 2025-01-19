module;

#include <string>
#include <string_view>
#include <algorithm>
#include <istream>
#include <ostream>
#include <bit>
#include <cstring>

export module coil.core.data;

import coil.core.base;

export namespace Coil
{
  static_assert((std::endian::native == std::endian::big) != (std::endian::native == std::endian::little));

  void EndianSwap(uint32_t& value)
  {
    value = ((value & 0xFF) << 24) | ((value & 0xFF00) << 8) | ((value & 0xFF0000) >> 8) | ((value & 0xFF000000) >> 24);
  }

  template <typename T>
  concept EndianSwappable = requires(T value)
  {
    EndianSwap(value);
  };

  // Input stream reading from buffer.
  class BufferInputStream final : public InputStream
  {
  public:
    BufferInputStream(Buffer const& buffer)
    : _buffer(buffer) {}

    size_t Read(Buffer const& buffer) override
    {
      size_t size = std::min(buffer.size, _buffer.size);
      memcpy(buffer.data, _buffer.data, size);
      _buffer.data = (uint8_t*)_buffer.data + size;
      _buffer.size -= size;
      return size;
    }

    size_t Skip(size_t size) override
    {
      size = std::min(size, _buffer.size);
      _buffer.data = (uint8_t*)_buffer.data + size;
      _buffer.size -= size;
      return size;
    }

    Buffer const& GetBuffer() const
    {
      return _buffer;
    }

  private:
    Buffer _buffer;
  };

  class BufferInputStreamSource final : public InputStreamSource
  {
  public:
    BufferInputStreamSource(Buffer const& buffer)
    : _buffer(buffer) {}

    BufferInputStream& CreateStream(Book& book) override
    {
      return book.Allocate<BufferInputStream>(_buffer);
    }

  private:
    Buffer const _buffer;
  };

  // Output stream writing into buffer.
  class BufferOutputStream final : public OutputStream
  {
  public:
    BufferOutputStream(Buffer const& buffer)
    : _buffer(buffer) {}

    void Write(Buffer const& buffer) override
    {
      if(_buffer.size < buffer.size)
        throw Exception("BufferOutputStream: end of dest buffer");
      memcpy(_buffer.data, buffer.data, buffer.size);
      _buffer.data = (uint8_t*)_buffer.data + buffer.size;
      _buffer.size -= buffer.size;
    }

  private:
    Buffer _buffer;
  };

  class MemoryStream final : public OutputStream
  {
  public:
    void Write(Buffer const& buffer) override
    {
      size_t initialSize = _data.size();
      _data.resize(initialSize + buffer.size);
      memcpy(_data.data() + initialSize, buffer.data, buffer.size);
    }

    Buffer ToBuffer() const
    {
      return _data;
    }

  private:
    std::vector<uint8_t> _data;
  };

  class BufferStorage final : public ReadableStorage, public WritableStorage
  {
  public:
    BufferStorage(Buffer const& buffer)
    : _buffer(buffer) {}

    uint64_t GetSize() const override
    {
      return _buffer.size;
    }

    size_t Read(uint64_t offset, Buffer const& buffer) const override
    {
      if(_buffer.size < offset) return 0;
      size_t toRead = std::min(buffer.size, (size_t)(_buffer.size - offset));
      std::copy_n((uint8_t const*)_buffer.data + offset, toRead, (uint8_t*)buffer.data);
      return toRead;
    }

    void Write(uint64_t offset, Buffer const& buffer) override
    {
      if(offset + buffer.size > _buffer.size)
        throw Exception("buffer storage overflow while writing");
      std::copy_n((uint8_t const*)buffer.data, buffer.size, (uint8_t*)_buffer.data + offset);
    }

  private:
    Buffer const _buffer;
  };

  class ReadableStorageStream : public InputStream
  {
  public:
    ReadableStorageStream(ReadableStorage const& storage, uint64_t offset, uint64_t size)
    : _storage(storage), _offset(offset), _size(size) {}

    size_t Read(Buffer const& buffer) override
    {
      size_t toRead = (size_t)std::min<uint64_t>(_size, buffer.size);
      toRead = _storage.Read(_offset, Buffer{buffer.data, toRead});
      _offset += toRead;
      _size -= toRead;
      return toRead;
    }

    size_t Skip(size_t size) override
    {
      size_t toSkip = (size_t)std::min<uint64_t>(_size, size);
      _offset += toSkip;
      _size -= toSkip;
      return toSkip;
    }

  private:
    ReadableStorage const& _storage;
    uint64_t _offset, _size;
  };

  // Writer of various data types.
  class StreamWriter
  {
  public:
    StreamWriter(OutputStream& stream)
    : _stream(stream) {}

    // Write any data.
    void Write(void const* data, size_t size)
    {
      if(size)
      {
        _stream.Write(Buffer(data, size));
        _written += size;
      }
    }

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
    void WriteNumber(uint64_t value)
    {
      // each byte contains 7 value bits, starting from least significant
      // most significant bit in each byte is 1, except the last byte
      uint8_t bytes[9];
      uint8_t i = 0;
      do
      {
        bytes[i] = value & 0x7F;
        value >>= 7;
        if(value) bytes[i] |= 0x80;
        ++i;
      }
      while(value);
      Write(bytes, i);
    }

    // Write string.
    void WriteString(std::string_view value)
    {
      WriteNumber(value.length());
      Write(value.data(), value.length());
    }

    // Write gap - align to specified boundary.
    template <size_t alignment>
    void WriteGap()
    {
      static_assert(!(alignment & (alignment - 1)), "alignment must be power of two");
      _WriteGap(alignment);
    }

    uint64_t GetWrittenSize() const
    {
      return _written;
    }

  private:
    void _WriteGap(size_t alignment)
    {
      alignment = alignment - ((_written - 1) & (alignment - 1)) - 1;
      if(alignment)
      {
        uint8_t* data = (uint8_t*)alloca(alignment);
        for(size_t i = 0; i < alignment; ++i)
          data[i] = 0xCC;
        Write(data, alignment);
      }
    }

    OutputStream& _stream;
    uint64_t _written = 0;
  };

  class StreamReader
  {
  public:
    StreamReader(InputStream& stream)
    : _stream(stream) {}

    void Read(void* data, size_t size)
    {
      size_t read = _stream.Read(Buffer(data, size));
      if(read != size)
        throw Exception("StreamReader: unexpected end of stream");
      _read += size;
    }

    void Skip(size_t size)
    {
      size_t skipped = _stream.Skip(size);
      if(skipped != size)
        throw Exception("StreamReader: unexpected end of stream");
      _read += size;
    }

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
    uint64_t ReadNumber()
    {
      uint64_t value = 0;
      uint8_t i = 0;
      uint64_t byte;
      do
      {
        byte = Read<uint8_t>();
        value |= (byte & 0x7F) << i;
        i += 7;
      }
      while(byte & 0x80);
      return value;
    }

    // Read string.
    std::string ReadString()
    {
      uint64_t length = ReadNumber();
      std::string value(length, '\0');
      Read(value.data(), length);
      return std::move(value);
    }

    // Read gap - align to specified boundary.
    template <size_t alignment>
    void ReadGap()
    {
      static_assert(!(alignment & (alignment - 1)), "alignment must be power of two");
      _ReadGap(alignment);
    }

    // Ensure stream has ended.
    void ReadEnd()
    {
      uint8_t u;
      if(_stream.Read(Buffer(&u, 1)) != 0)
        throw Exception("StreamReader: no end of stream");
    }

    uint64_t GetReadSize() const
    {
      return _read;
    }

  private:
    void _ReadGap(size_t alignment)
    {
      alignment = alignment - ((_read - 1) & (alignment - 1)) - 1;
      if(alignment)
      {
        uint8_t* data = (uint8_t*)alloca(alignment);
        Read(data, alignment);
      }
    }

    InputStream& _stream;
    uint64_t _read = 0;
  };

  class StdStreamOutputStream : public OutputStream
  {
  public:
    StdStreamOutputStream(std::ostream& stream)
    : _stream(stream) {}

    void Write(Buffer const& buffer) override
    {
      _stream.write((char const*)buffer.data, (std::streamsize)buffer.size);
      if(_stream.fail()) throw Exception("failed to write to std stream");
    }

  private:
    std::ostream& _stream;
  };

  class StdStreamInputStream : public InputStream
  {
  public:
    StdStreamInputStream(std::istream& stream)
    : _stream(stream) {}

    size_t Read(Buffer const& buffer) override
    {
      _stream.read((char*)buffer.data, (std::streamsize)buffer.size);
      if(_stream.fail()) throw Exception("failed to read from std stream");
      return _stream.gcount();
    }

  private:
    std::istream& _stream;
  };

  // circular byte queue
  class CircularMemoryBuffer
  {
  public:
    // actual data size
    size_t GetDataSize() const
    {
      return _size;
    }

    // buffer size
    size_t GetBufferSize() const
    {
      return _buffer.size();
    }

    // consume data from circular buffer
    // returns size actually read
    size_t Read(Buffer const& buffer)
    {
      // total size to read
      size_t toRead = std::min(_size, buffer.size);
      // first part
      size_t toRead1 = std::min(toRead, _buffer.size() - _start);
      std::copy_n(_buffer.data() + _start, toRead1, (uint8_t*)buffer.data);
      _start += toRead1;
      if(_start >= _buffer.size()) _start -= _buffer.size();
      // second part
      size_t toRead2 = toRead - toRead1;
      std::copy_n(_buffer.data(), toRead2, (uint8_t*)buffer.data + toRead1);
      _start += toRead2;

      _size -= toRead;

      // return total size read
      return toRead;
    }

    // push data into circular buffer
    // auto-expands if necessary
    void Write(Buffer const& buffer)
    {
      // expand buffer if necessary
      {
        size_t oldBufferSize = _buffer.size();
        if(GetDataSize() + buffer.size > _buffer.size())
        {
          _buffer.resize(GetDataSize() + buffer.size);
          // if data was wrapped around, need to move it a bit
          size_t end = _start + _size;
          if(end > oldBufferSize)
          {
            end -= oldBufferSize;
            // first part of [0, end) range must be moved after second part
            size_t toMove = std::min(end, _buffer.size() - oldBufferSize);
            std::copy_n(_buffer.data(), toMove, _buffer.data() + oldBufferSize);
            // second part of [0, end) range must be moved back to the beginning
            std::copy_n(_buffer.data() + toMove, end - toMove, _buffer.data());
          }
        }
      }

      // now buffer is enough, write data

      size_t end = _start + _size;
      if(end >= _buffer.size()) end -= _buffer.size();
      // first part
      size_t toWrite1 = std::min(_buffer.size() - end, buffer.size); // no need to limit by _start, as we know there's enough space
      std::copy_n((uint8_t const*)buffer.data, toWrite1, _buffer.data() + end);
      end += toWrite1;
      if(end >= _buffer.size()) end = 0;
      // second part
      size_t toWrite2 = buffer.size - toWrite1;
      std::copy_n((uint8_t const*)buffer.data + toWrite1, buffer.size - toWrite1, _buffer.data());

      _size += buffer.size;
    }

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
    LimitedInputStream(InputStream& inputStream, uint64_t limit)
    : _inputStream(inputStream), _remaining(limit)
    {
    }

    size_t Read(Buffer const& buffer) override
    {
      size_t readSize = _inputStream.Read(Buffer(buffer.data, (size_t)std::min<uint64_t>(buffer.size, _remaining)));
      _remaining -= readSize;
      return readSize;
    }

    size_t Skip(size_t size) override
    {
      size_t skipSize = _inputStream.Skip((size_t)std::min<uint64_t>(size, _remaining));
      _remaining -= skipSize;
      return skipSize;
    }

  private:
    InputStream& _inputStream;
    uint64_t _remaining;
  };
}
