#include "entrypoint.hpp"
#include <iostream>

import coil.core.appidentity;
import coil.core.process;

using namespace Coil;

int COIL_ENTRY_POINT(std::vector<std::string> args)
{
  AppIdentity::GetInstance().Name() = "test_process";
  AppIdentity::GetInstance().PackageName() = "coil.core.test_process";

  std::cout
    << "config: " << GetAppKnownLocation(AppKnownLocation::Config) << "\n"
    << "data: " << GetAppKnownLocation(AppKnownLocation::Data) << "\n"
    << "state: " << GetAppKnownLocation(AppKnownLocation::State) << "\n"
  ;

  return 0;
}
