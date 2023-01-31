#pragma once

#include "audio.hpp"
#include "ogg.hpp"

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
    // maximum frame count in packet (120ms = 3/25 s is maximum according to spec)
    static constexpr int32_t maxPacketFramesCount = samplingRate * 3 / 25;

  private:
    OggDecodeStream& _inputStream;
    Buffer _packet;
    int32_t _channelsCount;
    OpusDecoder* _decoder = nullptr;
    std::vector<float> _buffer;
  };
}
