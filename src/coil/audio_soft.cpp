#include "audio_soft.hpp"
#include <cstring>

namespace Coil
{
  AudioPausingStream::AudioPausingStream(AudioStream& stream, bool playing)
  : _stream(stream), _playing(playing), _format(_stream.GetFormat()) {}

  void AudioPausingStream::SetPlaying(bool playing)
  {
    _playing = playing;
  }

  AudioFormat AudioPausingStream::GetFormat() const
  {
    return _format;
  }

  Buffer AudioPausingStream::Read(int32_t framesCount)
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
      return Buffer(silence, std::min(framesCount, silenceSamplesCount / channelsCount) * channelsCount * sizeof(AudioSample));
    }
  }

  AudioVolumeStream::AudioVolumeStream(AudioStream& stream)
  : _stream(stream), _format(_stream.GetFormat()) {}

  void AudioVolumeStream::SetVolume(float volume)
  {
    _volume = volume;
  }

  AudioFormat AudioVolumeStream::GetFormat() const
  {
    return _format;
  }

  Buffer AudioVolumeStream::Read(int32_t framesCount)
  {
    Buffer buffer = _stream.Read(framesCount);
    _buffer.resize(buffer.size);
    for(size_t i = 0; i < buffer.size; i += sizeof(AudioSample))
      *(AudioSample*)(_buffer.data() + i) = *(AudioSample*)((uint8_t const*)buffer.data + i) * _volume;
    return _buffer;
  }

  AudioMixerStream::Player::Player(Book&& book, AudioStream& stream)
  : _book(std::move(book)), _stream(stream) {}

  AudioMixerStream::AudioMixerStream(AudioFormat const& format)
  : _format(format) {}

  void AudioMixerStream::Player::Stop()
  {
    _playing = false;
  }

  std::shared_ptr<AudioMixerStream::Player> AudioMixerStream::Play(Book&& book, AudioStream& stream)
  {
    if(stream.GetFormat() != _format)
      throw Exception("wrong stream format to play in audio mixer stream");

    std::shared_ptr<Player> player = std::make_shared<Player>(std::move(book), stream);

    std::scoped_lock lock(_mutex);
    _players.insert(player);

    return player;
  }

  AudioFormat AudioMixerStream::GetFormat() const
  {
    return _format;
  }

  Buffer AudioMixerStream::Read(int32_t framesCount)
  {
    std::scoped_lock lock(_mutex);

    // read from players which do not have data yet
    size_t minimumSize = 0;
    for(auto i = _players.begin(); i != _players.end(); )
    {
      auto& player = *i;
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
    for(auto& player : _players)
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
}
