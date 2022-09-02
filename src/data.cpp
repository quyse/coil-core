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
    // MSB of first byte determine total length, from 0 - 1 byte, to 11111111 - 9 bytes
    // the rest of the bits of the first byte, and the rest of the bytes
    // contain the number, in big-endian
    uint8_t length;
    uint8_t bytes[9];
    if(value < 0x80)
    {
      length = 0;
      bytes[0] = 0x00;
    }
    else if(value < 0x4000)
    {
      length = 1;
      bytes[0] = 0x80;
    }
    else if(value < 0x200000)
    {
      length = 2;
      bytes[0] = 0xC0;
    }
    else if(value < 0x10000000)
    {
      length = 3;
      bytes[0] = 0xE0;
    }
    else if(value < 0x800000000ULL)
    {
      length = 4;
      bytes[0] = 0xF0;
    }
    else if(value < 0x40000000000ULL)
    {
      length = 5;
      bytes[0] = 0xF8;
    }
    else if(value < 0x2000000000000ULL)
    {
      length = 6;
      bytes[0] = 0xFC;
    }
    else if(value < 0x1000000000000ULL)
    {
      length = 7;
      bytes[0] = 0xFE;
    }
    else
    {
      length = 8;
      bytes[0] = 0xFF;
    }

    // prepare first byte
    bytes[0] |= (uint8_t)(value >> (length * 8));
    // prepare additional bytes
    for(uint8_t i = 0; i < length; ++i)
      bytes[1 + i] = (uint8_t)(value >> ((length - 1 - i) * 8));

    // write
    Write(bytes, length + 1);
  }

  void StreamWriter::WriteString(std::string const& value)
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

  uint64_t StreamReader::ReadNumber()
  {
    uint8_t first = Read<uint8_t>();
    uint8_t length;
    if(!(first & 0x80)) return first;
    else if(!(first & 0x40))
    {
      length = 1;
      first &= ~0x80;
    }
    else if(!(first & 0x20))
    {
      length = 2;
      first &= ~0xC0;
    }
    else if(!(first & 0x10))
    {
      length = 3;
      first &= ~0xE0;
    }
    else if(!(first & 0x08))
    {
      length = 4;
      first &= ~0xF0;
    }
    else if(!(first & 0x04))
    {
      length = 5;
      first &= ~0xF8;
    }
    else if(!(first & 0x02))
    {
      length = 6;
      first &= ~0xFC;
    }
    else if(!(first & 0x01))
    {
      length = 7;
      first &= ~0xFE;
    }
    else
    {
      length = 8;
      first &= ~0xFF;
    }

    // read additional bytes
    uint8_t bytes[8];
    Read(bytes, length);

    // calculate value
    uint64_t value = ((uint64_t)first) << (length * 8);
    for(uint8_t i = 0; i < length; ++i)
      value |= ((uint64_t)bytes[i]) << ((length - 1 - i) * 8);

    return value;
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
    _stream.write((char const*)buffer.data, buffer.size);
    if(_stream.fail()) throw Exception("failed to write to std stream");
  }

  StdStreamInputStream::StdStreamInputStream(std::istream& stream)
  : _stream(stream) {}

  size_t StdStreamInputStream::Read(Buffer const& buffer)
  {
    _stream.read((char*)buffer.data, buffer.size);
    if(_stream.fail()) throw Exception("failed to read from std stream");
    return _stream.gcount();
  }
}
