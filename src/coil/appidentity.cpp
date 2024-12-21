#include "appidentity.hpp"

namespace Coil
{
  AppIdentity::AppIdentity()
  : name_{"App"}, packageName_{"app"} {}

  std::string& AppIdentity::Name()
  {
    return name_;
  }

  std::string& AppIdentity::PackageName()
  {
    return packageName_;
  }

  uint32_t& AppIdentity::Version()
  {
    return version_;
  }

  AppIdentity& AppIdentity::GetInstance()
  {
    static AppIdentity instance;
    return instance;
  }
}
