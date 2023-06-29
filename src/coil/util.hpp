#pragma once

#include <tuple>

namespace Coil
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


  // adaptable structs
#define COIL_ADAPT_STRUCT(name) \
  template <typename Adapter> \
  struct name : public Adapter::template Base<name>
#define COIL_ADAPT_STRUCT_MEMBER(type, name) \
  typename Adapter::template Member<type> name = this->template RegisterMember<type, []<typename Struct>() \
  { \
    return &Struct::name; \
  }>(#name)
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
