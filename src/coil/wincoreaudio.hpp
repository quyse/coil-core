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
    WinCoreAudioDevice() = default;
    ~WinCoreAudioDevice();

    static WinCoreAudioDevice& Init(Book& book);

    void Append(std::unique_ptr<AudioInputStream>&& stream) override;

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

    std::thread _thread;
    // the following members are accessed by any thread
    std::mutex _mutex;
    std::queue<std::unique_ptr<AudioInputStream>> _streams;
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
