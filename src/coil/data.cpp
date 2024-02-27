#include "data.hpp"
#include <algorithm>

namespace Coil
{
  void EndianSwap(uint32_t& value)
  {
    value = ((value & 0xFF) << 24) | ((value & 0xFF00) << 8) | ((value & 0xFF0000) >> 8) | ((value & 0xFF000000) >> 24);
  }


  StreamWriter::StreamWriter(OutputStream& stream)
  : _stream(stream) {}

  void StreamWriter::Write(void const* data, size_t size)
  {
    if(size)
    {
      _stream.Write(Buffer(data, size));
      _written += size;
    }
  }

  void StreamWriter::WriteNumber(uint64_t value)
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

  void StreamWriter::WriteString(std::string_view value)
  {
    WriteNumber(value.length());
    Write(value.data(), value.length());
  }

  uint64_t StreamWriter::GetWrittenSize() const
  {
    return _written;
  }

  void StreamWriter::_WriteGap(size_t alignment)
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


  StreamReader::StreamReader(InputStream& stream)
  : _stream(stream) {}

  void StreamReader::Read(void* data, size_t size)
  {
    size_t read = _stream.Read(Buffer(data, size));
    if(read != size)
      throw Exception("StreamReader: unexpected end of stream");
    _read += size;
  }

  void StreamReader::Skip(size_t size)
  {
    size_t skipped = _stream.Skip(size);
    if(skipped != size)
      throw Exception("StreamReader: unexpected end of stream");
    _read += size;
  }

  uint64_t StreamReader::ReadNumber()
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

  std::string StreamReader::ReadString()
  {
    uint64_t length = ReadNumber();
    std::string value(length, '\0');
    Read(value.data(), length);
    return std::move(value);
  }

  void StreamReader::ReadEnd()
  {
    uint8_t u;
    if(_stream.Read(Buffer(&u, 1)) != 0)
      throw Exception("StreamReader: no end of stream");
  }

  uint64_t StreamReader::GetReadSize() const
  {
    return _read;
  }

  void StreamReader::_ReadGap(size_t alignment)
  {
    alignment = alignment - ((_read - 1) & (alignment - 1)) - 1;
    if(alignment)
    {
      uint8_t* data = (uint8_t*)alloca(alignment);
      Read(data, alignment);
    }
  }


  StdStreamOutputStream::StdStreamOutputStream(std::ostream& stream)
  : _stream(stream) {}

  void StdStreamOutputStream::Write(Buffer const& buffer)
  {
    _stream.write((char const*)buffer.data, (std::streamsize)buffer.size);
    if(_stream.fail()) throw Exception("failed to write to std stream");
  }


  StdStreamInputStream::StdStreamInputStream(std::istream& stream)
  : _stream(stream) {}

  size_t StdStreamInputStream::Read(Buffer const& buffer)
  {
    _stream.read((char*)buffer.data, (std::streamsize)buffer.size);
    if(_stream.fail()) throw Exception("failed to read from std stream");
    return _stream.gcount();
  }


  size_t CircularMemoryBuffer::GetDataSize() const
  {
    return _size;
  }

  size_t CircularMemoryBuffer::GetBufferSize() const
  {
    return _buffer.size();
  }

  size_t CircularMemoryBuffer::Read(Buffer const& buffer)
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

  void CircularMemoryBuffer::Write(Buffer const& buffer)
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
}
