#include "opus.hpp"
#include <opus/opus.h>

namespace Coil
{
  OpusDecodeStream::OpusDecodeStream(OggDecodeStream& inputStream)
  : _inputStream(inputStream) {}

  OpusDecodeStream::~OpusDecodeStream()
  {
    if(_decoder)
    {
      opus_decoder_destroy(_decoder);
    }
  }

  Buffer OpusDecodeStream::Read()
  {
    Buffer packet = _inputStream.ReadPacket();
    if(!packet) return {};

    int32_t channelsCount = opus_packet_get_nb_channels((uint8_t const*)packet.data);
    if(!_decoder)
    {
      int err;
      _decoder = opus_decoder_create(samplingRate, channelsCount, &err);
      if(err != OPUS_OK)
        throw Exception("creating Opus decoder failed");
    }
    _buffer.resize(maxPacketSamplesCount * channelsCount);
    int32_t samplesCount = opus_decode_float(_decoder, (uint8_t const*)packet.data, packet.size, _buffer.data(), maxPacketSamplesCount, 0);
    _buffer.resize(samplesCount * channelsCount);
    return _buffer;
  }
}
