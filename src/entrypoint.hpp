#pragma once

#include "base.hpp"
#include <vector>
#include <string>

namespace Coil
{
  extern std::function<int(std::vector<std::string>&&)> g_entryPoint;
}

#define ___COIL_ENTRY_POINT std::function<int(std::vector<std::string>&&)> Coil::g_entryPoint
