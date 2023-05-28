#include "entrypoint.hpp"
#include "steam.hpp"
#include <iostream>

using namespace Coil;

int COIL_ENTRY_POINT(std::vector<std::string>&& args)
{
  auto& steam = Steam::Get();

  return 0;
}
