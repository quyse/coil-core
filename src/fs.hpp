#pragma once

#include "base.hpp"

namespace Coil
{
  class File
  {
  public:
#if defined(___COIL_PLATFORM_WINDOWS)
    File(void* hFile);
#elif defined(___COIL_PLATFORM_POSIX)
    File(int fd);
#endif
    ~File();

    File(File const&) = delete;
    File(File&&);

    size_t Read(uint64_t offset, Buffer& buffer);
    uint64_t GetSize() const;

    static File Open(char const* name);
    static Buffer Map(Book& book, char const* name);

  private:
#if defined(___COIL_PLATFORM_WINDOWS)
    // not using Windows HANDLE to not include windows.h
    void* _hFile = nullptr;
#elif defined(___COIL_PLATFORM_POSIX)
    int _fd = -1;
#endif
  };

  // Input stream which reads part of a file.
  class FileInputStream
  {
  public:
    FileInputStream(File& file, uint64_t offset, uint64_t size)
    : _file(file), _offset(offset), _size(size) {}

  private:
    File& _file;
    uint64_t _offset;
    uint64_t _size;
  };
}
