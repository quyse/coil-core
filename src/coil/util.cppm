module;

#include <string>
#include <string_view>
#include <tuple>

export module coil.core.util;

export namespace Coil
{
  // tuple with guaranteed order of initialization
  // (since std::tuple does not guarantee order)
  template <typename... T>
  struct Tuple;
  template <typename Head, typename... Tail>
  struct Tuple<Head, Tail...>
  {
    Head head;
    Tuple<Tail...> tail;

    template <size_t i>
    auto const& get() const
    {
      if constexpr(i == 0)
      {
        return head;
      }
      else
      {
        return tail.template get<i - 1>();
      }
    }
  };
  template <>
  struct Tuple<>
  {
  };
}

namespace std
{
  template <typename... T>
  struct tuple_size<Coil::Tuple<T...>>
  : public integral_constant<size_t, sizeof...(T)> {};

  template <typename Head, typename... Tail>
  struct tuple_element<0, Coil::Tuple<Head, Tail...>>
  {
    using type = Head;
  };
  template <size_t I, typename Head, typename... Tail>
  struct tuple_element<I, Coil::Tuple<Head, Tail...>>
  : public tuple_element<I - 1, Coil::Tuple<Tail...>> {};
}
