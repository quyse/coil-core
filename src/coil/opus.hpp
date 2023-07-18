#pragma once

#include "audio.hpp"
#include "tasks.hpp"

struct OpusDecoder;

namespace Coil
{
  class OpusDecodeStream final : public AudioStream
  {
  public:
    OpusDecodeStream(PacketInputStream& inputStream);
    ~OpusDecodeStream();

    AudioFormat GetFormat() const override;
    Buffer Read(int32_t framesCount) override;

    // always decode at 48 kHz
    static constexpr int32_t samplingRate = 48000;
    static_assert(samplingRate == AudioFormat::recommendedSamplingRate);
    // maximum frame count in packet (120ms = 3/25 s is maximum according to spec)
    static constexpr int32_t maxPacketFramesCount = samplingRate * 3 / 25;

  private:
    PacketInputStream& _inputStream;
    Buffer _packet;
    int32_t _channelsCount;
    OpusDecoder* _decoder = nullptr;
    std::vector<float> _buffer;
  };

  class OpusDecodeStreamSource final : public AudioStreamSource
  {
  public:
    OpusDecodeStreamSource(PacketInputStreamSource& inputStreamSource);

    OpusDecodeStream& CreateStream(Book& book) override;

  private:
    PacketInputStreamSource& _inputStreamSource;
  };

  class OpusAssetLoader
  {
  public:
    template <typename Asset, typename AssetContext>
    requires std::convertible_to<OpusDecodeStreamSource*, Asset>
    Task<Asset> LoadAsset(Book& book, AssetContext& assetContext) const
    {
      co_return &book.Allocate<OpusDecodeStreamSource>(
        *co_await assetContext.template LoadAssetParam<PacketInputStreamSource*>(book, "source")
      );
    }

    static constexpr std::string_view assetLoaderName = "opus";
  };
  static_assert(IsAssetLoader<OpusAssetLoader>);
}
