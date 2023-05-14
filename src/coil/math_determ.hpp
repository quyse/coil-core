#pragma once

#include "math.hpp"

namespace Coil
{
  template <typename T, size_t n>
  struct xvec_d;

  template <size_t n>
  struct alignas((n <= 1 ? 1 : 4) * sizeof(float)) xvec_d<float, n>
  {
    float t[n <= 1 ? 1 : 4];

    using scalar = xvec_d<float, 1>;

    xvec_d(std::nullptr_t); // uninitialized
    xvec_d();
    xvec_d(xvec_d const&);
    xvec_d& operator=(xvec_d const&);

    // xvec_d(scalar const& x) requires (n == 1); // duplicates copy constructor
    xvec_d(scalar const& x, scalar const& y) requires (n == 2);
    xvec_d(scalar const& x, scalar const& y, scalar const& z) requires (n == 3);
    xvec_d(scalar const& x, scalar const& y, scalar const& z, scalar const& w) requires (n == 4);

    // explicit constructors from non-deterministic values
    explicit xvec_d(float x) requires (n == 1);
    explicit xvec_d(xvec<float, n> const& other) requires (n > 1);
    explicit xvec_d(std::initializer_list<float> const& list);
    explicit xvec_d(int32_t x) requires (n == 1);

    // element access
    scalar x() const;
    scalar y() const requires (1 < n);
    scalar z() const requires (2 < n);
    scalar w() const requires (3 < n);
    void x(scalar const& v);
    void y(scalar const& v) requires (1 < n);
    void z(scalar const& v) requires (2 < n);
    void w(scalar const& v) requires (3 < n);

    bool operator==(xvec_d const& b) const;
    bool operator!=(xvec_d const& b) const;
    bool operator<(xvec_d const& b) const;
    bool operator<=(xvec_d const& b) const;
    bool operator>(xvec_d const& b) const;
    bool operator>=(xvec_d const& b) const;

    xvec_d operator-() const;

    // elementwise addition
    xvec_d& operator+=(xvec_d const& b);
    // elementwise subtraction
    xvec_d& operator-=(xvec_d const& b);
    // elementwise multiplication
    xvec_d& operator*=(xvec_d const& b);
    // vector-scalar multiplication
    xvec_d& operator*=(scalar const& b) requires (n > 1);
    // scalar-vector multiplication
    xvec_d mul_reverse(scalar const& a) const requires (n > 1);
    // elementwise division
    xvec_d& operator/=(xvec_d const& b);
    // vector-scalar division
    xvec_d& operator/=(scalar const& b) requires (n > 1);
    // scalar-vector division
    xvec_d div_reverse(scalar const& a) const requires (n > 1);

    explicit operator float() const requires (n == 1);
    explicit operator xvec<float, n>() const requires (n > 1);
    explicit operator int32_t() const requires (n == 1);
  };

  template <typename T>
  struct xquat_d;

  template <>
  struct xquat_d<float> : public xvec_d<float, 4>
  {
    xquat_d() = default;
    xquat_d(xquat_d const&) = default;
    explicit xquat_d(xvec_d<float, 4> const& other)
    : xvec_d<float, 4>(other) {}
    xquat_d& operator=(xquat_d const&) = default;

    // explicit constructor from non-deterministic value
    explicit xquat_d(xquat<float> const& other)
    : xvec_d<float, 4>(other) {}

    // quaternion conjugation
    xquat_d operator-() const;

    explicit operator xquat<float>() const;
  };

  // vector addition
  template <typename T, size_t n>
  xvec_d<T, n> operator+(xvec_d<T, n> a, xvec_d<T, n> const& b)
  {
    return a += b;
  }
  // vector subtraction
  template <typename T, size_t n>
  xvec_d<T, n> operator-(xvec_d<T, n> a, xvec_d<T, n> const& b)
  {
    return a -= b;
  }

  // scalar-scalar or vector-vector elementwise multiplication
  template <typename T, size_t n>
  xvec_d<T, n> operator*(xvec_d<T, n> a, xvec_d<T, n> const& b)
  {
    return a *= b;
  }
  // vector-scalar multiplication
  template <typename T, size_t n> requires (n > 1)
  xvec_d<T, n> operator*(xvec_d<T, n> a, xvec_d<T, 1> const& b)
  {
    return a *= b;
  }
  // scalar-vector multiplication
  template <typename T, size_t n> requires (n > 1)
  xvec_d<T, n> operator*(xvec_d<T, 1> const& a, xvec_d<T, n> const& b)
  {
    return b.mul_reverse(a);
  }

  // vector-vector elementwise division
  template <typename T, size_t n>
  xvec_d<T, n> operator/(xvec_d<T, n> a, xvec_d<T, n> const& b)
  {
    return a /= b;
  }
  // vector-scalar division
  template <typename T, size_t n> requires (n > 1)
  xvec_d<T, n> operator/(xvec_d<T, n> a, xvec_d<T, 1> const& b)
  {
    return a /= b;
  }
  // scalar-vector division
  template <typename T, size_t n> requires (n > 1)
  xvec_d<T, n> operator/(xvec_d<T, 1> const& a, xvec_d<T, n> const& b)
  {
    return b.div_reverse(a);
  }

  // user-defined literals
  inline xvec_d<float, 1> operator ""_fd(unsigned long long value)
  {
    return xvec_d<float, 1>(static_cast<float>(value));
  }
  inline xvec_d<float, 1> operator ""_fd(long double value)
  {
    return xvec_d<float, 1>(static_cast<float>(value));
  }

  // operations
  template <typename T, size_t n> requires (n > 1) xvec_d<T, 1> dot(xvec_d<T, n> const& a, xvec_d<T, n> const& b);
  template <typename T, size_t n> requires (n > 1) xvec_d<T, 1> length2(xvec_d<T, n> const& a);
  template <typename T, size_t n> requires (n > 1) xvec_d<T, 1> length(xvec_d<T, n> const& a);
  template <typename T, size_t n> requires (n > 1) xvec_d<T, n> normalize(xvec_d<T, n> const& a);
  template <typename T> xvec_d<T, 3> cross(xvec_d<T, 3> const& a, xvec_d<T, 3> const& b);
  template <typename T> xvec_d<T, 1> abs(xvec_d<T, 1> const& a);
  template <typename T> xvec_d<T, 1> floor(xvec_d<T, 1> const& a);
  template <typename T> xvec_d<T, 1> ceil(xvec_d<T, 1> const& a);
  template <typename T> xvec_d<T, 1> trunc(xvec_d<T, 1> const& a);
  template <typename T> xvec_d<T, 1> sqrt(xvec_d<T, 1> const& a);
  template <typename T> xvec_d<T, 1> sin(xvec_d<T, 1> a);
  template <typename T> xvec_d<T, 1> cos(xvec_d<T, 1> const& a);
  template <typename T> xquat_d<T> operator*(xquat_d<T> const& a, xquat_d<T> const& b);
  // operation specializations
  template <> xvec_d<float, 1> dot(xvec_d<float, 2> const& a, xvec_d<float, 2> const& b);
  template <> xvec_d<float, 1> dot(xvec_d<float, 3> const& a, xvec_d<float, 3> const& b);
  template <> xvec_d<float, 1> dot(xvec_d<float, 4> const& a, xvec_d<float, 4> const& b);
  template <> xvec_d<float, 3> cross(xvec_d<float, 3> const& a, xvec_d<float, 3> const& b);
  template <> xvec_d<float, 1> abs(xvec_d<float, 1> const& a);
  template <> xvec_d<float, 1> floor(xvec_d<float, 1> const& a);
  template <> xvec_d<float, 1> ceil(xvec_d<float, 1> const& a);
  template <> xvec_d<float, 1> trunc(xvec_d<float, 1> const& a);
  template <> xvec_d<float, 1> sqrt(xvec_d<float, 1> const& a);
  template <> xquat_d<float> operator*(xquat_d<float> const& a, xquat_d<float> const& b);

  // constants
  template <typename T> extern xvec_d<T, 1> const pi_d;
  template <typename T> extern xvec_d<T, 1> const pi_half_d;

  // explicit instantiations

  extern template class xvec_d<float, 1>;
  extern template class xvec_d<float, 2>;
  extern template class xvec_d<float, 3>;
  extern template class xvec_d<float, 4>;
  extern template class xquat_d<float>;

  extern template xvec_d<float, 1> length2(xvec_d<float, 2> const& a);
  extern template xvec_d<float, 1> length2(xvec_d<float, 3> const& a);
  extern template xvec_d<float, 1> length2(xvec_d<float, 4> const& a);
  extern template xvec_d<float, 1> length(xvec_d<float, 2> const& a);
  extern template xvec_d<float, 1> length(xvec_d<float, 3> const& a);
  extern template xvec_d<float, 1> length(xvec_d<float, 4> const& a);
  extern template xvec_d<float, 2> normalize(xvec_d<float, 2> const& a);
  extern template xvec_d<float, 3> normalize(xvec_d<float, 3> const& a);
  extern template xvec_d<float, 4> normalize(xvec_d<float, 4> const& a);
  extern template xvec_d<float, 1> sin(xvec_d<float, 1> a);
  extern template xvec_d<float, 1> cos(xvec_d<float, 1> const& a);

  extern template xvec_d<float, 1> const pi_d<float>;
  extern template xvec_d<float, 1> const pi_half_d<float>;

  // convenience synonyms
  using float_d = xvec_d<float, 1>;
  using vec1_d = float_d;
  using vec2_d = xvec_d<float, 2>;
  using vec3_d = xvec_d<float, 3>;
  using vec4_d = xvec_d<float, 4>;
  using quat_d = xquat_d<float>;
}
