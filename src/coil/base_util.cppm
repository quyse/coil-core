module;

#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>

export module coil.core.base.util;

namespace Coil
{
  template <typename T>
  struct ConstRefExceptScalarOfHelper
  {
    using Type = T const&;
  };
  template <typename T>
  requires std::is_scalar_v<T>
  struct ConstRefExceptScalarOfHelper<T>
  {
    using Type = T;
  };
}

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
    decltype(auto) get(this auto& self)
    {
      if constexpr(i == 0)
      {
        return self.head;
      }
      else
      {
        return self.tail.template get<i - 1>();
      }
    }
  };
  template <>
  struct Tuple<>
  {
  };

  template <typename T>
  concept IsDecayed = std::same_as<T, std::decay_t<T>>;

  template <typename P, typename Q>
  concept IsSameDecayed = std::same_as<std::decay_t<P>, Q>;

  template <typename T>
  using ConstRefExceptScalarOf = typename ConstRefExceptScalarOfHelper<T>::Type;
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
