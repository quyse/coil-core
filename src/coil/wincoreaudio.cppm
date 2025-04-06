module;

#include "windows.hpp"
#include <audioclient.h>
#include <mmdeviceapi.h>
#include <thread>

export module coil.core.audio.wincoreaudio;

import coil.core.audio;
import coil.core.base;
import coil.core.windows;

export namespace Coil
{
  class WinCoreAudioDevice : public AudioDevice
  {
  public:
    WinCoreAudioDevice(AudioStream& stream)
    : _stream(stream) {}

    ~WinCoreAudioDevice()
    {
      Queue<&WinCoreAudioDevice::OnShutdown>();
      _thread.join();
    }

    static WinCoreAudioDevice& Init(Book& book, AudioStream& stream)
    {
      WinCoreAudioDevice& device = book.Allocate<WinCoreAudioDevice>(stream);

      device.Init();

      return device;
    }

    void SetPlaying(bool playing) override
    {
      if(_playing == playing) return;
      _playing = playing;
      if(_playing)
      {
        CheckHResult(_pAudioClient->Start(), "resuming MM device failed");
      }
      else
      {
        CheckHResult(_pAudioClient->Stop(), "stopping MM device failed");
      }
    }

  private:
    void Init()
    {
      _thread = std::thread(&WinCoreAudioDevice::ThreadFunc, this);
    }

    void ThreadFunc()
    {
      ComThread com(false);

      // enumerate devices
      ComPtr<IMMDeviceEnumerator> pDeviceEnumerator;
      CheckHResult(CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pDeviceEnumerator), "creating MM device enumerator failed");

      // get default device
      ComPtr<IMMDevice> pDevice;
      CheckHResult(pDeviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice), "getting default MM device failed");

      // activate device, get audio client
      CheckHResult(pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&_pAudioClient), "activating MM device failed");

      // get device format
      CoTaskMemPtr<WAVEFORMATEX> pFormat;
      CheckHResult(_pAudioClient->GetMixFormat(&pFormat), "getting mix format failed");
      _frameSize = (pFormat->wBitsPerSample / 8) * pFormat->nChannels;

      // initialize device
      CheckHResult(_pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, 0, 0, pFormat, NULL), "initializing audio client failed");

      // get audio render client
      CheckHResult(_pAudioClient->GetService(__uuidof(IAudioRenderClient), (void**)&_pAudioRenderClient), "getting audio render client failed");

      // get device buffer size
      CheckHResult(_pAudioClient->GetBufferSize(&_bufferFramesCount), "getting buffer size failed");

      // create and set event
      _hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
      if(!_hEvent)
        throw Exception("creating event failed");
      CheckHResult(_pAudioClient->SetEventHandle(_hEvent), "setting event handle failed");

      // start device
      CheckHResult(_pAudioClient->Start(), "starting audio client failed");

      while(_working)
      {
        switch(WaitForSingleObjectEx(_hEvent, INFINITE, TRUE))
        {
        case WAIT_IO_COMPLETION:
          // queued APC executed, nothing else to do
          break;
        case WAIT_OBJECT_0:
          // event signaled, send data
          Fill();
          break;
        default:
          throw Exception("unexpected audio event error");
        }
      }
    }

    template <void (WinCoreAudioDevice::*handler)()>
    void Queue()
    {
      UserAPCHandler<handler>::Queue(_thread, this);
    }

    void Fill()
    {
      uint32_t paddingFramesCount;
      CheckHResult(_pAudioClient->GetCurrentPadding(&paddingFramesCount), "getting current padding failed");

      uint32_t availableFramesCount = _bufferFramesCount - paddingFramesCount;
      size_t deviceBufferSize = availableFramesCount * _frameSize;

      // get device buffer
      uint8_t* deviceBuffer = nullptr;
      CheckHResult(_pAudioRenderClient->GetBuffer(availableFramesCount, &deviceBuffer), "getting device buffer failed");

      // fill it
      uint32_t writtenSize = 0;
      while(deviceBufferSize > 0)
      {
        // if current buffer is empty
        if(!_currentBuffer)
        {
          // read packet
          _currentBuffer = _stream.Read(deviceBufferSize / _frameSize);
        }

        // get data from current buffer
        size_t size = std::min(_currentBuffer.size, deviceBufferSize);
        if(!size) break;
        memcpy(deviceBuffer, _currentBuffer.data, size);
        deviceBuffer += size;
        deviceBufferSize -= size;
        _currentBuffer.data = (uint8_t*)_currentBuffer.data + size;
        _currentBuffer.size -= size;
        writtenSize += size;
      }

      // release device buffer
      CheckHResult(_pAudioRenderClient->ReleaseBuffer(writtenSize / _frameSize, 0), "releasing device buffer failed");
    }

    void OnShutdown()
    {
      _working = false;
    }

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
