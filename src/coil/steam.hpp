#pragma once

#include "base.hpp"

namespace Coil
{
  class Steam
  {
  public:
    static Steam& Get();

  private:
    Steam();
    ~Steam();
  };
}
