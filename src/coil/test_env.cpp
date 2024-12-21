#include "appidentity.hpp"
#include "entrypoint.hpp"
#include "env.hpp"
#include <iostream>
using namespace Coil;

int COIL_ENTRY_POINT(std::vector<std::string> args)
{
  AppIdentity::GetInstance().Name() = "test_env";
  AppIdentity::GetInstance().PackageName() = "coil.core.test_env";

  std::cout
    << "config: " << GetAppKnownLocation(AppKnownLocation::Config) << "\n"
    << "data: " << GetAppKnownLocation(AppKnownLocation::Data) << "\n"
    << "state: " << GetAppKnownLocation(AppKnownLocation::State) << "\n"
  ;

  return 0;
}
