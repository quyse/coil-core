#include "opus.hpp"
#include <opus/opus.h>

namespace Coil
{
  OpusDecodeStream::OpusDecodeStream(OggDecodeStream& inputStream)
  : _inputStream(inputStream)
  {
    _packet = _inputStream.ReadPacket();
    _channelsCount = opus_packet_get_nb_channels((uint8_t const*)_packet.data);
    int err;
    _decoder = opus_decoder_create(samplingRate, _channelsCount, &err);
    if(err != OPUS_OK)
      throw Exception("creating Opus decoder failed");
  }

  OpusDecodeStream::~OpusDecodeStream()
  {
    opus_decoder_destroy(_decoder);
  }

  AudioFormat OpusDecodeStream::GetFormat() const
  {
    return
    {
      .channels = GetAudioFormatChannelsFromCount(_channelsCount),
      .samplingRate = samplingRate,
    };
  }

  Buffer OpusDecodeStream::Read(int32_t framesCount)
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

  OpusStreamSource::OpusStreamSource(Buffer const& buffer)
  : _buffer(buffer) {}

  OpusDecodeStream& OpusStreamSource::CreateStream(Book& book)
  {
    return book.Allocate<OpusDecodeStream>(book.Allocate<OggDecodeStream>(book.Allocate<BufferInputStream>(_buffer)));
  }
}
