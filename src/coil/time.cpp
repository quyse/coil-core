#include "time.hpp"
#if defined(___COIL_PLATFORM_WINDOWS)
#include "windows.hpp"
#elif defined(___COIL_PLATFORM_MACOS)
#include <mach/mach_time.h>
#else
#include <ctime>
#endif

namespace Coil
{
  Time::Tick const Time::ticksPerSecond = []()
  {
#if defined(___COIL_PLATFORM_WINDOWS)
    LARGE_INTEGER ticks;
    QueryPerformanceFrequency(&ticks);
    return ticks.QuadPart;
#elif defined(___COIL_PLATFORM_MACOS)
    mach_timebase_info_data_t timebase;
    mach_timebase_info(&timebase);
    return 1000000000ULL * timebase.numer / timebase.denom;
#else
    return 1000000000ULL;
#endif
  }();

  float const Time::secondsPerTick = 1.0f / Time::ticksPerSecond;

  Time::Tick Time::GetTick()
  {
#if defined(___COIL_PLATFORM_WINDOWS)
    LARGE_INTEGER tick;
    QueryPerformanceCounter(&tick);
    return tick.QuadPart;
#elif defined(___COIL_PLATFORM_MACOS)
    return mach_absolute_time();
#else
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1000000000ULL + t.tv_nsec;
#endif
  }

  Timer::Timer()
  : _lastTick(0), _pauseTick(0) {}

  void Timer::Pause()
  {
    // if already paused, do nothing
    if(_pauseTick)
      return;

    // remember pause time
    _pauseTick = Time::GetTick();
  }

  float Timer::Tick()
  {
    // get current time
    Time::Tick currentTick = Time::GetTick();

    // calculate amount of ticks from last tick
    Time::Tick ticks = 0;
    if(_lastTick)
    {
      ticks = (_pauseTick ? _pauseTick : currentTick) - _lastTick;
    }

    // if was paused, resume
    if(_pauseTick)
      _pauseTick = 0;

    // remember current tick as last tick
    _lastTick = currentTick;

    return ticks * Time::secondsPerTick;
  }
}
