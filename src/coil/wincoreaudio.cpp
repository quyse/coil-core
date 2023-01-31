#include "wincoreaudio.hpp"
#include <audioclient.h>
#include <mmdeviceapi.h>

namespace Coil
{
  WinCoreAudioDevice::WinCoreAudioDevice(std::unique_ptr<AudioStream>&& stream)
  : _stream(std::move(stream)) {}

  WinCoreAudioDevice::~WinCoreAudioDevice()
  {
    Queue<&WinCoreAudioDevice::OnShutdown>();
    _thread.join();
  }

  WinCoreAudioDevice& WinCoreAudioDevice::Init(Book& book, std::unique_ptr<AudioStream>&& stream)
  {
    WinCoreAudioDevice& device = book.Allocate<WinCoreAudioDevice>(std::move(stream));

    device.Init();

    return device;
  }

  void WinCoreAudioDevice::SetPlaying(bool playing)
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

  void WinCoreAudioDevice::Init()
  {
    _thread = std::thread(&WinCoreAudioDevice::ThreadFunc, this);
  }

  void WinCoreAudioDevice::ThreadFunc()
  {
    ComThread com(false);

    // enumerate devices
    ComPtr<IMMDeviceEnumerator> pDeviceEnumerator;
    CheckHResult(CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&pDeviceEnumerator), "creating MM device enumerator failed");

    // get default device
    ComPtr<IMMDevice> pDevice;
    CheckHResult(pDeviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice), "getting default MM device failed");

    // activate device, get audio client
    CheckHResult(pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&_pAudioClient), "activating MM device failed");

    // get device format
    CoTaskMemPtr<WAVEFORMATEX> pFormat;
    CheckHResult(_pAudioClient->GetMixFormat(&pFormat), "getting mix format failed");
    _frameSize = (pFormat->wBitsPerSample / 8) * pFormat->nChannels;

    // initialize device
    CheckHResult(_pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, 0, 0, pFormat, NULL), "initializing audio client failed");

    // get audio render client
    CheckHResult(_pAudioClient->GetService(IID_IAudioRenderClient, (void**)&_pAudioRenderClient), "getting audio render client failed");

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

  void WinCoreAudioDevice::Fill()
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
        _currentBuffer = _stream->Read(deviceBufferSize / _frameSize);
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

  void WinCoreAudioDevice::OnShutdown()
  {
    _working = false;
  }
}
