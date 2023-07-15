#pragma once

#include "audio.hpp"
#include "ogg.hpp"
#include "tasks.hpp"

struct OpusDecoder;

namespace Coil
{
  class OpusDecodeStream : public AudioStream
  {
  public:
    OpusDecodeStream(OggDecodeStream& inputStream);
    ~OpusDecodeStream();

    AudioFormat GetFormat() const override;
    Buffer Read(int32_t framesCount) override;

    // always decode at 48 kHz
    static constexpr int32_t samplingRate = 48000;
    static_assert(samplingRate == AudioFormat::recommendedSamplingRate);
    // maximum frame count in packet (120ms = 3/25 s is maximum according to spec)
    static constexpr int32_t maxPacketFramesCount = samplingRate * 3 / 25;

  private:
    OggDecodeStream& _inputStream;
    Buffer _packet;
    int32_t _channelsCount;
    OpusDecoder* _decoder = nullptr;
    std::vector<float> _buffer;
  };

  class OpusStreamSource final : public AudioStreamSource
  {
  public:
    OpusStreamSource(Buffer const& buffer);

    OpusDecodeStream& CreateStream(Book& book) override;

  private:
    Buffer const _buffer;
  };

  class OpusAssetLoader
  {
  public:
    template <std::same_as<AudioAsset> Asset, typename AssetContext>
    Task<Asset> LoadAsset(Book& book, AssetContext& assetContext) const
    {
      auto buffer = co_await assetContext.template LoadAssetParam<Buffer>(book, "buffer");
      co_return
      {
        .source = &book.Allocate<OpusStreamSource>(buffer),
      };
    }

    static constexpr std::string_view assetLoaderName = "audio_opus";
  };
  static_assert(IsAssetLoader<OpusAssetLoader>);
}
