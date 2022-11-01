#pragma once

#include "base.hpp"
#include <string>

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

    static File Open(std::string const& name);
    static Buffer Map(Book& book, std::string const& name);

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

  constexpr char FilePathSeparator =
#if defined(___COIL_PLATFORM_WINDOWS)
    '\\'
#else
    '/'
#endif
  ;

  // Get file name from file path.
  std::string GetFilePathName(std::string const& path);
  // Get directory from file path.
  std::string GetFilePathDirectory(std::string const& path);
}