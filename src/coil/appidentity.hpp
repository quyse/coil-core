#pragma once

#include <string>

namespace Coil
{
  // global app identity
  class AppIdentity
  {
  public:
    // app name
    static std::string name;
    // app version
    static uint32_t version;
  };
}
