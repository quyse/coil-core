#pragma once

#include "tasks.hpp"
#include <concepts>
#include <filesystem>
#include <string>
#include <string_view>

namespace Coil
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

  class File
  {
  public:
#if defined(COIL_PLATFORM_WINDOWS)
    File(void* hFile);
#elif defined(COIL_PLATFORM_POSIX)
    File(int fd);
#endif
    ~File();

    File(File const&) = delete;
    File(File&&) = delete;

    size_t Read(uint64_t offset, Buffer const& buffer);
    void Write(uint64_t offset, Buffer const& buffer);
    uint64_t GetSize() const;

    static File& Open(Book& book, std::string_view name, FileAccessMode accessMode, FileOpenMode openMode);
    static File& OpenRead(Book& book, std::string_view name);
    static File& OpenWrite(Book& book, std::string_view name);

    static Buffer Map(Book& book, std::string_view name, FileAccessMode accessMode, FileOpenMode openMode);
    static Buffer MapRead(Book& book, std::string_view name);
    static Buffer MapWrite(Book& book, std::string_view name);

    static void Write(std::string_view name, Buffer const& buffer);

  private:
    void Seek(uint64_t offset);
#if defined(COIL_PLATFORM_WINDOWS)
    // not using Windows HANDLE to not include windows.h
    static void* DoOpen(std::string_view name, FileAccessMode accessMode, FileOpenMode openMode);

    void* _hFile = nullptr;
#elif defined(COIL_PLATFORM_POSIX)
    static int DoOpen(std::string_view name, FileAccessMode accessMode, FileOpenMode openMode);

    int _fd = -1;
#endif
  };

  // Input stream which reads part of a file.
  class FileInputStream final : public InputStream
  {
  public:
    FileInputStream(File& file, uint64_t offset = 0);
    FileInputStream(File& file, uint64_t offset, uint64_t size);

    size_t Read(Buffer const& buffer) override;
    size_t Skip(size_t size) override;

    static FileInputStream& Open(Book& book, std::string_view name);

  private:
    File& _file;
    uint64_t _offset;
    uint64_t _size;
  };

  class FileOutputStream final : public OutputStream
  {
  public:
    FileOutputStream(File& file, uint64_t offset = 0);

    void Write(Buffer const& buffer) override;

    static FileOutputStream& Open(Book& book, std::string_view name);

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
  std::string_view GetFsPathName(std::string_view path);
  // Get directory from path.
  std::string_view GetFsPathDirectory(std::string_view path);

  // Convert path to native C++ path type.
  std::filesystem::path GetNativeFsPath(std::string_view path);
}
