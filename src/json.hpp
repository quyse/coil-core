#pragma once

#include "math.hpp"
#include <vector>
#include <nlohmann/json.hpp>

namespace Coil
{
  using json = nlohmann::json;

  template <typename T>
  struct JsonParser;

  // some base methods
  template <typename T>
  struct JsonParserBase
  {
    template <typename Key>
    static T ParseField(json const& j, Key const& key)
    {
      if(!j.is_object())
        throw Exception() << "parsing " << typeid(T).name() << ", JSON field is not an object";
      auto i = j.find(key);
      if(i == j.end())
        throw Exception() << "parsing " << typeid(T).name() << ", missing JSON field " << key;
      return JsonParser<T>::Parse(*i);
    }

    template <typename Key>
    static T ParseField(json const& j, Key const& key, T&& defaultValue)
    {
      if(!j.is_object())
        throw Exception() << "parsing " << typeid(T).name() << ", JSON field is not an object";
      auto i = j.find(key);
      if(i == j.end())
        return std::move(defaultValue);
      return JsonParser<T>::Parse(*i);
    }
  };

  // parser for various types from json
  // struct so we can use partial specialization
  template <typename T>
  struct JsonParser : public JsonParserBase<T>
  {
    static T Parse(json const& j)
    {
      if(j.is_null())
        throw Exception() << "parsing " << typeid(T).name() << ", got null";
      return j.get<T>();
    }
  };

  template <typename T, size_t n>
  struct JsonParser<xvec<T, n>> : public JsonParserBase<xvec<T, n>>
  {
    static xvec<T, n> Parse(json const& j)
    {
      if(!j.is_array() || j.size() != n)
        throw Exception() << "parsing " << typeid(xvec<T, n>).name() << ", expected JSON array of " << n << " " << typeid(T).name() << " but got: " << j;
      xvec<T, n> r;
      for(size_t i = 0; i < n; ++i)
        r(i) = JsonParser<T>::Parse(j.at(i));
      return r;
    }
  };

  template <typename T>
  struct JsonParser<xquat<T>> : public JsonParserBase<xquat<T>>
  {
    static xquat<T> Parse(json const& j)
    {
      if(!j.is_array() || j.size() != 4)
        throw Exception() << "parsing " << typeid(xquat<T>).name() << ", expected JSON array of 4 " << typeid(T).name() << " but got: " << j;
      xquat<T> r;
      for(size_t i = 0; i < 4; ++i)
        r(i) = JsonParser<T>::Parse(j.at(i));
      return r;
    }
  };

  template <typename T>
  struct JsonParser<std::optional<T>> : public JsonParserBase<std::optional<T>>
  {
    static std::optional<T> Parse(json const& j)
    {
      if(j.is_null()) return {};
      return JsonParser<T>::Parse(j);
    }
  };

  template <typename T>
  struct JsonParser<std::vector<T>> : public JsonParserBase<std::vector<T>>
  {
    static std::vector<T> Parse(json const& j)
    {
      if(!j.is_array())
        throw Exception() << "parsing " << typeid(std::vector<T>).name() << ", expected JSON array of " << typeid(T).name() << " but got: " << j;
      std::vector<T> r;
      r.reserve(j.size());
      for(size_t i = 0; i < j.size(); ++i)
        r.push_back(JsonParser<T>::Parse(j.at(i)));
      return r;
    }
  };
}
