#pragma once

#include "math.hpp"
#include "graphics_shaders.hpp"

namespace Coil
{
  template <typename Transform, typename T, size_t n>
  concept IsTransform = requires(Transform const& t, xvec<T, n> const& p)
  {
    { t * p } -> std::same_as<xvec<T, n>>;
    { t * t } -> std::same_as<Transform>;
  };

  // application of quaternion to vector
  template <typename T>
  constexpr xvec<T, 3> operator*(xquat<T> const& q, xvec<T, 3> const& p)
  {
    xquat<T> r = q * xquat<T>(p.x(), p.y(), p.z(), 0) * -q;
    return xvec<T, 3>(r.x(), r.y(), r.z());
  }

  // transform represented as quaternion + offset
  template <typename T>
  struct xquatoffset
  {
    consteval xquatoffset() = default;

    constexpr xquatoffset(xquat<T> const& q, xvec<T, 3> const& o)
    : q(q), o(o) {}

    xquat<T> q;
    xvec<T, 3> o;
  };

  // quatoffset combination
  template <typename T>
  constexpr xquatoffset<T> operator*(xquatoffset<T> const& a, xquatoffset<T> const& b)
  {
    return xquatoffset<T>(a.q * b.q, a.q * b.o + a.o);
  }

  // convert quatoffset to matrix
  template <typename T>
  constexpr xmat<T, 4, 4> AffineFromQuatOffset(xquatoffset<T> const& qo)
  {
    xmat<T, 4, 4> t = AffineFromQuat(qo.q);
    t(0, 3) = qo.o(0);
    t(1, 3) = qo.o(1);
    t(2, 3) = qo.o(2);
    return t;
  }

  // convenience type synonyms
  using quatoffset = xquatoffset<float>;
  using dquatoffset = xquatoffset<double>;

  // shader operators
  namespace Shaders
  {
    // application of quaternion to vector
    template <typename T>
    ShaderExpression<xvec<T, 3>> operator*(ShaderExpression<xquat<T>> const& q, ShaderExpression<xvec<T, 3>> const& p)
    {
      auto qq = swizzle(q, "xyz");
      return p + cross(qq, cross(qq, p) + p * swizzle(q, "w")) * ShaderExpression<T>(2);
    }
  }
}
