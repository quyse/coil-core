#pragma once

#include "audio.hpp"
#include "windows.hpp"
#include <mutex>
#include <queue>
#include <thread>

struct IAudioClient;
struct IAudioRenderClient;

namespace Coil
{
  class WinCoreAudioDevice : public AudioDevice
  {
  public:
    WinCoreAudioDevice(AudioStream& stream);
    ~WinCoreAudioDevice();

    static WinCoreAudioDevice& Init(Book& book, AudioStream& stream);

    void SetPlaying(bool playing) override;

  private:
    void Init();
    void ThreadFunc();

    template <void (WinCoreAudioDevice::*handler)()>
    void Queue()
    {
      UserAPCHandler<handler>::Queue(_thread, this);
    }

    void Fill();
    void OnShutdown();

    AudioStream& _stream;
    std::thread _thread;
    bool _playing = true;
    // the following members are accessed only in the audio thread
    bool _working = true;
    ComPtr<IAudioClient> _pAudioClient;
    ComPtr<IAudioRenderClient> _pAudioRenderClient;
    Handle _hEvent;
    uint32_t _bufferFramesCount = 0;
    size_t _frameSize = 0;
    Buffer _currentBuffer;
  };
}
