module;

#include "base.hpp"
#include <concepts>
#include <coroutine>
#include <filesystem>
#include <string>
#include <string_view>
#include <variant>

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

export module coil.core.fs;

import coil.core.base;
import coil.core.data;
import coil.core.tasks.storage;
import coil.core.tasks;
import coil.core.unicode;

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

export namespace Coil
{
  enum class FileAccessMode
  {
    ReadOnly,
    WriteOnly,
    ReadWrite,
  };

  enum class FileOpenMode
  {
    MustExist,
    ExistOrCreate,
    ExistAndTruncateOrCreate,
    MustCreate,
  };

  enum class FileAdviseMode
  {
    None,
    Sequential,
    Random,
  };

  // stores path as a pointer to null-terminated string or actual storage
  class FsPathInput
  {
  public:
    FsPathInput(char const* path)
    {
      if constexpr(std::same_as<std::filesystem::path::value_type, char>)
      {
        _path = path;
      }
      if constexpr(std::same_as<std::filesystem::path::value_type, wchar_t>)
      {
        std::wstring wpath;
        Unicode::Convert<char, char16_t>(path, wpath);
        _path = std::filesystem::path{std::move(wpath)};
      }
    }
    FsPathInput(std::string const& path)
    {
      if constexpr(std::same_as<std::filesystem::path::value_type, char>)
      {
        _path = std::filesystem::path{path};
      }
      if constexpr(std::same_as<std::filesystem::path::value_type, wchar_t>)
      {
        std::wstring wpath;
        Unicode::Convert<char, char16_t>(path.begin(), path.end(), wpath);
        _path = std::filesystem::path{std::move(wpath)};
      }
    }
    FsPathInput(std::string&& path)
    {
      if constexpr(std::same_as<std::filesystem::path::value_type, char>)
      {
        _path = std::filesystem::path{std::move(path)};
      }
      if constexpr(std::same_as<std::filesystem::path::value_type, wchar_t>)
      {
        std::wstring wpath;
        Unicode::Convert<char, char16_t>(path.begin(), path.end(), wpath);
        _path = std::filesystem::path{std::move(wpath)};
      }
    }
    FsPathInput(std::string_view path)
    {
      if constexpr(std::same_as<std::filesystem::path::value_type, char>)
      {
        _path = std::filesystem::path{path};
      }
      if constexpr(std::same_as<std::filesystem::path::value_type, wchar_t>)
      {
        std::wstring wpath;
        Unicode::Convert<char, char16_t>(path.begin(), path.end(), wpath);
        _path = std::filesystem::path{std::move(wpath)};
      }
    }
    FsPathInput(std::filesystem::path const& path)
    : _path{path.c_str()} {}
    FsPathInput(std::filesystem::path&& path)
    : _path{std::move(path)} {}

    FsPathInput(FsPathInput const&) = delete;
    FsPathInput(FsPathInput&&) = delete;

    std::filesystem::path::value_type const* GetCStr() const
    {
      return std::visit([this](auto const& path) -> std::filesystem::path::value_type const*
      {
        if constexpr(std::same_as<std::decay_t<decltype(path)>, std::filesystem::path::value_type const*>)
        {
          return path;
        }
        else
        {
          return path.c_str();
        }
      }, _path);
    }
    std::filesystem::path GetNativePath() const&
    {
      return std::visit([this](auto const& path) -> std::filesystem::path
      {
        if constexpr(std::same_as<std::decay_t<decltype(path)>, std::filesystem::path::value_type const*>)
        {
          return { path };
        }
        else
        {
          return path;
        }
      }, _path);
    }
    std::filesystem::path GetNativePath() &&
    {
      return std::visit([this](auto&& path) -> std::filesystem::path
      {
        if constexpr(std::same_as<std::decay_t<decltype(path)>, std::filesystem::path::value_type const*>)
        {
          return { path };
        }
        else
        {
          return std::move(path);
        }
      }, std::move(_path));
    }

    std::string GetString() const&
    {
      return std::visit([this](auto const& path) -> std::string
      {
        if constexpr(std::same_as<std::decay_t<decltype(path)>, std::filesystem::path::value_type const*>)
        {
          if constexpr(std::same_as<std::filesystem::path::value_type, char>)
          {
            return path;
          }
          if constexpr(std::same_as<std::filesystem::path::value_type, wchar_t>)
          {
            std::string spath;
            Unicode::Convert<char16_t, char>(path, spath);
            return std::move(spath);
          }
        }
        else
        {
          if constexpr(std::same_as<std::filesystem::path::value_type, char>)
          {
            return path.string();
          }
          if constexpr(std::same_as<std::filesystem::path::value_type, wchar_t>)
          {
            std::string spath;
            Unicode::Convert<char16_t, char>(path.c_str(), spath);
            return std::move(spath);
          }
        }
      }, _path);
    }
    std::string GetString() &&
    {
      return std::visit([this](auto&& path) -> std::string
      {
        if constexpr(std::same_as<std::decay_t<decltype(path)>, std::filesystem::path::value_type const*>)
        {
          if constexpr(std::same_as<std::filesystem::path::value_type, char>)
          {
            return path;
          }
          if constexpr(std::same_as<std::filesystem::path::value_type, wchar_t>)
          {
            std::string spath;
            Unicode::Convert<char16_t, char>(path, spath);
            return std::move(spath);
          }
        }
        else
        {
          if constexpr(std::same_as<std::filesystem::path::value_type, char>)
          {
            return std::move(path).string();
          }
          if constexpr(std::same_as<std::filesystem::path::value_type, wchar_t>)
          {
            std::string spath;
            Unicode::Convert<char16_t, char>(path.c_str(), spath);
            return std::move(spath);
          }
        }
      }, std::move(_path));
    }

  private:
    std::variant<std::filesystem::path::value_type const*, std::filesystem::path> _path;
  };

  class File final : public ReadableStorage, public WritableStorage, public AsyncReadableStorage, public AsyncWritableStorage
  {
  public:
#if defined(COIL_PLATFORM_WINDOWS)
    File(void* hFile)
    : _hFile(hFile) {}
#elif defined(COIL_PLATFORM_POSIX)
    File(int fd)
    : _fd(fd) {}
#endif
    ~File()
    {
#if defined(COIL_PLATFORM_WINDOWS)
      if(_hFile)
        ::CloseHandle(_hFile);
#elif defined(COIL_PLATFORM_POSIX)
      if(_fd >= 0)
        ::close(_fd);
#endif
    }

    File(File const&) = delete;
    File(File&&) = delete;

    // ReadableStorage
    uint64_t GetSize() const override
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

    size_t Read(uint64_t offset, Buffer const& buffer) const override
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

    // WritableStorage
    void Write(uint64_t offset, Buffer const& buffer) override
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

    // AsyncReadableStorage
    Task<size_t> AsyncRead(uint64_t offset, Buffer const& buffer) const override
    {
      co_return Read(offset, buffer);
    }

    // AsyncWritableStorage
    Task<void> AsyncWrite(uint64_t offset, Buffer const& buffer) override
    {
      Write(offset, buffer);
      co_return;
    }

    void SetModeExecutable(bool executable)
    {
#if defined(COIL_PLATFORM_WINDOWS)
      // not supported on Windows
#elif defined(COIL_PLATFORM_POSIX)
      ::fchmod(_fd, executable ? 0755 : 0644);
#endif
    }

    static File& Open(Book& book, FsPathInput const& path, FileAccessMode accessMode, FileOpenMode openMode, FileAdviseMode adviseMode = FileAdviseMode::None)
    {
      return book.Allocate<File>(DoOpen(path, accessMode, openMode, adviseMode));
    }
    static File& OpenRead(Book& book, FsPathInput const& path, FileAdviseMode adviseMode = FileAdviseMode::None)
    {
      return Open(book, path, FileAccessMode::ReadOnly, FileOpenMode::MustExist, adviseMode);
    }
    static File& OpenWrite(Book& book, FsPathInput const& path, FileAdviseMode adviseMode = FileAdviseMode::None)
    {
      return Open(book, path, FileAccessMode::WriteOnly, FileOpenMode::ExistAndTruncateOrCreate, adviseMode);
    }

    static Buffer Map(Book& book, FsPathInput const& path, FileAccessMode accessMode, FileOpenMode openMode, FileAdviseMode adviseMode = FileAdviseMode::None)
    {
      try
      {
        File file = DoOpen(path, accessMode, openMode, adviseMode);
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

        return Buffer(pMapping, (size_t)size);
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

        return { pMapping, (size_t)size };
#endif
      }
      catch(Exception const& exception)
      {
        throw Exception("mapping file failed: ") << path.GetString() << exception;
      }
    }
    static Buffer MapRead(Book& book, FsPathInput const& path, FileAdviseMode adviseMode = FileAdviseMode::None)
    {
      return Map(book, path, FileAccessMode::ReadOnly, FileOpenMode::MustExist, adviseMode);
    }
    static Buffer MapWrite(Book& book, FsPathInput const& path, FileAdviseMode adviseMode = FileAdviseMode::None)
    {
      return Map(book, path, FileAccessMode::WriteOnly, FileOpenMode::ExistAndTruncateOrCreate, adviseMode);
    }

    static void Write(FsPathInput const& path, Buffer const& buffer)
    {
      File(DoOpen(path, FileAccessMode::WriteOnly, FileOpenMode::ExistAndTruncateOrCreate, FileAdviseMode::Sequential)).Write(0, buffer);
    }

  private:
    void Seek(uint64_t offset)
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
    // not using Windows HANDLE to not include windows.h
    static void* DoOpen(FsPathInput const& path, FileAccessMode accessMode, FileOpenMode openMode, FileAdviseMode adviseMode)
    {
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

      DWORD flags = 0;
      switch(adviseMode)
      {
      case FileAdviseMode::None:
        break;
      case FileAdviseMode::Sequential:
        flags |= FILE_FLAG_SEQUENTIAL_SCAN;
        break;
      case FileAdviseMode::Random:
        flags |= FILE_FLAG_RANDOM_ACCESS;
        break;
      }

      HANDLE hFile = ::CreateFileW(path.GetCStr(), desiredAccess, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, creationDisposition, flags, NULL);
      if(hFile == INVALID_HANDLE_VALUE)
        throw Exception("opening file failed: ") << path.GetString();
      return hFile;
    }

    void* _hFile = nullptr;
#elif defined(COIL_PLATFORM_POSIX)
    static int DoOpen(FsPathInput const& path, FileAccessMode accessMode, FileOpenMode openMode, FileAdviseMode adviseMode)
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

      int fd = ::open(path.GetCStr(), flags, 0644);
      if(fd < 0)
        throw Exception("opening file failed: ") << path.GetString();

      int advice = POSIX_FADV_NORMAL;
      switch(adviseMode)
      {
      case FileAdviseMode::None:
        break;
      case FileAdviseMode::Sequential:
        advice = POSIX_FADV_SEQUENTIAL;
        break;
      case FileAdviseMode::Random:
        advice = POSIX_FADV_RANDOM;
        break;
      }
      if(advice != POSIX_FADV_NORMAL)
      {
        ::posix_fadvise(fd, 0, 0, advice);
      }

      return fd;
    }

    int _fd = -1;
#endif
  };

  // Input stream which reads part of a file.
  class FileInputStream final : public InputStream
  {
  public:
    FileInputStream(File& file, uint64_t offset = 0)
    : _file(file), _offset(offset), _size(file.GetSize()) {}
    FileInputStream(File& file, uint64_t offset, uint64_t size)
    : _file(file), _offset(offset), _size(size) {}

    size_t Read(Buffer const& buffer) override
    {
      size_t read = _file.Read(_offset, buffer);
      _offset += read;
      return read;
    }

    size_t Skip(size_t size) override
    {
      size = (size_t)std::min<uint64_t>(size, _size - _offset);
      _offset += size;
      return size;
    }

    static FileInputStream& Open(Book& book, FsPathInput const& path)
    {
      return book.Allocate<FileInputStream>(File::OpenRead(book, path, FileAdviseMode::Sequential));
    }

  private:
    File& _file;
    uint64_t _offset;
    uint64_t _size;
  };

  class FileOutputStream final : public OutputStream
  {
  public:
    FileOutputStream(File& file, uint64_t offset = 0)
    : _file(file), _offset(offset) {}

    void Write(Buffer const& buffer) override
    {
      _file.Write(_offset, buffer);
      _offset += buffer.size;
    }

    static FileOutputStream& Open(Book& book, FsPathInput const& path)
    {
      return book.Allocate<FileOutputStream>(File::OpenWrite(book, path, FileAdviseMode::Sequential));
    }

  private:
    File& _file;
    uint64_t _offset;
  };

  class FileAssetLoader
  {
  public:
    template <typename Asset, typename AssetContext>
    requires std::same_as<Asset, Buffer> || std::convertible_to<BufferInputStreamSource*, Asset>
    Task<Asset> LoadAsset(Book& book, AssetContext& assetContext) const
    {
      auto buffer = File::MapRead(book, assetContext.GetParam("path"));
      if constexpr(std::same_as<Asset, Buffer>)
      {
        co_return buffer;
      }
      else
      {
        co_return &book.Allocate<BufferInputStreamSource>(buffer);
      }
    }

    static constexpr std::string_view assetLoaderName = "file";
  };
  static_assert(IsAssetLoader<FileAssetLoader>);

  constexpr char const FsPathSeparator =
#if defined(COIL_PLATFORM_WINDOWS)
    '\\'
#else
    '/'
#endif
  ;

  // Get name from path.
  std::string_view GetFsPathName(std::string_view path)
  {
    size_t i;
    for(i = path.length(); i > 0 && path[i - 1] != FsPathSeparator; --i);
    return path.substr(i);
  }
  // Get directory from path.
  std::string_view GetFsPathDirectory(std::string_view path)
  {
    size_t i;
    for(i = path.length() - 1; i > 0 && path[i] != FsPathSeparator; --i);
    return path.substr(0, i);
  }
}
