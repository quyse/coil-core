#pragma once

#include "base.hpp"
#include <vector>
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

  // run process, do not wait for it
  void RunProcessAndForget(std::string const& program, std::vector<std::string> const& arguments);

  // run executable or open a document
  void RunOrOpenFile(std::string const& fileName);
}
