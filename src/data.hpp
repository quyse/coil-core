#pragma once

#include "base.hpp"
#include <string>

namespace Coil
{
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

    // Write number in shortened format.
    void WriteNumber(uint64_t value);
    // Write string.
    void WriteString(std::string const& value);

    // Write gap - align to specified boundary.
    template <size_t alignment>
    void WriteGap()
    {
      static_assert(!(alignment & (alignment - 1)), "alignment must be power of two");
      _WriteGap(alignment);
    }

    bigsize_t GetWrittenSize() const;

  private:
    void _WriteGap(size_t alignment);

    OutputStream& _stream;
    bigsize_t _written = 0;
  };

  class StreamReader
  {
  public:
    StreamReader(InputStream& stream);

    void Read(void* data, size_t size);

    // Read primitive data.
    template <typename T>
    T Read()
    {
      T value;
      Read(&value, sizeof(value));
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

    bigsize_t GetReadSize() const;

  private:
    void _ReadGap(size_t alignment);

    InputStream& _stream;
    bigsize_t _read = 0;
  };
}
