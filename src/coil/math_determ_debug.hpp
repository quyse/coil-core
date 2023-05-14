#pragma once

#include "math_determ.hpp"
#include <ostream>

namespace Coil
{
  template <typename T, size_t n>
  std::ostream& operator<<(std::ostream& s, xvec_d<T, n> const& a)
  {
    if constexpr (n == 1)
    {
      return s << a.t[0];
    }
    else
    {
      s << "xvec_d<" << typeid(T).name() << ", " << n << ">{ ";
      for(size_t i = 0; i < n; ++i)
      {
        if(i) s << ", ";
        s << a.t[i];
      }
      return s << " }";
    }
  }
}
