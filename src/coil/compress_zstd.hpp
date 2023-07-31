#pragma once

#include "tasks.hpp"
#include <zstd.h>

namespace Coil
{
  class ZstdCompressStream final : public OutputStream
  {
  public:
    ZstdCompressStream(OutputStream& outputStream);
    ~ZstdCompressStream();

    void Write(Buffer const& buffer) override;
    void End() override;

  private:
    void _Write(Buffer const& buffer, ZSTD_EndDirective op);

    OutputStream& _outputStream;
    ZSTD_CStream* _stream = nullptr;
  };

  class ZstdDecompressStream final : public InputStream
  {
  public:
    ZstdDecompressStream(InputStream& inputStream);
    ~ZstdDecompressStream();

    size_t Read(Buffer const& buffer) override;

  private:
    InputStream& _inputStream;
    ZSTD_DStream* _stream = nullptr;
    uint8_t _inBufferData[0x100];
    ZSTD_inBuffer _inBuffer =
    {
      .src = _inBufferData,
      .size = 0,
      .pos = 0,
    };
  };

  class ZstdDecompressStreamSource final : public InputStreamSource
  {
  public:
    ZstdDecompressStreamSource(InputStreamSource& source);

    InputStream& CreateStream(Book& book) override;

  private:
    InputStreamSource& _source;
  };

  class ZstdAssetLoader
  {
  public:
    template <typename Asset, typename AssetContext>
    requires std::convertible_to<ZstdDecompressStreamSource*, Asset>
    Task<Asset> LoadAsset(Book& book, AssetContext& assetContext) const
    {
      co_return &book.Allocate<ZstdDecompressStreamSource>(
        *co_await assetContext.template LoadAssetParam<InputStreamSource*>(book, "source")
      );
    }

    static constexpr std::string_view assetLoaderName = "zstd";
  };
  static_assert(IsAssetLoader<ZstdAssetLoader>);
}
