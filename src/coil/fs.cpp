#include "fs.hpp"
#if defined(___COIL_PLATFORM_WINDOWS)
#include "unicode.hpp"
#include "windows.hpp"
#elif defined(___COIL_PLATFORM_POSIX)
#include <limits>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#else
#error unsupported system
#endif

namespace
{
  class FileMapping
  {
  public:
    FileMapping(void* pMapping, size_t size)
    : _pMapping(pMapping), _size(size) {}

    ~FileMapping()
    {
#if defined(___COIL_PLATFORM_WINDOWS)
      ::UnmapViewOfFile(_pMapping);
#elif defined(___COIL_PLATFORM_POSIX)
      ::munmap(_pMapping, _size);
#endif
    }

  private:
    void* _pMapping;
    size_t _size;
  };
}

namespace Coil
{
#if defined(___COIL_PLATFORM_WINDOWS)
  File::File(void* hFile)
  : _hFile(hFile) {}
#elif defined(___COIL_PLATFORM_POSIX)
  File::File(int fd)
  : _fd(fd) {}
#endif

  File::~File()
  {
#if defined(___COIL_PLATFORM_WINDOWS)
    if(_hFile)
      ::CloseHandle(_hFile);
#elif defined(___COIL_PLATFORM_POSIX)
    if(_fd >= 0)
      ::close(_fd);
#endif
  }

  size_t File::Read(uint64_t offset, Buffer const& buffer)
  {
#if defined(___COIL_PLATFORM_WINDOWS)
    if(!::SetFilePointerEx(_hFile, { .QuadPart = (LONGLONG)offset }, NULL, FILE_BEGIN))
      throw Exception("setting file pointer failed");
    DWORD bytesRead;
    if(!::ReadFile(_hFile, buffer.data, buffer.size, &bytesRead, NULL))
      throw Exception("reading file failed");
    return bytesRead;
#elif defined(___COIL_PLATFORM_POSIX)
    if(::lseek64(_fd, offset, SEEK_SET) == -1)
      throw Exception("seeking in file failed");
    uint8_t* data = (uint8_t*)buffer.data;
    size_t size = buffer.size;
    size_t totalReadSize = 0;
    while(size > 0)
    {
      size_t readSize = std::min<size_t>(size, std::numeric_limits<ssize_t>::max());
      readSize = ::read(_fd, data, readSize);
      if(readSize < 0)
        throw Exception("reading file failed");
      if(readSize == 0)
        break;
      totalReadSize += readSize;
      size -= readSize;
      data += readSize;
    }
    return totalReadSize;
#endif
  }

  uint64_t File::GetSize() const
  {
#if defined(___COIL_PLATFORM_WINDOWS)
    LARGE_INTEGER size;
    if(!::GetFileSizeEx(_hFile, &size))
      throw Exception("getting file size failed");
    return size.QuadPart;
#elif defined(___COIL_PLATFORM_POSIX)
    struct stat st;
    if(::fstat(_fd, &st) != 0)
      throw Exception("getting file size failed");
    return st.st_size;
#endif
  }

  File& File::Open(Book& book, std::string const& name)
  {
#if defined(___COIL_PLATFORM_WINDOWS)
    std::vector<wchar_t> s;
    Unicode::Convert<char, char16_t>(name.c_str(), s);
    HANDLE hFile = ::CreateFileW(s.data(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, NULL);
    if(hFile == INVALID_HANDLE_VALUE)
      throw Exception("opening file failed: ") << name;
    return book.Allocate<File>(hFile);
#elif defined(___COIL_PLATFORM_POSIX)
    int fd = ::open(name.c_str(), O_RDONLY, 0);
    if(fd < 0)
      throw Exception("opening file failed: ") << name;
    return book.Allocate<File>(fd);
#endif
  }

  Buffer File::Map(Book& book, std::string const& name)
  {
    try
    {
#if defined(___COIL_PLATFORM_WINDOWS)
      // open file
      std::vector<wchar_t> s;
      Unicode::Convert<char, char16_t>(name.c_str(), s);
      HANDLE hFile = ::CreateFileW(s.data(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, NULL);
      if(hFile == INVALID_HANDLE_VALUE)
        throw Exception("opening file failed");
      File file = hFile;

      // create mapping
      uint64_t size = file.GetSize();
      if((size_t)size != size)
        throw Exception("too big file mapping");
      HANDLE hMapping = ::CreateFileMappingW(hFile, NULL, PAGE_READONLY, 0, 0, 0);
      if(!hMapping)
        throw Exception("creating file mapping failed");

      // map file
      void* pMapping = ::MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
      ::CloseHandle(hMapping);
      if(!pMapping)
        throw Exception("mapping view failed");

      book.Allocate<FileMapping>(pMapping, size);

      return Buffer(pMapping, size);
#elif defined(___COIL_PLATFORM_POSIX)
      int fd = ::open(name.c_str(), O_RDONLY, 0);
      if(fd < 0)
        throw Exception("opening file failed");
      File file = fd;

      uint64_t size = file.GetSize();
      if((size_t)size != size)
        throw Exception("too big file mapping");

      void* pMapping = ::mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
      if(pMapping == MAP_FAILED)
        throw Exception("mmap failed");

      book.Allocate<FileMapping>(pMapping, size);

      return Buffer(pMapping, size);
#endif
    }
    catch(Exception const& exception)
    {
      throw Exception("mapping file failed: ") << name << exception;
    }
  }

  FileInputStream::FileInputStream(File& file, uint64_t offset)
  : _file(file), _offset(offset), _size(file.GetSize()) {}
  FileInputStream::FileInputStream(File& file, uint64_t offset, uint64_t size)
  : _file(file), _offset(offset), _size(size) {}

  size_t FileInputStream::Read(Buffer const& buffer)
  {
    size_t read = _file.Read(_offset, buffer);
    _offset += read;
    return read;
  }

  FileInputStream& FileInputStream::Open(Book& book, std::string const& name)
  {
    return book.Allocate<FileInputStream>(File::Open(book, name));
  }

  std::string GetFilePathName(std::string const& path)
  {
    size_t i;
    for(i = path.length(); i > 0 && path[i - 1] != FilePathSeparator; --i);
    return path.substr(i);
  }

  std::string GetFilePathDirectory(std::string const& path)
  {
    size_t i;
    for(i = path.length() - 1; i > 0 && path[i] != FilePathSeparator; --i);
    return path.substr(0, i);
  }

}
