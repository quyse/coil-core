#pragma once

#include "audio.hpp"
#include <memory>

namespace Coil
{
  class PulseAudioDevice : public AudioDevice
  {
  public:
    PulseAudioDevice(AudioStream& stream);
    ~PulseAudioDevice();

    static PulseAudioDevice& Init(Book& book, AudioStream& stream);

    void SetPlaying(bool playing) override;

  private:
    class Impl;

    void Init();
    void ThreadFunc();

    AudioStream& _stream;
    std::unique_ptr<Impl> _impl;
  };
}
