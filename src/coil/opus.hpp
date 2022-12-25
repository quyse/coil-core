#pragma once

#include "ogg.hpp"

struct OpusDecoder;

namespace Coil
{
  class OpusDecodeStream
  {
  public:
    OpusDecodeStream(OggDecodeStream& inputStream);
    ~OpusDecodeStream();

    // read one packet, empty buffer if EOF
    // buffer is valid only until next read
    Buffer Read();

    // always decode at 48 kHz
    static constexpr int32_t samplingRate = 48000;
    // maximum sample count in packet (120ms = 3/25 s is maximum according to spec)
    static constexpr int32_t maxPacketSamplesCount = samplingRate * 3 / 25;

  private:
    OggDecodeStream& _inputStream;
    OpusDecoder* _decoder = nullptr;
    std::vector<float> _buffer;
  };
}
