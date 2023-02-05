#pragma once

#include "audio.hpp"
#include <memory>
#include <atomic>
#include <unordered_set>
#include <mutex>

namespace Coil
{
  // stream which can be paused
  // returns silence while paused
  class AudioPausingStream : public AudioStream
  {
  public:
    AudioPausingStream(AudioStream& stream, bool playing = true);

    void SetPlaying(bool playing);

    AudioFormat GetFormat() const override;
    Buffer Read(int32_t framesCount) override;

  private:
    AudioStream& _stream;
    std::atomic<bool> _playing = false;
    AudioFormat _format;
  };

  // volume filter
  class AudioVolumeStream : public AudioStream
  {
  public:
    AudioVolumeStream(AudioStream& stream);

    void SetVolume(float volume);

    AudioFormat GetFormat() const override;
    Buffer Read(int32_t framesCount) override;

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
      Player(Book&& book, AudioStream& stream);

      // stop and remove player from mixer
      // if player is already removed or simply reached end, do nothing
      void Stop();

    private:
      Book _book;
      AudioStream& _stream;
      std::atomic<bool> _playing = true;
      // access synchronized by mutex in AudioMixerStream
      Buffer _buffer;

      friend class AudioMixerStream;
    };
  public:
    AudioMixerStream(AudioFormat const& format);

    // play input stream from the current moment
    std::shared_ptr<Player> Play(Book&& book, AudioStream& stream);

    AudioFormat GetFormat() const override;
    Buffer Read(int32_t framesCount) override;

  private:
    AudioFormat const _format;
    std::mutex _mutex;
    std::unordered_set<std::shared_ptr<Player>> _players;
    std::vector<uint8_t> _buffer;
  };
}
