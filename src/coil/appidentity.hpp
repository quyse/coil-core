#pragma once

#include <string>
#include <cstdint>

namespace Coil
{
  // global app identity
  class AppIdentity
  {
  public:
    AppIdentity();

    // display name for app
    std::string& Name();
    // package name for use in directory names, etc
    // suggestion: use reverse-domain notation, i.e. "com.company.app"
    std::string& PackageName();
    // app version
    uint32_t& Version();

    static AppIdentity& GetInstance();

  private:
    std::string name_;
    std::string packageName_;
    uint32_t version_ = 0;
  };
}
