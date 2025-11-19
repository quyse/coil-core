#pragma once

#include <utility>

// pointer-to-member: returns pointer-to-member with specified name of any object
#define COIL_META_MEMBER_PTR(name) \
  ([]<typename Object>() \
  { \
    return &Object::name; \
  })
// field accessor: returns object which can get a field with specified name of any object
#define COIL_META_MEMBER(name) \
  ([](auto& object) -> decltype(auto) \
  { \
    return object.name; \
  })
// method call: returns object which can call a method with specified name of any object
#define COIL_META_CALL_METHOD(name) \
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
    this->template RegisterField<type, COIL_META_MEMBER_PTR(name), label>()
// meta struct field
#define COIL_META_STRUCT_FIELD(type, name) COIL_META_STRUCT_FIELD_LABEL(type, name, #name)
