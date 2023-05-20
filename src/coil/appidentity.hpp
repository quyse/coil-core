#pragma once

#include <string>

namespace Coil
{
  // global app identity
  class AppIdentity
  {
  public:
    AppIdentity();

    std::string& Name();
    uint32_t& Version();

    static AppIdentity& GetInstance();

  private:
    // app name
    std::string _name;
    // app version
    uint32_t _version;
  };
}
