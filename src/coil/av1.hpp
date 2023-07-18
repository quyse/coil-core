#pragma once

#include "tasks.hpp"
#include "video.hpp"
#include <gav1/decoder.h>

namespace Coil
{
  class Av1DecodeStream final : public VideoStream
  {
  public:
    Av1DecodeStream(PacketInputStream& inputStream);

    // VideoStream's methods
    VideoFrame ReadFrame() override;

  private:
    static VideoFrame::Format GetFormat(libgav1::ImageFormat format);
    static VideoFrame::ColorRange GetColorRange(libgav1::ColorRange colorRange);

    PacketInputStream& _inputStream;
    libgav1::Decoder _decoder;
  };

  class Av1DecodeStreamSource final : public VideoStreamSource
  {
  public:
    Av1DecodeStreamSource(PacketInputStreamSource& inputStreamSource);

    Av1DecodeStream& CreateStream(Book& book) override;

  private:
    PacketInputStreamSource& _inputStreamSource;
  };

  class Av1AssetLoader
  {
  public:
    template <typename Asset, typename AssetContext>
    requires std::convertible_to<Av1DecodeStreamSource*, Asset>
    Task<Asset> LoadAsset(Book& book, AssetContext& assetContext) const
    {
      co_return &book.Allocate<Av1DecodeStreamSource>(
        *co_await assetContext.template LoadAssetParam<PacketInputStreamSource*>(book, "source")
      );
    }

    static constexpr std::string_view assetLoaderName = "av1";
  };
  static_assert(IsAssetLoader<Av1AssetLoader>);
}
