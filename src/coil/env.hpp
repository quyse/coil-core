#pragma once

#include "base.hpp"
#include <string>

namespace Coil
{
  enum class AppKnownLocation
  {
    Config,
    Data,
    State,
  };

  // get known location
  std::string GetAppKnownLocation(AppKnownLocation location);
  // make known location directory if necessary and return it
  std::string EnsureAppKnownLocation(AppKnownLocation location);
}
