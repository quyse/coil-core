#pragma once

#include "tasks.hpp"
#include "tasks_storage.hpp"
#include "data.hpp"
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
    FsPathInput(char const* path);
    FsPathInput(std::string const& path);
    FsPathInput(std::string&& path);
    FsPathInput(std::string_view path);
    FsPathInput(std::filesystem::path const& path);
    FsPathInput(std::filesystem::path&& path);

    FsPathInput(FsPathInput const&) = delete;
    FsPathInput(FsPathInput&&) = delete;

    std::filesystem::path::value_type const* GetCStr() const;
    std::filesystem::path GetNativePath() const&;
    std::filesystem::path GetNativePath() &&;

    std::string GetString() const&;
    std::string GetString() &&;

  private:
    std::variant<std::filesystem::path::value_type const*, std::filesystem::path> _path;
  };

  class File final : public ReadableStorage, public WritableStorage, public AsyncReadableStorage, public AsyncWritableStorage
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

    // ReadableStorage
    uint64_t GetSize() const override;
    size_t Read(uint64_t offset, Buffer const& buffer) const override;
    // WritableStorage
    void Write(uint64_t offset, Buffer const& buffer) override;
    // AsyncReadableStorage
    Task<size_t> AsyncRead(uint64_t offset, Buffer const& buffer) const override;
    // AsyncWritableStorage
    Task<void> AsyncWrite(uint64_t offset, Buffer const& buffer) override;

    void SetModeExecutable(bool executable);

    static File& Open(Book& book, FsPathInput const& path, FileAccessMode accessMode, FileOpenMode openMode, FileAdviseMode adviseMode = FileAdviseMode::None);
    static File& OpenRead(Book& book, FsPathInput const& path, FileAdviseMode adviseMode = FileAdviseMode::None);
    static File& OpenWrite(Book& book, FsPathInput const& path, FileAdviseMode adviseMode = FileAdviseMode::None);

    static Buffer Map(Book& book, FsPathInput const& path, FileAccessMode accessMode, FileOpenMode openMode, FileAdviseMode adviseMode = FileAdviseMode::None);
    static Buffer MapRead(Book& book, FsPathInput const& path, FileAdviseMode adviseMode = FileAdviseMode::None);
    static Buffer MapWrite(Book& book, FsPathInput const& path, FileAdviseMode adviseMode = FileAdviseMode::None);

    static void Write(FsPathInput const& path, Buffer const& buffer);

  private:
    void Seek(uint64_t offset);
#if defined(COIL_PLATFORM_WINDOWS)
    // not using Windows HANDLE to not include windows.h
    static void* DoOpen(FsPathInput const& path, FileAccessMode accessMode, FileOpenMode openMode, FileAdviseMode adviseMode);

    void* _hFile = nullptr;
#elif defined(COIL_PLATFORM_POSIX)
    static int DoOpen(FsPathInput const& path, FileAccessMode accessMode, FileOpenMode openMode, FileAdviseMode adviseMode);

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

    static FileInputStream& Open(Book& book, FsPathInput const& path);

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

    static FileOutputStream& Open(Book& book, FsPathInput const& path);

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
}
