module;

#include <pulse/pulseaudio.h>
#include <thread>

export module coil.core.audio.pulseaudio;

import coil.core.appidentity;
import coil.core.audio;
import coil.core.base;

export namespace Coil
{
  class PulseAudioDevice : public AudioDevice
  {
  public:
    PulseAudioDevice(AudioStream& outputStream)
    : _outputStream(outputStream) {}

    ~PulseAudioDevice()
    {
      _shutdown = true;
      _thread.join();

      if(_stream)
      {
        pa_stream_disconnect(_stream);
        pa_stream_unref(_stream);
      }
      if(_context)
      {
        pa_context_disconnect(_context);
        pa_context_unref(_context);
      }
      if(_mainloop)
      {
        pa_mainloop_free(_mainloop);
      }
    }

    static PulseAudioDevice& Init(Book& book, AudioStream& stream)
    {
      PulseAudioDevice& device = book.Allocate<PulseAudioDevice>(stream);
      device.Init();
      return device;
    }

    void SetPlaying(bool playing) override
    {
      // TODO
    }

  private:
    void Init()
    {
      _thread = std::thread(&PulseAudioDevice::ThreadFunc, this);
    }

    void ThreadFunc()
    {
      _mainloop = pa_mainloop_new();
      if(!_mainloop)
        throw Exception("creating PulseAudio mainloop failed");

      pa_mainloop_api* mainloop_api = pa_mainloop_get_api(_mainloop);

      _context = pa_context_new(mainloop_api, AppIdentity::GetInstance().Name().c_str());
      if(!_context)
        throw Exception("creating PulseAudio context failed");

      if(pa_context_connect(_context, nullptr, PA_CONTEXT_NOFLAGS, nullptr) < 0)
        throw Exception("connecting to PulseAudio failed");

      LoopUntil([&]()
      {
        pa_context_state_t state = pa_context_get_state(_context);
        if(!PA_CONTEXT_IS_GOOD(state))
          throw Exception("connecting to PulseAudio failed");

        return state == PA_CONTEXT_READY;
      });

      {
        pa_sample_spec sample_spec =
        {
          .format = std::endian::native == std::endian::little ? PA_SAMPLE_FLOAT32LE : PA_SAMPLE_FLOAT32BE,
          .rate = AudioFormat::recommendedSamplingRate,
          .channels = _channelsCount,
        };

        pa_channel_map channel_map;
        pa_channel_map_init_stereo(&channel_map);

        _stream = pa_stream_new(_context, "Playback", &sample_spec, &channel_map);
        if(!_stream)
          throw Exception("creating PulseAudio stream failed");
      }

      pa_stream_set_write_callback(_stream, &StaticStreamWriteCallback, this);

      if(pa_stream_connect_playback(_stream, nullptr, nullptr, PA_STREAM_NOFLAGS, nullptr, nullptr))
        throw Exception("connecting PulseAudio playback failed");

      LoopUntil([&]()
      {
        pa_stream_state_t state = pa_stream_get_state(_stream);
        if(!PA_STREAM_IS_GOOD(state))
          throw Exception("connecting PulseAudio playback failed");

        return state == PA_STREAM_READY;
      });

      // main loop
      LoopUntil([&]() -> bool
      {
        return _shutdown;
      });
    }

    template <typename F>
    void LoopUntil(F const& f)
    {
      for(;;)
      {
        if(pa_mainloop_iterate(_mainloop, 1, nullptr) < 0)
          throw Exception("iterating PulseAudio mainloop failed");

        if(f()) break;
      }
    }

    void StreamWriteCallback()
    {
      // get device buffer
      Buffer deviceBuffer;
      deviceBuffer.size = -1; // recommended, allows device to choose size automatically
      if(pa_stream_begin_write(_stream, &deviceBuffer.data, &deviceBuffer.size) || !deviceBuffer.data)
        throw Exception("beginning PulseAudio stream write failed");

      // get data
      uint8_t* deviceBufferPtr = (uint8_t*)deviceBuffer.data;
      size_t deviceBufferSize = deviceBuffer.size;
      size_t writtenSize = 0;
      while(deviceBufferSize > 0)
      {
        // if current buffer is empty
        if(!_currentBuffer)
        {
          // read packet
          _currentBuffer = _outputStream.Read((int32_t)(deviceBufferSize / _frameSize));
        }

        // get data from current buffer
        if(!_currentBuffer.size) break;
        size_t size = std::min(_currentBuffer.size, deviceBufferSize);
        memcpy(deviceBufferPtr, _currentBuffer.data, size);
        deviceBufferPtr += size;
        deviceBufferSize -= size;
        _currentBuffer.data = (uint8_t*)_currentBuffer.data + size;
        _currentBuffer.size -= size;
        writtenSize += size;
      }

      // stop thread if stream has ended
      if(!writtenSize) _shutdown = true;

      if(pa_stream_write(_stream, deviceBuffer.data, writtenSize, nullptr, 0, PA_SEEK_RELATIVE))
        throw Exception("writing to PulseAudio stream failed");
    }
    static void StaticStreamWriteCallback(pa_stream* stream, size_t size, void* userdata)
    {
      ((PulseAudioDevice*)userdata)->StreamWriteCallback();
    }

    AudioStream& _outputStream;
    std::thread _thread;
    std::atomic<bool> _shutdown = false;
    Buffer _currentBuffer;

    pa_mainloop* _mainloop = nullptr;
    pa_context* _context = nullptr;
    pa_stream* _stream = nullptr;

    // fixed channels count
    static constexpr uint32_t const _channelsCount = 2;
    static constexpr size_t const _frameSize = _channelsCount * sizeof(AudioSample);
  };
}
