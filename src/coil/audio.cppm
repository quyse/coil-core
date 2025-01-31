module;

#include <string_view>

export module coil.core.audio;

import coil.core.base;

export namespace Coil
{
  enum class AudioFormatChannels
  {
    Mono,
    Stereo,
  };

  int32_t GetAudioFormatChannelsCount(AudioFormatChannels channels)
  {
    switch(channels)
    {
    case AudioFormatChannels::Mono:
      return 1;
    case AudioFormatChannels::Stereo:
      return 2;
    }
  }

  AudioFormatChannels GetAudioFormatChannelsFromCount(int32_t channelsCount)
  {
    switch(channelsCount)
    {
    case 1:
      return AudioFormatChannels::Mono;
    case 2:
      return AudioFormatChannels::Stereo;
    default:
      throw Exception("unknown audio channels count");
    }
  }

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
    virtual ~AudioStream() = default;

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
    virtual ~AudioDevice() = default;

    // resume or stop playing
    virtual void SetPlaying(bool playing) = 0;
  };

  // audio stream factory
  class AudioStreamSource
  {
  public:
    virtual ~AudioStreamSource() = default;

    // create new stream playing audio from start
    virtual AudioStream& CreateStream(Book& book) = 0;
  };

  template <>
  struct AssetTraits<AudioStreamSource*>
  {
    static constexpr std::string_view assetTypeName = "audio";
  };
  static_assert(IsAsset<AudioStreamSource*>);
}
