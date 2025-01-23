#pragma once

#include "base.hpp"
#include <thread>

#if !defined(COIL_PLATFORM_WINDOWS)
  #error not windows
#endif

// windows fuckery
#define STRICT
#define NOMINMAX
#if !defined(UNICODE)
  #define UNICODE
  #define _UNICODE
#endif

// prevent inclusion of winsock 1.0 (breaks winsock 2.0)
#define _WINSOCKAPI_

#include <windows.h>

#undef _WINSOCKAPI_

// auto-linking COM
#include <initguid.h>

// undef unpleasant macros
#undef CreateWindow
#undef GetMessage
