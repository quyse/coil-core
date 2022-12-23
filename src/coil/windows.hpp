#pragma once

#include "base.hpp"

#if !defined(___COIL_PLATFORM_WINDOWS)
  #error not windows
#endif

#define STRICT
#define NOMINMAX
#if !defined(UNICODE)
  #define UNICODE
  #define _UNICODE
#endif

#include <windows.h>
