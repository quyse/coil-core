module;

#include <atomic>
#include <memory>
#include <mutex>
#include <unordered_set>
#include <vector>

export module coil.core.audio.soft;

import coil.core.audio;
import coil.core.base;

export namespace Coil
{
  // stream which can be paused
  // returns silence while paused
  class AudioPausingStream : public AudioStream
  {
  public:
    AudioPausingStream(AudioStream& stream, bool playing = true)
    : _stream(stream), _playing(playing), _format(_stream.GetFormat()) {}

    void SetPlaying(bool playing)
    {
      _playing = playing;
    }

    AudioFormat GetFormat() const override
    {
      return _format;
    }

    Buffer Read(int32_t framesCount) override
    {
      if(_playing)
      {
        return _stream.Read(framesCount);
      }
      else
      {
        static constexpr int32_t silenceSamplesCount = 0x1000;
        static constinit AudioSample const silence[silenceSamplesCount] = {};
        int32_t channelsCount = GetAudioFormatChannelsCount(_format.channels);
        return { silence, std::min(framesCount, silenceSamplesCount / channelsCount) * channelsCount * sizeof(AudioSample) };
      }
    }

  private:
    AudioStream& _stream;
    std::atomic<bool> _playing = false;
    AudioFormat _format;
  };

  // volume filter
  class AudioVolumeStream : public AudioStream
  {
  public:
    AudioVolumeStream(AudioStream& stream)
    : _stream(stream), _format(_stream.GetFormat()) {}

    void SetVolume(float volume)
    {
      _volume = volume;
    }

    AudioFormat GetFormat() const override
    {
      return _format;
    }

    Buffer Read(int32_t framesCount) override
    {
      Buffer buffer = _stream.Read(framesCount);
      _buffer.resize(buffer.size);
      for(size_t i = 0; i < buffer.size; i += sizeof(AudioSample))
        *(AudioSample*)(_buffer.data() + i) = *(AudioSample*)((uint8_t const*)buffer.data + i) * _volume;
      return _buffer;
    }

  private:
    AudioStream& _stream;
    AudioFormat _format;
    float _volume = 1.0f;
    std::vector<uint8_t> _buffer;
  };

  // mixer stream
  // allows to add streams to mix in real time
  // streams' formats must be equal to mixer's
  class AudioMixerStream : public AudioStream
  {
  public:
    class Player
    {
    public:
      Player(Book&& book, AudioStream& stream)
      : _book(std::move(book)), _stream(stream) {}

      // stop and remove player from mixer
      // if player is already removed or simply reached end, do nothing
      void Stop()
      {
        _playing = false;
      }

    private:
      Book _book;
      AudioStream& _stream;
      std::atomic<bool> _playing = true;
      // access synchronized by mutex in AudioMixerStream
      Buffer _buffer;

      friend class AudioMixerStream;
    };
  public:
    AudioMixerStream(AudioFormat const& format)
    : _format(format) {}

    // play input stream from the current moment
    std::shared_ptr<Player> Play(Book&& book, AudioStream& stream)
    {
      if(stream.GetFormat() != _format)
        throw Exception("wrong stream format to play in audio mixer stream");

      std::shared_ptr<Player> player = std::make_shared<Player>(std::move(book), stream);

      std::scoped_lock lock(_mutex);
      _players.insert(player);

      return player;
    }

    AudioFormat GetFormat() const override
    {
      return _format;
    }

    Buffer Read(int32_t framesCount) override
    {
      std::scoped_lock lock(_mutex);

      // read from players which do not have data yet
      size_t minimumSize = 0;
      for(auto i = _players.begin(); i != _players.end(); )
      {
        auto const& player = *i;
        // read from stream if it's not cancelled, and there's no existing data
        if(player->_playing && !player->_buffer.size)
        {
          // read data
          player->_buffer = player->_stream.Read(framesCount);
        }
        // if stream is cancelled, or end of sound is reached, remove player
        if(!player->_playing || !player->_buffer.size)
        {
          i = _players.erase(i);
          continue;
        }

        if(!minimumSize || minimumSize > player->_buffer.size)
        {
          minimumSize = player->_buffer.size;
        }

        ++i;
      }

      // sum up all samples up to the minimum
      _buffer.assign(minimumSize, 0);
      for(auto const& player : _players)
      {
        auto& playerBuffer = player->_buffer;
        for(size_t i = 0; i < minimumSize; i += sizeof(AudioSample))
        {
          *(AudioSample*)(_buffer.data() + i) += *(AudioSample*)((uint8_t*)playerBuffer.data + i);
        }
        playerBuffer.size -= minimumSize;
        std::memmove(playerBuffer.data, (uint8_t const*)playerBuffer.data + minimumSize, playerBuffer.size);
      }

      return _buffer;
    }

  private:
    AudioFormat const _format;
    std::mutex _mutex;
    std::unordered_set<std::shared_ptr<Player>> _players;
    std::vector<uint8_t> _buffer;
  };
}
