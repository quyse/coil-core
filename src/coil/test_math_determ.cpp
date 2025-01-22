#include "entrypoint.hpp"
#include <cstring>
#include <iostream>
#include <random>

import coil.core.math.debug;
import coil.core.math.determ.debug;
import coil.core.math.determ;
import coil.core.math;

using namespace Coil;

std::mt19937_64 g_rnd;

float g_eps1 = 0, g_eps2 = 0;

template <typename T>
struct RandomGenerator;

template <>
struct RandomGenerator<float_d>
{
  static float_d Generate()
  {
    return float_d((int32_t)g_rnd()) * 1000.0_fd / 0x7fffffff_fd;
  }
};

template <typename T>
struct RandomGenerator<xvec_d<T, 2>>
{
  static xvec_d<T, 2> Generate()
  {
    auto x = RandomGenerator<float_d>::Generate();
    auto y = RandomGenerator<float_d>::Generate();
    return xvec_d<T, 2>(x, y);
  }
};
template <typename T>
struct RandomGenerator<xvec_d<T, 3>>
{
  static xvec_d<T, 3> Generate()
  {
    auto x = RandomGenerator<float_d>::Generate();
    auto y = RandomGenerator<float_d>::Generate();
    auto z = RandomGenerator<float_d>::Generate();
    return xvec_d<T, 3>(x, y, z);
  }
};
template <typename T>
struct RandomGenerator<xvec_d<T, 4>>
{
  static xvec_d<T, 4> Generate()
  {
    auto x = RandomGenerator<float_d>::Generate();
    auto y = RandomGenerator<float_d>::Generate();
    auto z = RandomGenerator<float_d>::Generate();
    auto w = RandomGenerator<float_d>::Generate();
    return xvec_d<T, 4>(x, y, z, w);
  }
};

template <typename T>
struct RandomGenerator<xquat_d<T>>
{
  static xquat_d<T> Generate()
  {
    return xquat_d<T>(RandomGenerator<xvec_d<T, 4>>::Generate());
  }
};

template <typename T>
struct Comparer
{
  static bool IsEqual(T const& a, T const& b)
  {
    return std::abs(a - b) < g_eps1 + g_eps2 * std::min(std::abs(a), std::abs(b)) || memcmp(&a, &b, sizeof(T)) == 0;
  }

  static bool IsEqual(T const& a, xvec_d<T, 1> const& b)
  {
    return IsEqual(a, (T)b);
  }
};

template <typename T, size_t n>
requires (n > 1)
struct Comparer<xvec<T, n>>
{
  static bool IsEqual(xvec<T, n> const& a, xvec_d<T, n> const& b)
  {
    for(size_t i = 0; i < n; ++i)
      if(!Comparer<T>::IsEqual(a(i), b.t[i])) return false;
    return true;
  }
};

template <typename T>
struct Comparer<xquat<T>>
{
  static bool IsEqual(xquat<T> const& a, xquat_d<T> const& b)
  {
    return Comparer<xvec<T, 4>>::IsEqual(a, b);
  }
};

uint32_t g_hashSum = 0;
bool g_hashOk = true;

void add_hash(uint32_t a)
{
  g_hashSum = g_hashSum * 65599 + a;
}
template <size_t n>
void add_hash(xvec_d<float, n> const& a)
{
  for(size_t i = 0; i < n; ++i)
    add_hash(*(uint32_t*)&a.t[i]);
}

void check_hash(char const* msg, uint32_t expected)
{
  std::cout << msg;
  if(g_hashSum == expected)
  {
    std::cout << " HASH OK\n";
  }
  else
  {
    std::cout << " HASH WRONG: got " << g_hashSum << ", expected " << expected << "\n";
    g_hashOk = false;
  }
  g_hashSum = 0;
}

int COIL_ENTRY_POINT(std::vector<std::string> args)
{
  uint32_t okCount = 0, totalCount = 0;

  auto test_op1 = [&]<typename T, typename D>(char const* s, auto const& f)
  {
    D a_d = RandomGenerator<D>::Generate();
    T a = (T)a_d;
    auto r = f(a);
    auto r_d = f(a_d);
    add_hash(r_d);
    if(Comparer<decltype(r)>::IsEqual(r, r_d)) ++okCount;
    ++totalCount;
  };

  auto test_op2 = [&]<typename T, typename D>(char const* s, auto const& f)
  {
    D a_d = RandomGenerator<D>::Generate();
    T a = (T)a_d;
    D b_d = RandomGenerator<D>::Generate();
    T b = (T)b_d;
    auto r = f(a, b);
    auto r_d = f(a_d, b_d);
    add_hash(r_d);
    if(Comparer<decltype(r)>::IsEqual(r, r_d)) ++okCount;
    ++totalCount;
  };

  auto test_op2x = [&]<typename T1, typename T2, typename D1, typename D2>(char const* s, auto const& f)
  {
    D1 a_d = RandomGenerator<D1>::Generate();
    T1 a = (T1)a_d;
    D2 b_d = RandomGenerator<D2>::Generate();
    T2 b = (T2)b_d;
    auto r = f(a, b);
    auto r_d = f(a_d, b_d);
    add_hash(r_d);
    if(Comparer<decltype(r)>::IsEqual(r, r_d)) ++okCount;
    ++totalCount;
  };

  auto test_op3 = [&]<typename T, typename D>(char const* s, auto const& f)
  {
    D a_d = RandomGenerator<D>::Generate();
    T a = (T)a_d;
    D b_d = RandomGenerator<D>::Generate();
    T b = (T)b_d;
    D c_d = RandomGenerator<D>::Generate();
    T c = (T)c_d;
    auto r = f(a, b, c);
    auto r_d = f(a_d, b_d, c_d);
    add_hash(r_d);
    if(Comparer<decltype(r)>::IsEqual(r, r_d)) ++okCount;
    ++totalCount;
  };

  auto test_type_unary_ops = [&]<typename T, typename D>(char const* typeName)
  {
    test_op1.operator()<T, D>("-", [&](auto const& a)
    {
      return -a;
    });
    test_op1.operator()<T, D>("x", [&](auto const& a)
    {
      if constexpr(std::same_as<std::decay_t<decltype(a)>, float>) return a;
      else return a.x();
    });
    if constexpr(!std::same_as<T, float>)
    {
      test_op1.operator()<T, D>("y", [&](auto const& a)
      {
        return a.y();
      });
      if constexpr(!std::same_as<T, vec2>)
      {
        test_op1.operator()<T, D>("z", [&](auto const& a)
        {
          return a.z();
        });
        if constexpr(!std::same_as<T, vec3>)
        {
          test_op1.operator()<T, D>("w", [&](auto const& a)
          {
            return a.w();
          });
        }
      }
      test_op1.operator()<T, D>("length2", [&](auto const& a)
      {
        return length2(a);
      });
      test_op1.operator()<T, D>("length", [&](auto const& a)
      {
        return length(a);
      });
      test_op1.operator()<T, D>("normalize", [&](auto const& a)
      {
        return normalize(a);
      });
    }
  };
  auto test_type_binary_ops = [&]<typename T, typename D>(char const* typeName)
  {
    test_op2.operator()<T, D>("+", [&](auto const& a, auto const& b)
    {
      return a + b;
    });
    test_op2.operator()<T, D>("+=", [&](auto a, auto const& b)
    {
      a += b;
      return a;
    });
    test_op2.operator()<T, D>("-", [&](auto const& a, auto const& b)
    {
      return a - b;
    });
    test_op2.operator()<T, D>("-=", [&](auto a, auto const& b)
    {
      a -= b;
      return a;
    });
    test_op2.operator()<T, D>("*", [&](auto const& a, auto const& b)
    {
      return a * b;
    });
    test_op2.operator()<T, D>("*=", [&](auto a, auto const& b)
    {
      a *= b;
      return a;
    });
    test_op2.operator()<T, D>("/", [&](auto const& a, auto const& b)
    {
      return a / b;
    });
    test_op2.operator()<T, D>("/=", [&](auto a, auto const& b)
    {
      a /= b;
      return a;
    });
    if constexpr(!std::same_as<T, float>)
    {
      test_op2.operator()<T, D>("dot", [&](auto const& a, auto const& b)
      {
        return dot(a, b);
      });
    }
    if constexpr(std::same_as<T, vec3>)
    {
      test_op2.operator()<T, D>("cross", [&](auto const& a, auto const& b)
      {
        return cross(a, b);
      });
    }
  };
  auto test_type_vec_scalar_ops = [&]<typename T1, typename T2, typename D1, typename D2>(char const* typeName)
  {
    test_op2x.operator()<T1, T2, D1, D2>("*", [&](auto const& a, auto const& b)
    {
      return a * b;
    });
    test_op2x.operator()<T1, T2, D1, D2>("*", [&](auto const& a, auto const& b)
    {
      return b * a;
    });
    test_op2x.operator()<T1, T2, D1, D2>("*=", [&](auto const& a, auto b)
    {
      b *= a;
      return b;
    });
    test_op2x.operator()<T1, T2, D1, D2>("/", [&](auto const& a, auto const& b)
    {
      return a / b;
    });
    test_op2x.operator()<T1, T2, D1, D2>("/", [&](auto const& a, auto const& b)
    {
      return b / a;
    });
    test_op2x.operator()<T1, T2, D1, D2>("/=", [&](auto const& a, auto b)
    {
      b /= a;
      return b;
    });
  };
  auto test_type_exprs = [&]<typename T, typename D>(char const* typeName)
  {
    test_op3.operator()<T, D>("mul+add", [&](auto const& a, auto const& b, auto const& c)
    {
      return a * b + c;
    });
  };
  auto test_type_simple_funcs = [&]<typename T, typename D>(char const* typeName)
  {
    test_op1.operator()<T, D>("abs", [&](auto const& a)
    {
      if constexpr(std::same_as<std::decay_t<decltype(a)>, float>) return std::abs(a);
      else return abs(a);
    });
    test_op1.operator()<T, D>("floor", [&](auto const& a)
    {
      if constexpr(std::same_as<std::decay_t<decltype(a)>, float>) return std::floor(a);
      else return floor(a);
    });
    test_op1.operator()<T, D>("ceil", [&](auto const& a)
    {
      if constexpr(std::same_as<std::decay_t<decltype(a)>, float>) return std::ceil(a);
      else return ceil(a);
    });
    test_op1.operator()<T, D>("trunc", [&](auto const& a)
    {
      if constexpr(std::same_as<std::decay_t<decltype(a)>, float>) return std::trunc(a);
      else return trunc(a);
    });
    test_op1.operator()<T, D>("sqrt", [&](auto const& a)
    {
      if constexpr(std::same_as<std::decay_t<decltype(a)>, float>) return std::sqrt(a);
      else return sqrt(a);
    });
  };
  auto test_type_quat_ops = [&]<typename T, typename D>(char const* typeName)
  {
    test_op1.operator()<T, D>("-", [&](auto const& a)
    {
      return -a;
    });
    test_op2.operator()<T, D>("*", [&](auto const& a, auto const& b)
    {
      return a * b;
    });
  };
  auto test_type_approx_funcs = [&]<typename T, typename D>(char const* typeName)
  {
    test_op1.operator()<T, D>("sin", [&](auto const& a)
    {
      if constexpr(std::same_as<std::decay_t<decltype(a)>, float>) return std::sin(a);
      else return sin(a);
    });
    test_op1.operator()<T, D>("cos", [&](auto const& a)
    {
      if constexpr(std::same_as<std::decay_t<decltype(a)>, float>) return std::cos(a);
      else return cos(a);
    });
  };

  for(size_t i = 0; i < 100000; ++i)
  {
    g_eps1 = 1e-8f;
    g_eps2 = 1e-1f;
    test_type_unary_ops.operator()<float, float_d>("float");
    test_type_unary_ops.operator()<vec2, vec2_d>("vec2");
    test_type_unary_ops.operator()<vec3, vec3_d>("vec3");
    test_type_unary_ops.operator()<vec4, vec4_d>("vec4");
    test_type_binary_ops.operator()<float, float_d>("float");
    test_type_binary_ops.operator()<vec2, vec2_d>("vec2");
    test_type_binary_ops.operator()<vec3, vec3_d>("vec3");
    test_type_binary_ops.operator()<vec4, vec4_d>("vec4");
    test_type_vec_scalar_ops.operator()<float, vec2, float_d, vec2_d>("vec2");
    test_type_vec_scalar_ops.operator()<float, vec3, float_d, vec3_d>("vec3");
    test_type_vec_scalar_ops.operator()<float, vec4, float_d, vec4_d>("vec4");
    test_type_exprs.operator()<float, float_d>("float");
    test_type_exprs.operator()<vec2, vec2_d>("vec2");
    test_type_exprs.operator()<vec3, vec3_d>("vec3");
    test_type_exprs.operator()<vec4, vec4_d>("vec4");
    test_type_simple_funcs.operator()<float, float_d>("float");
    test_type_quat_ops.operator()<quat, quat_d>("quat");
    g_eps1 = 1e-4f;
    g_eps2 = 1e-1f;
    test_type_approx_funcs.operator()<float, float_d>("float");
  }

  std::cout << "OK: " << okCount << " FAILED: " << (totalCount - okCount) << ", TOTAL: " << totalCount << "\n";

  check_hash("total", 4166856038);

  return (g_hashOk && okCount == totalCount) ? 0 : 1;
}
