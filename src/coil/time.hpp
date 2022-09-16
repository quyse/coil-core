#pragma once

#include "base.hpp"

namespace Coil
{
  class Time
  {
  public:
    using Tick = uint64_t;

    static Tick GetTick();
    static Tick const ticksPerSecond;
    static float const secondsPerTick;
  };

  class Timer
  {
  public:
    Timer();

    void Pause();
    // time since last tick
    float Tick();

  private:
    Time::Tick _lastTick;
    Time::Tick _pauseTick;
  };
}
