module;

#include "entrypoint.hpp"
#include <random>

export module coil.core.test.json;

import coil.core.base;
import coil.core.json;
import coil.core.unicode;

using namespace Coil;

namespace
{
  template <typename T>
  struct Random {};

  template <>
  struct Random<int32_t>
  {
    static int32_t Generate()
    {
      static std::mt19937 engine;
      return (int32_t)engine();
    }
  };
  template <>
  struct Random<uint32_t>
  {
    static uint32_t Generate()
    {
      static std::mt19937 engine;
      return (uint32_t)engine();
    }
  };
  template <>
  struct Random<int64_t>
  {
    static int64_t Generate()
    {
      static std::mt19937_64 engine;
      return (int64_t)engine();
    }
  };
  template <>
  struct Random<uint64_t>
  {
    static uint64_t Generate()
    {
      static std::mt19937_64 engine;
      return (uint64_t)engine();
    }
  };
  template <>
  struct Random<float>
  {
    static float Generate()
    {
      static std::mt19937 engine;
      return (float)engine() / 65536.0f;
    }
  };
  template <>
  struct Random<std::string>
  {
    static std::string Generate()
    {
      std::vector<char32_t> v(Random<uint32_t>::Generate() % 100);
      for(size_t i = 0; i < v.size(); ++i)
      {
        for(;;)
        {
          char32_t c = (Random<uint32_t>::Generate() & ((1 << 20) - 1)) + 1;
          // do not generate surrogates
          if(c >= 0xD800 && c <= 0xDFFF) continue;
          v[i] = c;
          break;
        }
      }
      std::string s;
      Unicode::Convert<char32_t, char>(v.begin(), v.end(), s);

      return std::move(s);
    }
  };
  template <typename T>
  struct Random<std::optional<T>>
  {
    static std::optional<T> Generate()
    {
      if(Random<uint32_t>::Generate() % 2)
        return Random<T>::Generate();
      return {};
    }
  };
  template <typename T>
  struct Random<std::vector<T>>
  {
    static std::vector<T> Generate()
    {
      std::vector<T> v(Random<uint32_t>::Generate() % 100);
      for(size_t i = 0; i < v.size(); ++i)
        v[i] = Random<T>::Generate();
      return std::move(v);
    }
  };
}

template <typename T>
bool TestEncodeDecode()
{
  T value = Random<T>::Generate();
  Json j = JsonEncode<T>(value);
  std::string s = JsonToString(j);
  Json j2 = JsonFromBuffer(Buffer(s.data(), s.length()));
  T decodedValue = JsonDecode<T>(j);
  return value == decodedValue && j == j2;
}

template <typename T>
bool Test()
{
  for(size_t i = 0; i < 100; ++i)
    if(!TestEncodeDecode<T>())
      return false;
  return true;
}

extern "C++" int COIL_ENTRY_POINT(std::vector<std::string> args)
{
  if(!Test<int32_t>()) return 1;
  if(!Test<uint32_t>()) return 1;
  if(!Test<int64_t>()) return 1;
  if(!Test<uint64_t>()) return 1;
  if(!Test<float>()) return 1;
  if(!Test<std::string>()) return 1;
  if(!Test<std::vector<std::optional<uint32_t>>>()) return 1;

  return 0;
}
