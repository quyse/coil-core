#pragma once

#include <string>
#include <string_view>
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


  // pointer-to-member: returns pointer-to-member with specified name of any object
  #define COIL_META_MEMBER_PTR(name) \
    ([]<typename Object>() \
    { \
      return &Object::name; \
    })
  // field accessor: returns object which can get a field with specified name of any object
  #define COIL_META_FIELD(name) \
    ([]<typename Object>(Object&& object) -> auto&& \
    { \
      return std::forward<Object>(object).name; \
    })
  // method call: returns object which can call a method with specified name of any object
  #define COIL_META_METHOD(name) \
    ([]<typename Object, typename... Args>(Object&& object, Args&&... args) -> decltype(auto) \
    { \
      return std::forward<Object>(object).name(std::forward<Args>(args)...); \
    })

  // meta struct
  #define COIL_META_STRUCT(name) \
    template <typename Adapter> \
    struct name : public Adapter::template Base<name>
  // meta struct field with custom label
  #define COIL_META_STRUCT_FIELD_LABEL(type, name, label) \
    typename Adapter::template Field<type> name = \
      this->template RegisterField<type, COIL_META_MEMBER_PTR(name)>(label)
  // meta struct field
  #define COIL_META_STRUCT_FIELD(type, name) COIL_META_STRUCT_FIELD_LABEL(type, name, #name)


  // serialization to/from string, to be specialized
  template <typename T>
  std::string ToString(T const& value);
  template <typename T>
  T FromString(std::string_view const& value);
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
