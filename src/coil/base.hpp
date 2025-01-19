#pragma once

// platform definitions

#if defined(_WIN32) || defined(__WIN32__)
  #define COIL_PLATFORM_WINDOWS
#elif defined(__APPLE__) && defined(__MACH__)
  #define COIL_PLATFORM_MACOS
  #define COIL_PLATFORM_POSIX
#else
  #define COIL_PLATFORM_LINUX
  #define COIL_PLATFORM_POSIX
#endif
