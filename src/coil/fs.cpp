#include "fs.hpp"
#include "unicode.hpp"
#if defined(COIL_PLATFORM_WINDOWS)
#include "windows.hpp"
#elif defined(COIL_PLATFORM_POSIX)
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
#if defined(COIL_PLATFORM_WINDOWS)
      ::UnmapViewOfFile(_pMapping);
#elif defined(COIL_PLATFORM_POSIX)
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
#if defined(COIL_PLATFORM_WINDOWS)
  File::File(void* hFile)
  : _hFile(hFile) {}
#elif defined(COIL_PLATFORM_POSIX)
  File::File(int fd)
  : _fd(fd) {}
#endif

  File::~File()
  {
#if defined(COIL_PLATFORM_WINDOWS)
    if(_hFile)
      ::CloseHandle(_hFile);
#elif defined(COIL_PLATFORM_POSIX)
    if(_fd >= 0)
      ::close(_fd);
#endif
  }

  size_t File::Read(uint64_t offset, Buffer const& buffer)
  {
#if defined(COIL_PLATFORM_WINDOWS)
    OVERLAPPED overlapped =
    {
      .Offset = (DWORD)(offset & 0xFFFFFFFF),
      .OffsetHigh = (DWORD)(offset >> 32),
    };
    DWORD bytesRead;
    if(!::ReadFile(_hFile, buffer.data, buffer.size, &bytesRead, &overlapped) && GetLastError() != ERROR_HANDLE_EOF)
      throw Exception("reading file failed");
    return bytesRead;
#elif defined(COIL_PLATFORM_POSIX)
    uint8_t* data = (uint8_t*)buffer.data;
    size_t size = buffer.size;
    size_t totalReadSize = 0;
    while(size > 0)
    {
      size_t const readSize = ::pread(_fd, data, std::min<size_t>(size, std::numeric_limits<ssize_t>::max()), offset);
      if(readSize < 0)
        throw Exception("reading file failed");
      if(readSize == 0)
        break;
      totalReadSize += readSize;
      size -= readSize;
      data += readSize;
      offset += readSize;
    }
    return totalReadSize;
#endif
  }

  void File::Write(uint64_t offset, Buffer const& buffer)
  {
#if defined(COIL_PLATFORM_WINDOWS)
    OVERLAPPED overlapped =
    {
      .Offset = (DWORD)(offset & 0xFFFFFFFF),
      .OffsetHigh = (DWORD)(offset >> 32),
    };
    DWORD bytesWritten;
    if(!::WriteFile(_hFile, buffer.data, buffer.size, &bytesWritten, &overlapped) || bytesWritten != buffer.size)
      throw Exception("writing file failed");
#elif defined(COIL_PLATFORM_POSIX)
    uint8_t const* data = (uint8_t const*)buffer.data;
    size_t size = buffer.size;
    while(size > 0)
    {
      size_t const writtenSize = ::pwrite(_fd, data, std::min<size_t>(size, std::numeric_limits<ssize_t>::max()), offset);
      if(writtenSize <= 0)
        throw Exception("writing file failed");
      size -= writtenSize;
      data += writtenSize;
      offset += writtenSize;
    }
#endif
  }

  uint64_t File::GetSize() const
  {
#if defined(COIL_PLATFORM_WINDOWS)
    LARGE_INTEGER size;
    if(!::GetFileSizeEx(_hFile, &size))
      throw Exception("getting file size failed");
    return size.QuadPart;
#elif defined(COIL_PLATFORM_POSIX)
    struct stat st;
    if(::fstat(_fd, &st) != 0)
      throw Exception("getting file size failed");
    return st.st_size;
#endif
  }

  File& File::Open(Book& book, std::string const& name, FileAccessMode accessMode, FileOpenMode openMode)
  {
    return book.Allocate<File>(DoOpen(name, accessMode, openMode));
  }

  File& File::OpenRead(Book& book, std::string const& name)
  {
    return Open(book, name, FileAccessMode::ReadOnly, FileOpenMode::MustExist);
  }

  File& File::OpenWrite(Book& book, std::string const& name)
  {
    return Open(book, name, FileAccessMode::WriteOnly, FileOpenMode::ExistAndTruncateOrCreate);
  }

  Buffer File::Map(Book& book, std::string const& name, FileAccessMode accessMode, FileOpenMode openMode)
  {
    try
    {
      File file = DoOpen(name, accessMode, openMode);
#if defined(COIL_PLATFORM_WINDOWS)
      // create mapping
      uint64_t size = file.GetSize();
      if((size_t)size != size)
        throw Exception("too big file mapping");
      DWORD protect = 0, desiredAccess = 0;
      switch(accessMode)
      {
      case FileAccessMode::ReadOnly:
        protect = PAGE_READONLY;
        desiredAccess = FILE_MAP_READ;
        break;
      case FileAccessMode::WriteOnly:
      case FileAccessMode::ReadWrite:
        protect = PAGE_READWRITE;
        desiredAccess = FILE_MAP_WRITE;
        break;
      }
      HANDLE hMapping = ::CreateFileMappingW(file._hFile, NULL, protect, 0, 0, 0);
      if(!hMapping)
        throw Exception("creating file mapping failed");

      // map file
      void* pMapping = ::MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
      ::CloseHandle(hMapping);
      if(!pMapping)
        throw Exception("mapping view failed");

      book.Allocate<FileMapping>(pMapping, size);

      return Buffer(pMapping, size);
#elif defined(COIL_PLATFORM_POSIX)
      uint64_t size = file.GetSize();
      if((size_t)size != size)
        throw Exception("too big file mapping");

      int prot = PROT_NONE;
      switch(accessMode)
      {
      case FileAccessMode::ReadOnly:
        prot = PROT_READ;
        break;
      case FileAccessMode::WriteOnly:
        prot = PROT_WRITE;
        break;
      case FileAccessMode::ReadWrite:
        prot = PROT_READ | PROT_WRITE;
        break;
      }

      void* pMapping = ::mmap(nullptr, size, prot, MAP_SHARED, file._fd, 0);
      if(pMapping == MAP_FAILED)
        throw Exception("mmap failed");

      book.Allocate<FileMapping>(pMapping, size);

      return { pMapping, size };
#endif
    }
    catch(Exception const& exception)
    {
      throw Exception("mapping file failed: ") << name << exception;
    }
  }

  Buffer File::MapRead(Book& book, std::string const& name)
  {
    return Map(book, name, FileAccessMode::ReadOnly, FileOpenMode::MustExist);
  }

  Buffer File::MapWrite(Book& book, std::string const& name)
  {
    return Map(book, name, FileAccessMode::WriteOnly, FileOpenMode::ExistAndTruncateOrCreate);
  }

  void File::Write(std::string const& name, Buffer const& buffer)
  {
    File(DoOpen(name, FileAccessMode::WriteOnly, FileOpenMode::ExistAndTruncateOrCreate)).Write(0, buffer);
  }

  void File::Seek(uint64_t offset)
  {
#if defined(COIL_PLATFORM_WINDOWS)
    if(!::SetFilePointerEx(_hFile, { .QuadPart = (LONGLONG)offset }, NULL, FILE_BEGIN))
      throw Exception("setting file pointer failed");
#elif defined(COIL_PLATFORM_POSIX)
    if(::lseek64(_fd, offset, SEEK_SET) == -1)
      throw Exception("seeking in file failed");
#endif
  }

#if defined(COIL_PLATFORM_WINDOWS)
  void* File::DoOpen(std::string const& name, FileAccessMode accessMode, FileOpenMode openMode)
  {
    std::wstring s;
    Unicode::Convert<char, char16_t>(name.begin(), name.end(), s);

    DWORD desiredAccess = 0;
    switch(accessMode)
    {
    case FileAccessMode::ReadOnly:
      desiredAccess = GENERIC_READ;
      break;
    case FileAccessMode::WriteOnly:
      desiredAccess = GENERIC_WRITE;
      break;
    case FileAccessMode::ReadWrite:
      desiredAccess = GENERIC_READ | GENERIC_WRITE;
      break;
    }

    DWORD creationDisposition = 0;
    switch(openMode)
    {
    case FileOpenMode::MustExist:
      creationDisposition = OPEN_EXISTING;
      break;
    case FileOpenMode::ExistOrCreate:
      creationDisposition = OPEN_ALWAYS;
      break;
    case FileOpenMode::ExistAndTruncateOrCreate:
      creationDisposition = CREATE_ALWAYS;
      break;
    case FileOpenMode::MustCreate:
      creationDisposition = CREATE_NEW;
      break;
    }

    HANDLE hFile = ::CreateFileW(s.c_str(), desiredAccess, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, creationDisposition, 0, NULL);
    if(hFile == INVALID_HANDLE_VALUE)
      throw Exception("opening file failed: ") << name;
    return hFile;
  }
#elif defined(COIL_PLATFORM_POSIX)
  int File::DoOpen(std::string const& name, FileAccessMode accessMode, FileOpenMode openMode)
  {
    int flags = 0;
    switch(accessMode)
    {
    case FileAccessMode::ReadOnly:
      flags |= O_RDONLY;
      break;
    case FileAccessMode::WriteOnly:
      flags |= O_WRONLY;
      break;
    case FileAccessMode::ReadWrite:
      flags |= O_RDWR;
      break;
    }
    switch(openMode)
    {
    case FileOpenMode::MustExist:
      break;
    case FileOpenMode::ExistOrCreate:
      flags |= O_CREAT;
      break;
    case FileOpenMode::ExistAndTruncateOrCreate:
      flags |= O_CREAT | O_TRUNC;
      break;
    case FileOpenMode::MustCreate:
      flags |= O_CREAT | O_EXCL;
      break;
    }
    int fd = ::open(name.c_str(), flags, 0644);
    if(fd < 0)
      throw Exception("opening file failed: ") << name;
    return fd;
  }
#endif

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

  size_t FileInputStream::Skip(size_t size)
  {
    size = std::min(size, _size - _offset);
    _offset += size;
    return size;
  }

  FileInputStream& FileInputStream::Open(Book& book, std::string const& name)
  {
    return book.Allocate<FileInputStream>(File::OpenRead(book, name));
  }

  FileOutputStream::FileOutputStream(File& file, uint64_t offset)
  : _file(file), _offset(offset) {}

  void FileOutputStream::Write(Buffer const& buffer)
  {
    _file.Write(_offset, buffer);
    _offset += buffer.size;
  }

  FileOutputStream& FileOutputStream::Open(Book& book, std::string const& name)
  {
    return book.Allocate<FileOutputStream>(File::OpenWrite(book, name));
  }

  std::string GetFsPathName(std::string const& path)
  {
    size_t i;
    for(i = path.length(); i > 0 && path[i - 1] != FsPathSeparator; --i);
    return path.substr(i);
  }

  std::string GetFsPathDirectory(std::string const& path)
  {
    size_t i;
    for(i = path.length() - 1; i > 0 && path[i] != FsPathSeparator; --i);
    return path.substr(0, i);
  }

  std::filesystem::path GetNativeFsPath(std::string const& path)
  {
    if constexpr(std::same_as<std::filesystem::path::string_type, std::string>)
    {
      return path;
    }
    else if constexpr(std::same_as<std::filesystem::path::string_type, std::wstring>)
    {
      std::wstring s;
      Unicode::Convert<char, char16_t>(path.begin(), path.end(), s);
      return std::move(s);
    }
  }
}
