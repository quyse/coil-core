#pragma once

#include "base.hpp"
#include <memory>

namespace Coil
{
  class AudioInputStream
  {
  public:
    virtual ~AudioInputStream() {}

    // read one packet, empty buffer if EOF
    // buffer can be valid only until next read
    virtual Buffer Read() = 0;
  };

  class AudioDevice
  {
  public:
    // append audio stream to device queue
    // starts immediately if queue is empty
    virtual void Append(std::unique_ptr<AudioInputStream>&& stream) = 0;
  };
}
