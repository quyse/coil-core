#pragma once

#include "base.hpp"

namespace Coil
{
  enum class AudioFormatChannels
  {
    Mono,
    Stereo,
  };

  int32_t GetAudioFormatChannelsCount(AudioFormatChannels channels);
  AudioFormatChannels GetAudioFormatChannelsFromCount(int32_t channelsCount);

  // float is always used for samples
  using AudioSample = float;

  // audio format
  struct AudioFormat
  {
    // recommended sampling rate
    static constexpr int32_t recommendedSamplingRate = 48000;

    // channels configuration
    AudioFormatChannels channels;
    // sampling rate in Hz
    int32_t samplingRate = recommendedSamplingRate;

    friend auto operator<=>(AudioFormat const&, AudioFormat const&) = default;
  };

  // audio input stream
  // all methods must be thread safe
  class AudioStream
  {
  public:
    virtual ~AudioStream() {}

    // get stream format
    // must be constant while stream exists
    virtual AudioFormat GetFormat() const = 0;

    // read some samples
    // framesCount specifies recommended number of frames,
    // according to the latency of the device
    // if stream returns more frames, they will be queued,
    // if less, the method will be called again
    // zero frames returned means EOF
    // buffer is allowed to be valid only until next read
    virtual Buffer Read(int32_t framesCount) = 0;
  };

  class AudioDevice
  {
  public:
    // resume or stop playing
    virtual void SetPlaying(bool playing) = 0;
  };
}
