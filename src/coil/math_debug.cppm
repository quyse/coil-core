module;

#include <ostream>

export module coil.core.math.debug;

import coil.core.math;

export namespace Coil
{
  template <typename T, size_t n, MathOptions o>
  std::ostream& operator<<(std::ostream& s, xvec<T, n, o> const& a)
  {
    s << "xvec<" << typeid(T).name() << ", " << n << ">{ ";
    for(size_t i = 0; i < n; ++i)
    {
      if(i) s << ", ";
      s << a(i);
    }
    return s << " }";
  }

  template <typename T, size_t n, size_t m>
  std::ostream& operator<<(std::ostream& s, xmat<T, n, m> const& a)
  {
    s << "xmat<" << typeid(T).name() << ", " << n << ", " << m << ">{\n";
    for(size_t i = 0; i < n; ++i)
    {
      for(size_t j = 0; j < m; ++j)
        s << a(i, j) << ",\t";
      s << '\n';
    }
    return s << "}\n";
  }
}
