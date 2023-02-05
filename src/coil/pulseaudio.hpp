#pragma once

#include "audio.hpp"
#include <memory>

namespace Coil
{
  class PulseAudioDevice : public AudioDevice
  {
  public:
    PulseAudioDevice(std::unique_ptr<AudioStream>&& stream);
    ~PulseAudioDevice();

    static PulseAudioDevice& Init(Book& book, std::unique_ptr<AudioStream>&& stream);

    void SetPlaying(bool playing) override;

  private:
    class Impl;

    void Init();
    void ThreadFunc();

    std::unique_ptr<AudioStream> _stream;
    std::unique_ptr<Impl> _impl;
  };
}
