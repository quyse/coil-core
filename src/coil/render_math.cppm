module;

#include <cstddef>
#include <concepts>

export module coil.core.render.math;

import coil.core.graphics.shaders;
import coil.core.math;

export namespace Coil
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

  // axis-aligned bounding box
  template <typename T, size_t n>
  struct xaabb
  {
    xvec<T, n> a;
    xvec<T, n> b;

    constexpr void normalize()
    {
      for(size_t i = 0; i < n; ++i)
        if(a(i) > b(i))
          swap(a(i), b(i));
    }
  };

  // combining AABBs (as a Minkowski sum)
  template <typename T, size_t n>
  constexpr xaabb<T, n> operator+(xaabb<T, n> const& a, xaabb<T, n> const& b)
  {
    return
    {
      .a = a.a + b.a,
      .b = a.b + b.b,
    };
  }

  // AABB tranform
  template <typename T>
  constexpr xaabb<T, 3> operator*(xquatoffset<T> const& qo, xaabb<T, 3> const& aabb)
  {
    auto x = qo.q * xvec<T, 3>(1, 0, 0);
    auto y = qo.q * xvec<T, 3>(0, 1, 0);
    auto z = qo.q * xvec<T, 3>(0, 0, 1);
    xaabb<T, 3> tx =
    {
      .a = x * aabb.a.x(),
      .b = x * aabb.b.x(),
    };
    tx.normalize();
    xaabb<T, 3> ty =
    {
      .a = y * aabb.a.y(),
      .b = y * aabb.b.y(),
    };
    ty.normalize();
    xaabb<T, 3> tz =
    {
      .a = z * aabb.a.z(),
      .b = z * aabb.b.z(),
    };
    tz.normalize();
    return tx + ty + tz;
  }

  // convenience type synonyms
  using quatoffset = xquatoffset<float>;
  using dquatoffset = xquatoffset<double>;
  using aabb3 = xaabb<float, 3>;
  using daabb3 = xaabb<double, 3>;

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
