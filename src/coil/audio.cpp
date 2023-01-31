#include "audio.hpp"

namespace Coil
{
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
}
