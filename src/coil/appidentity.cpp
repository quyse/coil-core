#include "appidentity.hpp"

namespace Coil
{
  AppIdentity::AppIdentity()
  : _name("Coil Core App"), _version(0) {}

  std::string& AppIdentity::Name()
  {
    return _name;
  }

  uint32_t& AppIdentity::Version()
  {
    return _version;
  }

  AppIdentity& AppIdentity::GetInstance()
  {
    static AppIdentity instance;
    return instance;
  }
}
