module;

#include <opus/opus.h>
#include <string_view>
#include <vector>

export module coil.core.audio.opus;

import coil.core.audio;
import coil.core.base;

export namespace Coil
{
  class OpusDecodeStream final : public AudioStream
  {
  public:
    OpusDecodeStream(PacketInputStream& inputStream)
    : _inputStream(inputStream)
    {
      _packet = _inputStream.ReadPacket();
      _channelsCount = opus_packet_get_nb_channels((uint8_t const*)_packet.data);
      int err;
      _decoder = opus_decoder_create(samplingRate, _channelsCount, &err);
      if(err != OPUS_OK)
        throw Exception("creating Opus decoder failed");
    }

    ~OpusDecodeStream()
    {
      opus_decoder_destroy(_decoder);
    }

    AudioFormat GetFormat() const override
    {
      return
      {
        .channels = GetAudioFormatChannelsFromCount(_channelsCount),
        .samplingRate = samplingRate,
      };
    }

    Buffer Read(int32_t framesCount) override
    {
      if(!_packet)
      {
        _packet = _inputStream.ReadPacket();
        if(!_packet) return {};
      }

      _buffer.resize(maxPacketFramesCount * _channelsCount);
      int32_t samplesCount = opus_decode_float(_decoder, (uint8_t const*)_packet.data, _packet.size, _buffer.data(), maxPacketFramesCount, 0);
      _packet = {};
      _buffer.resize(samplesCount * _channelsCount);
      return _buffer;
    }

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
    OpusDecodeStreamSource(PacketInputStreamSource& inputStreamSource)
    : _inputStreamSource(inputStreamSource) {}

    OpusDecodeStream& CreateStream(Book& book) override
    {
      return book.Allocate<OpusDecodeStream>(_inputStreamSource.CreateStream(book));
    }

  private:
    PacketInputStreamSource& _inputStreamSource;
  };

  class OpusAssetLoader
  {
  public:
    template <typename Asset, typename AssetContext>
    requires std::convertible_to<OpusDecodeStreamSource*, Asset>
    Asset LoadAsset(Book& book, AssetContext& assetContext) const
    {
      return &book.Allocate<OpusDecodeStreamSource>(
        *assetContext.template LoadAssetParam<PacketInputStreamSource*>(book, "source")
      );
    }

    static constexpr std::string_view assetLoaderName = "opus";
  };
  static_assert(IsAssetLoader<OpusAssetLoader>);
}
