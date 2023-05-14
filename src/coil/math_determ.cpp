#include "math_determ.hpp"
#include <numbers>

// for now we only support SSE2

#define IMPL_SSE2

#if defined(IMPL_SSE2)
#include <emmintrin.h>
#else
#error "no deterministic math implementation"
#endif

// this file should be compiled with strict floating point model

namespace Coil
{
  template <size_t n>
  xvec_d<float, n>::xvec_d(std::nullptr_t) {}

  template <size_t n>
  xvec_d<float, n>::xvec_d()
  {
#if defined(IMPL_SSE2)
    if constexpr(n == 1)
    {
      _mm_store_ss(t, _mm_setzero_ps());
    }
    else
    {
      _mm_store_ps(t, _mm_setzero_ps());
    }
#endif
  }

  template <size_t n>
  xvec_d<float, n>::xvec_d(xvec_d const& other)
  {
    *this = other;
  }

  template <size_t n>
  xvec_d<float, n>& xvec_d<float, n>::operator=(xvec_d const& other)
  {
#if defined(IMPL_SSE2)
    if constexpr(n == 1)
    {
      _mm_store_ss(t, _mm_load_ss(other.t));
    }
    else
    {
      _mm_store_ps(t, _mm_load_ps(other.t));
    }
#endif
    return *this;
  }

  template <size_t n>
  xvec_d<float, n>::xvec_d(scalar const& x, scalar const& y) requires (n == 2)
  {
#if defined(IMPL_SSE2)
    _mm_store_ps(t, _mm_unpacklo_ps(_mm_load_ss(x.t), _mm_load_ss(y.t)));
#endif
  }
  template <size_t n>
  xvec_d<float, n>::xvec_d(scalar const& x, scalar const& y, scalar const& z) requires (n == 3)
  {
#if defined(IMPL_SSE2)
    _mm_store_ps(t, _mm_unpacklo_ps(
      _mm_unpacklo_ps(_mm_load_ss(x.t), _mm_load_ss(z.t)),
      _mm_load_ss(y.t)
    ));
#endif
  }
  template <size_t n>
  xvec_d<float, n>::xvec_d(scalar const& x, scalar const& y, scalar const& z, scalar const& w) requires (n == 4)
  {
#if defined(IMPL_SSE2)
    _mm_store_ps(t, _mm_unpacklo_ps(
      _mm_unpacklo_ps(_mm_load_ss(x.t), _mm_load_ss(z.t)),
      _mm_unpacklo_ps(_mm_load_ss(y.t), _mm_load_ss(w.t))
    ));
#endif
  }

  template <size_t n>
  xvec_d<float, n>::xvec_d(float x) requires (n == 1)
  {
    t[0] = x;
  }

  template <size_t n>
  xvec_d<float, n>::xvec_d(xvec<float, n> const& other) requires (n > 1)
  {
    for(size_t i = 0; i < n; ++i)
      t[i] = other.t[i];
  }

  template <size_t n>
  xvec_d<float, n>::xvec_d(std::initializer_list<float> const& list)
  {
    float const* data = std::data(list);
    size_t k = 0;
    size_t listSize = list.size();
    for(size_t i = 0; i < n; ++i)
      t[i] = k < listSize ? data[k++] : float{};
  }

  template <size_t n>
  xvec_d<float, n>::xvec_d(int32_t x) requires (n == 1)
  {
    _mm_store_ss(t, _mm_cvt_si2ss(_mm_setzero_ps(), x));
  }

  template <size_t n>
  xvec_d<float, 1> xvec_d<float, n>::x() const
  {
#if defined(IMPL_SSE2)
    scalar r(nullptr);
    _mm_store_ss(r.t, _mm_load_ss(t));
    return r;
#endif
  }
  template <size_t n>
  xvec_d<float, 1> xvec_d<float, n>::y() const requires (1 < n)
  {
#if defined(IMPL_SSE2)
    scalar r(nullptr);
    auto v = _mm_load_ps(t);
    _mm_store_ss(r.t, _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 1)));
    return r;
#endif
  }
  template <size_t n>
  xvec_d<float, 1> xvec_d<float, n>::z() const requires (2 < n)
  {
#if defined(IMPL_SSE2)
    scalar r(nullptr);
    auto v = _mm_load_ps(t);
    _mm_store_ss(r.t, _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 2)));
    return r;
#endif
  }
  template <size_t n>
  xvec_d<float, 1> xvec_d<float, n>::w() const requires (3 < n)
  {
#if defined(IMPL_SSE2)
    scalar r(nullptr);
    auto v = _mm_load_ps(t);
    _mm_store_ss(r.t, _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 3)));
    return r;
#endif
  }
  template <size_t n>
  void xvec_d<float, n>::x(scalar const& v)
  {
#if defined(IMPL_SSE2)
    _mm_store_ss(t, _mm_load_ss(v.t));
#endif
  }
  template <size_t n>
  void xvec_d<float, n>::y(scalar const& v) requires (1 < n)
  {
#if defined(IMPL_SSE2)
    _mm_store_ss(t + 1, _mm_load_ss(v.t));
#endif
  }
  template <size_t n>
  void xvec_d<float, n>::z(scalar const& v) requires (2 < n)
  {
#if defined(IMPL_SSE2)
    _mm_store_ss(t + 2, _mm_load_ss(v.t));
#endif
  }
  template <size_t n>
  void xvec_d<float, n>::w(scalar const& v) requires (3 < n)
  {
#if defined(IMPL_SSE2)
    _mm_store_ss(t + 3, _mm_load_ss(v.t));
#endif
  }

  template <size_t n>
  bool xvec_d<float, n>::operator==(xvec_d const& b) const
  {
#if defined(IMPL_SSE2)
    if constexpr(n == 1)
    {
      return !!_mm_comieq_ss(_mm_load_ss(t), _mm_load_ss(b.t));
    }
    else
    {
      constexpr uint8_t mask = (1 << n) - 1;
      return (_mm_movemask_ps(_mm_cmpeq_ps(_mm_load_ps(t), _mm_load_ps(b.t))) & mask) == mask;
    }
#endif
  }
  template <size_t n>
  bool xvec_d<float, n>::operator!=(xvec_d const& b) const
  {
    return !(*this == b);
  }
  template <size_t n>
  bool xvec_d<float, n>::operator<(xvec_d const& b) const
  {
#if defined(IMPL_SSE2)
    if constexpr(n == 1)
    {
      return !!_mm_comilt_ss(_mm_load_ss(t), _mm_load_ss(b.t));
    }
    else
    {
      constexpr uint8_t mask = (1 << n) - 1;
      auto p1 = _mm_load_ps(t);
      auto p2 = _mm_load_ps(b.t);
      auto q1 = _mm_movemask_ps(_mm_cmplt_ps(p1, p2)) & mask;
      auto q2 = _mm_movemask_ps(_mm_cmpgt_ps(p1, p2)) & mask;

      return q1 > q2;
    }
#endif
  }
  template <size_t n>
  bool xvec_d<float, n>::operator<=(xvec_d const& b) const
  {
    return !(b < *this);
  }
  template <size_t n>
  bool xvec_d<float, n>::operator>(xvec_d const& b) const
  {
    return b < *this;
  }
  template <size_t n>
  bool xvec_d<float, n>::operator>=(xvec_d const& b) const
  {
    return !(*this < b);
  }

  template <size_t n>
  xvec_d<float, n> xvec_d<float, n>::operator-() const
  {
    return xvec_d() - *this;
  }

  template <size_t n>
  xvec_d<float, n>& xvec_d<float, n>::operator+=(xvec_d<float, n> const& b)
  {
#if defined(IMPL_SSE2)
    if constexpr(n == 1)
    {
      _mm_store_ss(t, _mm_add_ss(_mm_load_ss(t), _mm_load_ss(b.t)));
    }
    else
    {
      _mm_store_ps(t, _mm_add_ps(_mm_load_ps(t), _mm_load_ps(b.t)));
    }
#endif
    return *this;
  }

  template <size_t n>
  xvec_d<float, n>& xvec_d<float, n>::operator-=(xvec_d<float, n> const& b)
  {
#if defined(IMPL_SSE2)
    if constexpr(n == 1)
    {
      _mm_store_ss(t, _mm_sub_ss(_mm_load_ss(t), _mm_load_ss(b.t)));
    }
    else
    {
      _mm_store_ps(t, _mm_sub_ps(_mm_load_ps(t), _mm_load_ps(b.t)));
    }
#endif
    return *this;
  }

  template <size_t n>
  xvec_d<float, n>& xvec_d<float, n>::operator*=(xvec_d<float, n> const& b)
  {
#if defined(IMPL_SSE2)
    if constexpr(n == 1)
    {
      _mm_store_ss(t, _mm_mul_ss(_mm_load_ss(t), _mm_load_ss(b.t)));
    }
    else
    {
      _mm_store_ps(t, _mm_mul_ps(_mm_load_ps(t), _mm_load_ps(b.t)));
    }
#endif
    return *this;
  }

  template <size_t n>
  xvec_d<float, n>& xvec_d<float, n>::operator*=(xvec_d<float, 1> const& b) requires (n > 1)
  {
#if defined(IMPL_SSE2)
    _mm_store_ps(t, _mm_mul_ps(_mm_load_ps(t), _mm_load_ps1(b.t)));
#endif
    return *this;
  }

  template <size_t n>
  xvec_d<float, n> xvec_d<float, n>::mul_reverse(xvec_d<float, 1> const& a) const requires (n > 1)
  {
#if defined(IMPL_SSE2)
    xvec_d<float, n> r(nullptr);
    _mm_store_ps(r.t, _mm_mul_ps(_mm_load_ps1(a.t), _mm_load_ps(t)));
    return r;
#endif
  }

  template <size_t n>
  xvec_d<float, n>& xvec_d<float, n>::operator/=(xvec_d<float, n> const& b)
  {
#if defined(IMPL_SSE2)
    if constexpr(n == 1)
    {
      _mm_store_ss(t, _mm_div_ss(_mm_load_ss(t), _mm_load_ss(b.t)));
    }
    else
    {
      _mm_store_ps(t, _mm_div_ps(_mm_load_ps(t), _mm_load_ps(b.t)));
      // make sure excess numbers are zeroes
      for(size_t i = n; i < 4; ++i)
        _mm_store_ss(t + i, _mm_setzero_ps());
    }
#endif
    return *this;
  }

  template <size_t n>
  xvec_d<float, n>& xvec_d<float, n>::operator/=(xvec_d<float, 1> const& b) requires (n > 1)
  {
#if defined(IMPL_SSE2)
    _mm_store_ps(t, _mm_div_ps(_mm_load_ps(t), _mm_load_ps1(b.t)));
#endif
    return *this;
  }

  template <size_t n>
  xvec_d<float, n> xvec_d<float, n>::div_reverse(xvec_d<float, 1> const& a) const requires (n > 1)
  {
#if defined(IMPL_SSE2)
    xvec_d<float, n> r(nullptr);
    _mm_store_ps(r.t, _mm_div_ps(_mm_load_ps1(a.t), _mm_load_ps(t)));
    return r;
#endif
  }

  template <size_t n>
  xvec_d<float, n>::operator float() const requires (n == 1)
  {
    return t[0];
  }

  template <size_t n>
  xvec_d<float, n>::operator xvec<float, n>() const requires (n > 1)
  {
    xvec<float, n> r;
    for(size_t i = 0; i < n; ++i)
      r(i) = t[i];
    return r;
  }

  template <size_t n>
  xvec_d<float, n>::operator int32_t() const requires (n == 1)
  {
#if defined(IMPL_SSE2)
    return _mm_cvtt_ss2si(_mm_load_ss(t));
#endif
  }

  xquat_d<float> xquat_d<float>::operator-() const
  {
#if defined(IMPL_SSE2)
    xquat_d<float> r;
    _mm_store_ps(r.t, _mm_xor_ps(_mm_load_ps(t), _mm_set_epi32(0, 0x80000000, 0x80000000, 0x80000000)));
    return r;
#endif
  }

  xquat_d<float>::operator xquat<float>() const
  {
    return xquat<float>((xvec<float, 4>)*this);
  }

  template <>
  xquat_d<float> operator*(xquat_d<float> const& a, xquat_d<float> const& b)
  {
#if defined(IMPL_SSE2)
    xquat_d<float> r;
    auto av = _mm_load_ps(a.t);
    auto bv = _mm_load_ps(b.t);
    _mm_store_ps(r.t, _mm_xor_ps(
      _mm_add_ps(
        _mm_add_ps(
          _mm_mul_ps(
            _mm_shuffle_ps(av, av, _MM_SHUFFLE(0, 3, 3, 3)),
            _mm_shuffle_ps(bv, bv, _MM_SHUFFLE(0, 2, 1, 0))
          ),
          _mm_mul_ps(
            _mm_shuffle_ps(av, av, _MM_SHUFFLE(1, 0, 1, 0)),
            _mm_shuffle_ps(bv, bv, _MM_SHUFFLE(1, 1, 3, 3))
          )
        ),
        _mm_sub_ps(
          _mm_mul_ps(
            _mm_shuffle_ps(av, av, _MM_SHUFFLE(2, 2, 2, 1)),
            _mm_shuffle_ps(bv, bv, _MM_SHUFFLE(2, 3, 0, 2))
          ),
          _mm_mul_ps(
            _mm_shuffle_ps(av, av, _MM_SHUFFLE(3, 1, 0, 2)),
            _mm_shuffle_ps(bv, bv, _MM_SHUFFLE(3, 0, 2, 1))
          )
        )
      ),
      _mm_set_epi32(0x80000000, 0, 0, 0)
    ));
    return r;
#endif
  }

  template <>
  xvec_d<float, 1> dot(xvec_d<float, 2> const& a, xvec_d<float, 2> const& b)
  {
#if defined(IMPL_SSE2)
    xvec_d<float, 1> r(nullptr);
    auto p = _mm_mul_ps(_mm_load_ps(a.t), _mm_load_ps(b.t));
    _mm_store_ss(r.t, _mm_add_ss(p, _mm_shuffle_ps(p, p, _MM_SHUFFLE(0, 0, 0, 1))));
    return r;
#endif
  }
  template <>
  xvec_d<float, 1> dot(xvec_d<float, 3> const& a, xvec_d<float, 3> const& b)
  {
#if defined(IMPL_SSE2)
    xvec_d<float, 1> r(nullptr);
    auto p = _mm_mul_ps(_mm_load_ps(a.t), _mm_load_ps(b.t));
    _mm_store_ss(r.t, _mm_add_ss(
      _mm_add_ss(p, _mm_shuffle_ps(p, p, _MM_SHUFFLE(0, 0, 0, 1))),
      _mm_shuffle_ps(p, p, _MM_SHUFFLE(0, 0, 0, 2))
    ));
    return r;
#endif
  }
  template <>
  xvec_d<float, 1> dot(xvec_d<float, 4> const& a, xvec_d<float, 4> const& b)
  {
#if defined(IMPL_SSE2)
    xvec_d<float, 1> r(nullptr);
    auto p = _mm_mul_ps(_mm_load_ps(a.t), _mm_load_ps(b.t));
    p = _mm_add_ps(p, _mm_shuffle_ps(p, p, _MM_SHUFFLE(0, 0, 3, 2)));
    _mm_store_ss(r.t, _mm_add_ss(p, _mm_shuffle_ps(p, p, _MM_SHUFFLE(0, 0, 0, 1))));
    return r;
#endif
  }

  template <typename T, size_t n>
  requires (n > 1)
  xvec_d<T, 1> length2(xvec_d<T, n> const& a)
  {
    return dot(a, a);
  }

  template <typename T, size_t n>
  requires (n > 1)
  xvec_d<T, 1> length(xvec_d<T, n> const& a)
  {
    return sqrt(length2(a));
  }

  template <typename T, size_t n>
  requires (n > 1)
  xvec_d<T, n> normalize(xvec_d<T, n> const& a)
  {
    return a / length(a);
  }

  template <>
  xvec_d<float, 3> cross(xvec_d<float, 3> const& a, xvec_d<float, 3> const& b)
  {
#if defined(IMPL_SSE2)
    auto av = _mm_load_ps(a.t);
    auto bv = _mm_load_ps(b.t);
    xvec_d<float, 3> r(nullptr);
    _mm_store_ps(r.t, _mm_sub_ps(
      _mm_mul_ps(
        _mm_shuffle_ps(av, av, _MM_SHUFFLE(3, 0, 2, 1)),
        _mm_shuffle_ps(bv, bv, _MM_SHUFFLE(3, 1, 0, 2))
      ),
      _mm_mul_ps(
        _mm_shuffle_ps(av, av, _MM_SHUFFLE(3, 1, 0, 2)),
        _mm_shuffle_ps(bv, bv, _MM_SHUFFLE(3, 0, 2, 1))
      )
    ));
    return r;
#endif
  }

  template <>
  xvec_d<float, 1> abs(xvec_d<float, 1> const& a)
  {
#if defined(IMPL_SSE2)
    xvec_d<float, 1> r(nullptr);
    _mm_store_ss(r.t, _mm_and_ps(_mm_load_ss(a.t), _mm_set_epi32(0, 0, 0, 0x7fffffff)));
    return r;
#endif
  }
  template <>
  xvec_d<float, 1> floor(xvec_d<float, 1> const& a)
  {
#if defined(IMPL_SSE2)
    auto p = _mm_load_ss(a.t);
    auto q = _mm_cvtepi32_ps(_mm_cvttps_epi32(p));
    xvec_d<float, 1> r(nullptr);
    _mm_store_ss(r.t, _mm_sub_ps(q, _mm_and_ps(_mm_cmplt_ps(p, q), _mm_set_ss(1.0f))));
    return r;
#endif
  }
  template <>
  xvec_d<float, 1> ceil(xvec_d<float, 1> const& a)
  {
#if defined(IMPL_SSE2)
    auto p = _mm_load_ss(a.t);
    auto q = _mm_cvtepi32_ps(_mm_cvttps_epi32(p));
    xvec_d<float, 1> r(nullptr);
    _mm_store_ss(r.t, _mm_add_ps(q, _mm_and_ps(_mm_cmpgt_ps(p, q), _mm_set_ss(1.0f))));
    return r;
#endif
  }
  template <>
  xvec_d<float, 1> trunc(xvec_d<float, 1> const& a)
  {
#if defined(IMPL_SSE2)
    xvec_d<float, 1> r(nullptr);
    _mm_store_ss(r.t, _mm_cvtepi32_ps(_mm_cvttps_epi32(_mm_load_ss(a.t))));
    return r;
#endif
  }
  template <>
  xvec_d<float, 1> sqrt(xvec_d<float, 1> const& a)
  {
#if defined(IMPL_SSE2)
    xvec_d<float, 1> r(nullptr);
    _mm_store_ss(r.t, _mm_sqrt_ss(_mm_load_ss(a.t)));
    return r;
#endif
  }
  template <typename T>
  xvec_d<T, 1> sin(xvec_d<T, 1> a)
  {
    using scalar = xvec_d<T, 1>;

    // handle negative values
    if(a < scalar{}) return -sin(-a);

    // reduce to range [0, pi]
    scalar n;
    if(a > pi_d<T>)
    {
      n = trunc(a / pi_d<T>);
      a -= n * pi_d<T>;
    }
    // reduce to range [0, pi/2]
    if(a > pi_half_d<T>) a = pi_d<T> - a;

    static scalar const
      k1 = scalar(0.99976073735983227f),
      k3 = scalar(-0.16580121984779175f),
      k5 = scalar(0.00756279111686865f);

    scalar
      a2 = a * a,
      a3 = a2 * a,
      a5 = a3 * a2;

    a = a * k1 + a3 * k3 + a5 * k5;

    return ((int32_t)n % 2 == 0) ? a : -a;
  }
  template <typename T>
  xvec_d<T, 1> cos(xvec_d<T, 1> const& a)
  {
    return sin(a + pi_half_d<T>);
  }

  template <typename T>
  xvec_d<T, 1> const pi_d = xvec_d<T, 1>(std::numbers::pi_v<T>);
  template <typename T>
  xvec_d<T, 1> const pi_half_d = pi_d<T> / xvec_d<T, 1>(2);

  // explicit instantiations

  template class xvec_d<float, 1>;
  template class xvec_d<float, 2>;
  template class xvec_d<float, 3>;
  template class xvec_d<float, 4>;
  template class xquat_d<float>;

  template xvec_d<float, 1> length2(xvec_d<float, 2> const& a);
  template xvec_d<float, 1> length2(xvec_d<float, 3> const& a);
  template xvec_d<float, 1> length2(xvec_d<float, 4> const& a);
  template xvec_d<float, 1> length(xvec_d<float, 2> const& a);
  template xvec_d<float, 1> length(xvec_d<float, 3> const& a);
  template xvec_d<float, 1> length(xvec_d<float, 4> const& a);
  template xvec_d<float, 2> normalize(xvec_d<float, 2> const& a);
  template xvec_d<float, 3> normalize(xvec_d<float, 3> const& a);
  template xvec_d<float, 4> normalize(xvec_d<float, 4> const& a);
  template xvec_d<float, 1> sin(xvec_d<float, 1> a);
  template xvec_d<float, 1> cos(xvec_d<float, 1> const& a);

  template xvec_d<float, 1> const pi_d<float>;
  template xvec_d<float, 1> const pi_half_d<float>;
}
