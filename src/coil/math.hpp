#pragma once

#include "base.hpp"
#include <cmath>

namespace Coil
{
  template <typename T, size_t n, size_t alignment = (n <= 1 ? 1 : n <= 2 ? 2 : 4) * sizeof(T)>
  struct alignas(alignment) xvec
  {
    T t[n];

    consteval xvec()
    {
      for(size_t i = 0; i < n; ++i)
        t[i] = {};
    }

    constexpr xvec(xvec const&) = default;
    constexpr xvec& operator=(xvec const&) = default;

    // tolerate another alignment when copying
    template <size_t alignment2>
    constexpr xvec(xvec<T, n, alignment2> const& value)
    {
      for(size_t i = 0; i < n; ++i)
        t[i] = value.t[i];
    }
    template <size_t alignment2>
    constexpr xvec& operator=(xvec<T, n, alignment2> const& value)
    {
      for(size_t i = 0; i < n; ++i)
        t[i] = value.t[i];
      return *this;
    }

    template <typename... Args>
    constexpr xvec(Args const... args)
    requires (std::is_convertible_v<Args, T> && ...)
    : xvec({ static_cast<T>(args)... }) {}

    constexpr xvec(std::initializer_list<T> const& list)
    {
      T const* data = std::data(list);
      size_t k = 0;
      size_t listSize = list.size();
      for(size_t i = 0; i < n; ++i)
        t[i] = k < listSize ? data[k++] : T{};
    }

    T& operator()(size_t i)
    {
      return t[i];
    }

    T operator()(size_t i) const
    {
      return t[i];
    }

    constexpr T x() const { return t[0]; }
    constexpr T y() const { return t[1]; }
    constexpr T z() const { return t[2]; }
    constexpr T w() const { return t[3]; }
    constexpr T& x() { return t[0]; }
    constexpr T& y() { return t[1]; }
    constexpr T& z() { return t[2]; }
    constexpr T& w() { return t[3]; }

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

    constexpr xmat(std::initializer_list<T> const& list)
    {
      T const* data = std::data(list);
      size_t k = 0;
      size_t listSize = list.size();
      for(size_t i = 0; i < n; ++i)
        for(size_t j = 0; j < m; ++j)
          t[i].t[j] = k < listSize ? data[k++] : T{};
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
  constexpr xmat<T, n, n> mat_identity()
  {
    xmat<T, n, n> t;
    for(size_t i = 0; i < n; ++i)
      for(size_t j = 0; j < n; ++j)
        t(i, j) = i == j;
    return t;
  }

  template <typename T, size_t alignment = 4 * sizeof(T)>
  struct xquat : public xvec<T, 4, alignment>
  {
    consteval xquat()
    {
      this->w() = 1;
    }

    constexpr xquat(T x, T y, T z, T w)
    : xvec<T, 4, alignment>(x, y, z, w) {}

    constexpr xquat(xquat const&) = default;
    constexpr xquat& operator=(xquat const&) = default;

    // tolerate another alignment when copying
    template <size_t alignment2>
    constexpr xquat(xquat<T, alignment2> const& value)
    : xvec<T, 4, alignment>(value) {}
    template <size_t alignment2>
    constexpr xquat& operator=(xquat<T, alignment2> const& value)
    {
      static_cast<xvec<T, 4, alignment>&>(*this) = value;
      return *this;
    }

    // quaternion conjugation
    constexpr xquat operator-() const
    {
      return xquat(-this->x(), -this->y(), -this->z(), this->w());
    }
  };

  // vector addition
  template <typename T, size_t n>
  constexpr xvec<T, n> operator+(xvec<T, n> const& a, xvec<T, n> const& b)
  {
    xvec<T, n> r;
    for(size_t i = 0; i < n; ++i)
      r(i) = a(i) + b(i);
    return r;
  }
  // vector subtraction
  template <typename T, size_t n>
  constexpr xvec<T, n> operator-(xvec<T, n> const& a, xvec<T, n> const& b)
  {
    xvec<T, n> r;
    for(size_t i = 0; i < n; ++i)
      r(i) = a(i) - b(i);
    return r;
  }

  // vector-scalar multiplication
  template <typename T, size_t n>
  constexpr xvec<T, n> operator*(xvec<T, n> const& a, T b)
  {
    xvec<T, n> r;
    for(size_t i = 0; i < n; ++i)
      r(i) = a(i) * b;
    return r;
  }
  // scalar-vector multiplication
  template <typename T, size_t n>
  constexpr xvec<T, n> operator*(T a, xvec<T, n> const& b)
  {
    xvec<T, n> r;
    for(size_t i = 0; i < n; ++i)
      r(i) = a * b(i);
    return r;
  }

  // matrix addition
  template <typename T, size_t n, size_t m>
  constexpr xmat<T, n, m> operator+(xmat<T, n, m> const& a, xmat<T, n, m> const& b)
  {
    xmat<T, n, m> r;
    for(size_t i = 0; i < n; ++i)
      for(size_t j = 0; j < m; ++j)
        r(i, j) = a(i, j) + b(i, j);
    return r;
  }
  // matrix subtraction
  template <typename T, size_t n, size_t m>
  constexpr xmat<T, n, m> operator-(xmat<T, n, m> const& a, xmat<T, n, m> const& b)
  {
    xmat<T, n, m> r;
    for(size_t i = 0; i < n; ++i)
      for(size_t j = 0; j < m; ++j)
        r(i, j) = a(i, j) - b(i, j);
    return r;
  }

  // matrix-matrix multiplication
  template <typename T, size_t n, size_t m, size_t k>
  constexpr xmat<T, n, m> operator*(xmat<T, n, k> const& a, xmat<T, k, m> const& b)
  {
    xmat<T, n, m> r;
    for(size_t i = 0; i < n; ++i)
      for(size_t j = 0; j < m; ++j)
      {
        T p = {};
        for(size_t q = 0; q < k; ++q)
          p += a(i, q) * b(q, j);
        r(i, j) = p;
      }
    return r;
  }

  // matrix-vector multiplication
  template <typename T, size_t n, size_t m>
  constexpr xvec<T, n> operator*(xmat<T, n, m> const& a, xvec<T, m> const& b)
  {
    xvec<T, n> r;
    for(size_t i = 0; i < n; ++i)
    {
      T p = {};
      for(size_t j = 0; j < m; ++j)
        p += a(i, j) * b(j);
      r(i) = p;
    }
    return r;
  }

  // vector-matrix multiplication
  template <typename T, size_t n, size_t m>
  constexpr xvec<T, m> operator*(xvec<T, n> const& a, xmat<T, n, m> const& b)
  {
    xvec<T, m> r;
    for(size_t j = 0; j < m; ++j)
    {
      T p = {};
      for(size_t i = 0; i < n; ++i)
        p += a(i) * b(i, j);
      r(j) = p;
    }
    return r;
  }

  // quaternion multiplication
  template <typename T>
  constexpr xquat<T> operator*(xquat<T> const& a, xquat<T> const& b)
  {
    return xquat<T>(
      a.w() * b.x() + a.x() * b.w() + a.y() * b.z() - a.z() * b.y(),
      a.w() * b.y() - a.x() * b.z() + a.y() * b.w() + a.z() * b.x(),
      a.w() * b.z() + a.x() * b.y() - a.y() * b.x() + a.z() * b.w(),
      a.w() * b.w() - a.x() * b.x() - a.y() * b.y() - a.z() * b.z());
  }

  template <typename T, size_t n>
  constexpr T dot(xvec<T, n> const& a, xvec<T, n> const& b)
  {
    T r = {};
    for(size_t i = 0; i < n; ++i)
      r += a(i) * b(i);
    return r;
  }

  template <typename T, size_t n>
  constexpr T length2(xvec<T, n> const& a)
  {
    return dot(a, a);
  }

  template <typename T, size_t n>
  constexpr T length(xvec<T, n> const& a)
  {
    return sqrt(length2(a));
  }

  template <typename T, size_t n>
  constexpr xvec<T, n> normalize(xvec<T, n> const& a)
  {
    T s = 1 / length(a);
    xvec<T, n> r = {};
    for(size_t i = 0; i < n; ++i)
      r(i) = a(i) * s;
    return r;
  }

  template <typename T>
  constexpr xvec<T, 3> cross(xvec<T, 3> const& a, xvec<T, 3> const& b)
  {
    return
    {
      a.y() * b.z() - a.z() * b.y(),
      a.z() * b.x() - a.x() * b.z(),
      a.x() * b.y() - a.y() * b.x(),
    };
  }

  // some vector traits
  template <typename T, size_t n>
  struct PossiblyScalarVectorTraits
  {
    using PossiblyScalarType = xvec<T, n>;
  };
  template <typename T>
  struct PossiblyScalarVectorTraits<T, 1>
  {
    using PossiblyScalarType = T;
  };
  template <typename T>
  struct VectorTraits;
  template <typename T, size_t n, size_t alignment>
  struct VectorTraits<xvec<T, n, alignment>>
  {
    // scalar type
    using Scalar = T;
    // size of vector
    static constexpr size_t N = n;
    // reduced type
    using PossiblyScalar = typename PossiblyScalarVectorTraits<T, n>::PossiblyScalarType;
  };
  template <typename T, size_t alignment>
  struct VectorTraits<xquat<T, alignment>> : public VectorTraits<xvec<T, 4, alignment>> {};
  // define for selected scalars as well
  template <typename T>
  requires
    std::is_same_v<T, float> ||
    std::is_same_v<T, uint32_t> ||
    std::is_same_v<T, uint16_t> ||
    std::is_same_v<T, uint8_t> ||
    std::is_same_v<T, int32_t> ||
    std::is_same_v<T, int16_t> ||
    std::is_same_v<T, int8_t> ||
    std::is_same_v<T, bool>
  struct VectorTraits<T>
  {
    using Scalar = T;
    static constexpr size_t N = 1;
    using PossiblyScalar = T;
  };

  template <typename T>
  concept IsScalarOrVector = requires
  {
    typename VectorTraits<T>::Scalar;
  };

  template <typename T>
  concept IsFloatScalarOrVector = requires
  {
    std::is_same_v<typename VectorTraits<T>::Scalar, float>;
  };

  // convenience synonyms

  using vec1 = float;
  using vec2 = xvec<float, 2>;
  using vec3 = xvec<float, 3>;
  using vec4 = xvec<float, 4>;
  using mat1x1 = float;
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

  using uvec1 = uint32_t;
  using uvec2 = xvec<uint32_t, 2>;
  using uvec3 = xvec<uint32_t, 3>;
  using uvec4 = xvec<uint32_t, 4>;
  using umat1x1 = uint32_t;
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

  using ivec1 = int32_t;
  using ivec2 = xvec<int32_t, 2>;
  using ivec3 = xvec<int32_t, 3>;
  using ivec4 = xvec<int32_t, 4>;
  using imat1x1 = int32_t;
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

  using bvec1 = bool;
  using bvec2 = xvec<bool, 2>;
  using bvec3 = xvec<bool, 3>;
  using bvec4 = xvec<bool, 4>;
  using bmat1x1 = bool;
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

  using quat = xquat<float>;
  using dquat = xquat<double>;

  // unaligned

  template <typename T, size_t n>
  using xvec_ua = xvec<T, n, 0>;
  using vec1_ua = float;
  using vec2_ua = xvec_ua<float, 2>;
  using vec3_ua = xvec_ua<float, 3>;
  using vec4_ua = xvec_ua<float, 4>;
  template <typename T>
  using xquat_ua = xquat<T, 0>;
  using quat_ua = xquat_ua<float>;

  // some alignment/size tests
  static_assert(sizeof(xvec<float, 1>) == 4 && alignof(xvec<float, 1>) == 4);
  static_assert(sizeof(vec2) == 8 && alignof(vec2) == 8);
  static_assert(sizeof(vec3) == 16 && alignof(vec3) == 16);
  static_assert(sizeof(vec4) == 16 && alignof(vec4) == 16);
  static_assert(sizeof(vec2_ua) == 8 && alignof(vec2_ua) == 4);
  static_assert(sizeof(vec3_ua) == 12 && alignof(vec3_ua) == 4);
  static_assert(sizeof(vec4_ua) == 16 && alignof(vec3_ua) == 4);
  static_assert(sizeof(mat2x3) == 32 && alignof(mat2x3) == 16);
  static_assert(sizeof(mat3x2) == 24 && alignof(mat3x2) == 8);
  static_assert(sizeof(mat3x4) == 48 && alignof(mat3x4) == 16);
  static_assert(sizeof(mat4x3) == 64 && alignof(mat3x4) == 16);
  static_assert(sizeof(mat4x4) == 64 && alignof(mat4x4) == 16);
}
