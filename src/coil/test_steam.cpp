#include "entrypoint.hpp"
#include "steam.hpp"
#include <iostream>

using namespace Coil;

___COIL_ENTRY_POINT = [](std::vector<std::string>&& args) -> int
{
  auto& steam = Steam::Get();

  return 0;
};
