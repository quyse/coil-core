module;

#include <optional>
#include <ostream>
#include <vector>

export module coil.core.base.debug;

namespace std
{
  template <typename T>
  std::ostream& operator<<(std::ostream& s, std::optional<T> const& a)
  {
    s << "std::optional<" << typeid(T).name() << ">{ ";
    if(a.has_value()) s << a.value();
    return s << " }";
  }

  template <typename T>
  std::ostream& operator<<(std::ostream& s, std::vector<T> const& a)
  {
    s << "std::vector<" << typeid(T).name() << ">{ ";
    for(size_t i = 0; i < a.size(); ++i)
    {
      if(i) s << ", ";
      s << a[i];
    }
    return s << " }";
  }
}
