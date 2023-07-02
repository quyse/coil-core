#pragma once

#include "math.hpp"
#include <concepts>
#include <optional>
#include <unordered_map>
#include <vector>
#include <nlohmann/json.hpp>

namespace Coil
{
  using json = nlohmann::json;

  template <typename T>
  struct JsonDecoder;

  // convenience functions
  template <typename T>
  T JsonDecode(json const& j)
  {
    return JsonDecoder<T>::Decode(j);
  }
  template <typename T, typename Key>
  T JsonDecodeField(json const& j, Key const& key)
  {
    return JsonDecoder<T>::template DecodeField<Key>(j, key);
  }
  template <typename T, typename Key>
  T JsonDecodeField(json const& j, Key const& key, T&& defaultValue)
  {
    return JsonDecoder<T>::template DecodeField<Key>(j, key, std::move(defaultValue));
  }

  // some base methods
  template <typename T>
  struct JsonDecoderBase
  {
    template <typename Key>
    static T DecodeField(json const& j, Key const& key)
    {
      if(!j.is_object())
        throw Exception() << "decoding " << typeid(T).name() << ", JSON field is not an object";
      auto i = j.find(key);
      if(i == j.end())
        throw Exception() << "decoding " << typeid(T).name() << ", missing JSON field " << key;
      return JsonDecode<T>(*i);
    }

    template <typename Key>
    static T DecodeField(json const& j, Key const& key, T&& defaultValue)
    {
      if(!j.is_object())
        throw Exception() << "decoding " << typeid(T).name() << ", JSON field is not an object";
      auto i = j.find(key);
      if(i == j.end())
        return std::move(defaultValue);
      return JsonDecode<T>(*i);
    }
  };

  // decoder for various types from json
  // struct so we can use partial specialization
  template <typename T>
  struct JsonDecoder;

  // specialization for types already implemented in nlohmann library
  template <typename T>
  requires requires(json const& j) { j.get<T>(); }
  struct JsonDecoder<T> : public JsonDecoderBase<T>
  {
    static T Decode(json const& j)
    {
      if(j.is_null())
        throw Exception() << "decoding " << typeid(T).name() << ", got null";
      return j.get<T>();
    }
  };

  // encoder for various types to json
  // struct so we can use partial specialization
  template <typename T>
  struct JsonEncoder;

  // specialization for types already implemented in nlohmann library
  template <typename T>
  requires std::constructible_from<json, T const&>
  struct JsonEncoder<T>
  {
    static json Encode(T const& v)
    {
      return v;
    }
  };

  // convenience function
  template <typename T>
  json JsonEncode(T const& v)
  {
    return JsonEncoder<T>::Encode(v);
  }

  template <typename T, size_t n, MathOptions o>
  struct JsonDecoder<xvec<T, n, o>> : public JsonDecoderBase<xvec<T, n, o>>
  {
    static xvec<T, n, o> Decode(json const& j)
    {
      if(!j.is_array() || j.size() != n)
        throw Exception() << "decoding " << typeid(xvec<T, n, o>).name() << ", expected JSON array of " << n << " " << typeid(T).name() << " but got: " << j;
      xvec<T, n, o> r;
      for(size_t i = 0; i < n; ++i)
        r(i) = JsonDecode<T>(j.at(i));
      return r;
    }
  };
  template <typename T, size_t n, MathOptions o>
  struct JsonEncoder<xvec<T, n, o>>
  {
    static json Encode(xvec<T, n, o> const& v)
    {
      json t[n];
      for(size_t i = 0; i < n; ++i)
        t[i] = JsonEncode<T>(v.t[i]);
      return t;
    }
  };

  template <typename T, MathOptions o>
  struct JsonDecoder<xquat<T, o>> : public JsonDecoderBase<xquat<T, o>>
  {
    static xquat<T, o> Decode(json const& j)
    {
      if(!j.is_array() || j.size() != 4)
        throw Exception() << "decoding " << typeid(xquat<T, o>).name() << ", expected JSON array of 4 " << typeid(T).name() << " but got: " << j;
      xquat<T, o> r;
      for(size_t i = 0; i < 4; ++i)
        r(i) = JsonDecode<T>(j.at(i));
      return r;
    }
  };
  template <typename T, MathOptions o>
  struct JsonEncoder<xquat<T, o>>
  {
    static json Encode(xquat<T, o> const& v)
    {
      return Encode<xvec<T, 4, o>>(v);
    }
  };

  template <typename T>
  struct JsonDecoder<std::optional<T>> : public JsonDecoderBase<std::optional<T>>
  {
    static std::optional<T> Decode(json const& j)
    {
      if(j.is_null()) return {};
      return JsonDecode<T>(j);
    }
  };
  template <typename T>
  struct JsonEncoder<std::optional<T>>
  {
    static json Encode(std::optional<T> const& v)
    {
      return v.has_value() ? JsonEncode<T>(v.value()) : json(nullptr);
    }
  };

  template <typename T>
  struct JsonDecoder<std::vector<T>> : public JsonDecoderBase<std::vector<T>>
  {
    static std::vector<T> Decode(json const& j)
    {
      if(!j.is_array())
        throw Exception() << "decoding " << typeid(std::vector<T>).name() << ", expected JSON array of " << typeid(T).name() << " but got: " << j;
      std::vector<T> r;
      r.reserve(j.size());
      for(size_t i = 0; i < j.size(); ++i)
        r.push_back(JsonDecode<T>(j.at(i)));
      return r;
    }
  };
  template <typename T>
  struct JsonEncoder<std::vector<T>>
  {
    static json Encode(std::vector<T> const& v)
    {
      std::vector<json> j(v.size());
      for(size_t i = 0; i < v.size(); ++i)
        j[i] = JsonEncode<T>(v[i]);
      return std::move(j);
    }
  };

  template <typename K, typename V>
  struct JsonDecoder<std::map<K, V>> : public JsonDecoderBase<std::map<K, V>>
  {
    static std::map<K, V> Decode(json const& j)
    {
      if(!j.is_object())
        throw Exception() << "decoding " << typeid(std::map<K, V>).name() << ", expected JSON object but got: " << j;
      std::map<K, V> r;
      for(auto const& [k, v] : j.items())
        r.insert({ JsonDecode<K>(k), JsonDecode<V>(v) });
      return r;
    }
  };
  template <typename K, typename V>
  struct JsonEncoder<std::map<K, V>>
  {
    static json Encode(std::map<K, V> const& m)
    {
      json r = json::object();
      for(auto const& [k, v] : m)
        r[JsonEncode<K>(k)] = JsonEncode<V>(v);
      return r;
    }
  };

  template <typename K, typename V>
  struct JsonDecoder<std::unordered_map<K, V>> : public JsonDecoderBase<std::unordered_map<K, V>>
  {
    static std::unordered_map<K, V> Decode(json const& j)
    {
      if(!j.is_object())
        throw Exception() << "decoding " << typeid(std::unordered_map<K, V>).name() << ", expected JSON object but got: " << j;
      std::unordered_map<K, V> r;
      r.reserve(j.size());
      for(auto const& [k, v] : j.items())
        r.insert({ JsonDecode<K>(k), JsonDecode<V>(v) });
      return r;
    }
  };
  template <typename K, typename V>
  struct JsonEncoder<std::unordered_map<K, V>>
  {
    static json Encode(std::unordered_map<K, V> const& m)
    {
      json r = json::object();
      for(auto const& [k, v] : m)
        r[JsonEncode<K>(k)] = JsonEncode<V>(v);
      return r;
    }
  };

  // deserialize JSON from buffer
  json JsonFromBuffer(Buffer const& buffer);
  // serialize JSON into buffer
  std::string JsonToString(json const& j);
}
