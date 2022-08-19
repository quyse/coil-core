#pragma once

#include "base.hpp"

namespace Coil
{
  template <typename T, size_t n>
  struct alignas((n <= 1 ? 1 : n <= 2 ? 2 : 4) * sizeof(T)) xvec
  {
    T t[n];

    consteval xvec()
    {
      for(size_t i = 0; i < n; ++i)
        t[i] = {};
    }

    T& operator()(size_t i)
    {
      return t[i];
    }

    T operator()(size_t i) const
    {
      return t[i];
    }

    friend auto operator<=>(xvec const&, xvec const&) = default;
  };

  template <typename T>
  struct alignas(2 * sizeof(T)) xvec<T, 2>
  {
    union
    {
      struct
      {
        T x, y;
      };
      T t[2];
    };

    xvec(T x = T(), T y = T())
    : x(x), y(y) {}

    T& operator()(size_t i)
    {
      return t[i];
    }

    T operator()(size_t i) const
    {
      return t[i];
    }

    friend auto operator<=>(xvec const&, xvec const&) = default;
  };

  template <typename T>
  struct alignas(4 * sizeof(T)) xvec<T, 3>
  {
    union
    {
      struct
      {
        T x, y, z;
      };
      T t[3];
    };

    xvec(T x = T(), T y = T(), T z = T())
    : x(x), y(y), z(z) {}

    T& operator()(size_t i)
    {
      return t[i];
    }

    T operator()(size_t i) const
    {
      return t[i];
    }

    friend auto operator<=>(xvec const&, xvec const&) = default;
  };

  template <typename T>
  struct alignas(4 * sizeof(T)) xvec<T, 4>
  {
    union
    {
      struct
      {
        T x, y, z, w;
      };
      T t[4];
    };

    xvec(T x = T(), T y = T(), T z = T(), T w = T())
    : x(x), y(y), z(z), w(w) {}

    T& operator()(size_t i)
    {
      return t[i];
    }

    T operator()(size_t i) const
    {
      return t[i];
    }

    friend auto operator<=>(xvec const&, xvec const&) = default;
  };

  template <typename T, size_t n, size_t m>
  struct xmat
  {
    struct alignas((m <= 1 ? 1 : m <= 2 ? 2 : 4) * sizeof(T)) Row
    {
      T t[m];
    };

    Row t[n];

    consteval xmat()
    {
      for(size_t i = 0; i < n; ++i)
        for(size_t j = 0; j < m; ++j)
          t[i].t[j] = {};
    }

    constexpr T& operator()(size_t i, size_t j)
    {
      return t[i].t[j];
    }

    constexpr T operator()(size_t i, size_t j) const
    {
      return t[i].t[j];
    }

    friend auto operator<=>(xmat const&, xmat const&) = default;
  };

  template <typename T, size_t n>
  xmat<T, n, n> mat_identity()
  {
    xmat<T, n, n> t;
    for(size_t i = 0; i < n; ++i)
      for(size_t j = 0; j < n; ++j)
        t(i, j) = i == j;
    return t;
  }

  // convenience synonyms

  using vec2 = xvec<float, 2>;
  using vec3 = xvec<float, 3>;
  using vec4 = xvec<float, 4>;
  using mat1x2 = xmat<float, 1, 2>;
  using mat1x3 = xmat<float, 1, 3>;
  using mat1x4 = xmat<float, 1, 4>;
  using mat2x1 = xmat<float, 2, 1>;
  using mat2x2 = xmat<float, 2, 2>;
  using mat2x3 = xmat<float, 2, 3>;
  using mat2x4 = xmat<float, 2, 4>;
  using mat3x1 = xmat<float, 3, 1>;
  using mat3x2 = xmat<float, 3, 2>;
  using mat3x3 = xmat<float, 3, 3>;
  using mat3x4 = xmat<float, 3, 4>;
  using mat4x1 = xmat<float, 4, 1>;
  using mat4x2 = xmat<float, 4, 2>;
  using mat4x3 = xmat<float, 4, 3>;
  using mat4x4 = xmat<float, 4, 4>;

  using uvec2 = xvec<uint32_t, 2>;
  using uvec3 = xvec<uint32_t, 3>;
  using uvec4 = xvec<uint32_t, 4>;
  using umat1x2 = xmat<uint32_t, 1, 2>;
  using umat1x3 = xmat<uint32_t, 1, 3>;
  using umat1x4 = xmat<uint32_t, 1, 4>;
  using umat2x1 = xmat<uint32_t, 2, 1>;
  using umat2x2 = xmat<uint32_t, 2, 2>;
  using umat2x3 = xmat<uint32_t, 2, 3>;
  using umat2x4 = xmat<uint32_t, 2, 4>;
  using umat3x1 = xmat<uint32_t, 3, 1>;
  using umat3x2 = xmat<uint32_t, 3, 2>;
  using umat3x3 = xmat<uint32_t, 3, 3>;
  using umat3x4 = xmat<uint32_t, 3, 4>;
  using umat4x1 = xmat<uint32_t, 4, 1>;
  using umat4x2 = xmat<uint32_t, 4, 2>;
  using umat4x3 = xmat<uint32_t, 4, 3>;
  using umat4x4 = xmat<uint32_t, 4, 4>;

  using ivec2 = xvec<int32_t, 2>;
  using ivec3 = xvec<int32_t, 3>;
  using ivec4 = xvec<int32_t, 4>;
  using imat1x2 = xmat<int32_t, 1, 2>;
  using imat1x3 = xmat<int32_t, 1, 3>;
  using imat1x4 = xmat<int32_t, 1, 4>;
  using imat2x1 = xmat<int32_t, 2, 1>;
  using imat2x2 = xmat<int32_t, 2, 2>;
  using imat2x3 = xmat<int32_t, 2, 3>;
  using imat2x4 = xmat<int32_t, 2, 4>;
  using imat3x1 = xmat<int32_t, 3, 1>;
  using imat3x2 = xmat<int32_t, 3, 2>;
  using imat3x3 = xmat<int32_t, 3, 3>;
  using imat3x4 = xmat<int32_t, 3, 4>;
  using imat4x1 = xmat<int32_t, 4, 1>;
  using imat4x2 = xmat<int32_t, 4, 2>;
  using imat4x3 = xmat<int32_t, 4, 3>;
  using imat4x4 = xmat<int32_t, 4, 4>;

  using bvec2 = xvec<bool, 2>;
  using bvec3 = xvec<bool, 3>;
  using bvec4 = xvec<bool, 4>;
  using bmat1x2 = xmat<bool, 1, 2>;
  using bmat1x3 = xmat<bool, 1, 3>;
  using bmat1x4 = xmat<bool, 1, 4>;
  using bmat2x1 = xmat<bool, 2, 1>;
  using bmat2x2 = xmat<bool, 2, 2>;
  using bmat2x3 = xmat<bool, 2, 3>;
  using bmat2x4 = xmat<bool, 2, 4>;
  using bmat3x1 = xmat<bool, 3, 1>;
  using bmat3x2 = xmat<bool, 3, 2>;
  using bmat3x3 = xmat<bool, 3, 3>;
  using bmat3x4 = xmat<bool, 3, 4>;
  using bmat4x1 = xmat<bool, 4, 1>;
  using bmat4x2 = xmat<bool, 4, 2>;
  using bmat4x3 = xmat<bool, 4, 3>;
  using bmat4x4 = xmat<bool, 4, 4>;

  template <typename T>
  using xquat = xvec<T, 4>;
  using quat = xquat<float>;
  using dquat = xquat<double>;

  // some alignment/size tests
  static_assert(sizeof(xvec<float, 1>) == 4 && alignof(xvec<float, 1>) == 4);
  static_assert(sizeof(vec2) == 8 && alignof(vec2) == 8);
  static_assert(sizeof(vec3) == 16 && alignof(vec3) == 16);
  static_assert(sizeof(vec4) == 16 && alignof(vec4) == 16);
  static_assert(sizeof(mat2x3) == 32 && alignof(mat2x3) == 16);
  static_assert(sizeof(mat3x2) == 24 && alignof(mat3x2) == 8);
  static_assert(sizeof(mat3x4) == 48 && alignof(mat3x4) == 16);
  static_assert(sizeof(mat4x3) == 64 && alignof(mat3x4) == 16);
  static_assert(sizeof(mat4x4) == 64 && alignof(mat4x4) == 16);
}
