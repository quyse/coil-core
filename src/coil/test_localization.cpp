#include "entrypoint.hpp"
#include <iostream>

import coil.core.localization;
import localized;

using namespace Coil;
using namespace Localized;

int COIL_ENTRY_POINT(std::vector<std::string> args)
{
  uint64_t p[] = { 0, 1, 2, 3, 4, 5, 10, 11, 15, 21, 22, 25 };
  for(size_t i = 0; i < sizeof(Localized::Sets) / sizeof(Localized::Sets[0]); ++i)
  {
    Localized::Set::Current = Localized::Sets[i];
    for(size_t j = 0; j < sizeof(p) / sizeof(p[0]); ++j)
    {
      "stuff_remaining"_loc(std::cout, p[j], p[j]);
      std::cout << "\n";
    }
    for(size_t j = 0; j < sizeof(p) / sizeof(p[0]); ++j)
    {
      "numbers"_loc(std::cout, p[j], p[j] + 1, p[j] + 2);
      std::cout << "\n";
    }
  }

  return 0;
}
