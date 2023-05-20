#pragma once

#include "base.hpp"
#include <functional>
#include <vector>
#include <string>

namespace Coil
{
  extern std::function<int(std::vector<std::string>&&)> const g_entryPoint;
}

#define ___COIL_ENTRY_POINT std::function<int(std::vector<std::string>&&)> const Coil::g_entryPoint
