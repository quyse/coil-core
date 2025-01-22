module;

// for now we only support SSE2

#define IMPL_SSE2

#include <initializer_list>
#include <iterator>
#include <numbers>

#if defined(IMPL_SSE2)
#include <emmintrin.h>
#else
#error "no deterministic math implementation"
#endif

export module coil.core.math.determ;

import coil.core.math;

export namespace Coil
{
  template <typename T, size_t n>
  struct xvec_d;

  template <size_t n>
  struct alignas((n <= 1 ? 1 : 4) * sizeof(float)) xvec_d<float, n>
  {
    float t[n <= 1 ? 1 : 4];

    using scalar = xvec_d<float, 1>;

    xvec_d(std::nullptr_t) {} // uninitialized
    xvec_d()
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
    xvec_d(xvec_d const& other)
    {
      *this = other;
    }
    xvec_d& operator=(xvec_d const& other)
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

    // xvec_d(scalar const& x) requires (n == 1); // duplicates copy constructor
    xvec_d(scalar const& x, scalar const& y) requires (n == 2)
    {
#if defined(IMPL_SSE2)
      _mm_store_ps(t, _mm_unpacklo_ps(_mm_load_ss(x.t), _mm_load_ss(y.t)));
#endif
    }
    xvec_d(scalar const& x, scalar const& y, scalar const& z) requires (n == 3)
    {
#if defined(IMPL_SSE2)
      _mm_store_ps(t, _mm_unpacklo_ps(
        _mm_unpacklo_ps(_mm_load_ss(x.t), _mm_load_ss(z.t)),
        _mm_load_ss(y.t)
      ));
#endif
    }
    xvec_d(scalar const& x, scalar const& y, scalar const& z, scalar const& w) requires (n == 4)
    {
#if defined(IMPL_SSE2)
      _mm_store_ps(t, _mm_unpacklo_ps(
        _mm_unpacklo_ps(_mm_load_ss(x.t), _mm_load_ss(z.t)),
        _mm_unpacklo_ps(_mm_load_ss(y.t), _mm_load_ss(w.t))
      ));
#endif
    }

    // explicit constructors from non-deterministic values
    explicit xvec_d(float x) requires (n == 1)
    {
      t[0] = x;
    }
    explicit xvec_d(xvec<float, n> const& other) requires (n > 1)
    {
      for(size_t i = 0; i < n; ++i)
        t[i] = other.t[i];
    }
    explicit xvec_d(std::initializer_list<float> const& list)
    {
      float const* data = std::data(list);
      size_t k = 0;
      size_t listSize = list.size();
      for(size_t i = 0; i < n; ++i)
        t[i] = k < listSize ? data[k++] : float{};
    }
    explicit xvec_d(int32_t x) requires (n == 1)
    {
      _mm_store_ss(t, _mm_cvt_si2ss(_mm_setzero_ps(), x));
    }

    // element access
    scalar x() const
    {
#if defined(IMPL_SSE2)
      scalar r(nullptr);
      _mm_store_ss(r.t, _mm_load_ss(t));
      return r;
#endif
    }
    scalar y() const requires (1 < n)
    {
#if defined(IMPL_SSE2)
      scalar r(nullptr);
      auto v = _mm_load_ps(t);
      _mm_store_ss(r.t, _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 1)));
      return r;
#endif
    }
    scalar z() const requires (2 < n)
    {
#if defined(IMPL_SSE2)
      scalar r(nullptr);
      auto v = _mm_load_ps(t);
      _mm_store_ss(r.t, _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 2)));
      return r;
#endif
    }
    scalar w() const requires (3 < n)
    {
#if defined(IMPL_SSE2)
      scalar r(nullptr);
      auto v = _mm_load_ps(t);
      _mm_store_ss(r.t, _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 3)));
      return r;
#endif
    }
    void x(scalar const& v)
    {
#if defined(IMPL_SSE2)
      _mm_store_ss(t, _mm_load_ss(v.t));
#endif
    }
    void y(scalar const& v) requires (1 < n)
    {
#if defined(IMPL_SSE2)
      _mm_store_ss(t + 1, _mm_load_ss(v.t));
#endif
    }
    void z(scalar const& v) requires (2 < n)
    {
#if defined(IMPL_SSE2)
      _mm_store_ss(t + 2, _mm_load_ss(v.t));
#endif
    }
    void w(scalar const& v) requires (3 < n)
    {
#if defined(IMPL_SSE2)
      _mm_store_ss(t + 3, _mm_load_ss(v.t));
#endif
    }

    bool operator==(xvec_d const& b) const
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
    bool operator!=(xvec_d const& b) const
    {
      return !(*this == b);
    }
    bool operator<(xvec_d const& b) const
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
    bool operator<=(xvec_d const& b) const
    {
      return !(b < *this);
    }
    bool operator>(xvec_d const& b) const
    {
      return b < *this;
    }
    bool operator>=(xvec_d const& b) const
    {
      return !(*this < b);
    }

    xvec_d operator-() const
    {
      return xvec_d() - *this;
    }

    // elementwise addition
    xvec_d& operator+=(xvec_d const& b)
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
    // elementwise subtraction
    xvec_d& operator-=(xvec_d const& b)
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
    // elementwise multiplication
    xvec_d& operator*=(xvec_d const& b)
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
    // vector-scalar multiplication
    xvec_d& operator*=(scalar const& b) requires (n > 1)
    {
#if defined(IMPL_SSE2)
      _mm_store_ps(t, _mm_mul_ps(_mm_load_ps(t), _mm_load_ps1(b.t)));
#endif
      return *this;
    }
    // scalar-vector multiplication
    xvec_d mul_reverse(scalar const& a) const requires (n > 1)
    {
#if defined(IMPL_SSE2)
      xvec_d<float, n> r(nullptr);
      _mm_store_ps(r.t, _mm_mul_ps(_mm_load_ps1(a.t), _mm_load_ps(t)));
      return r;
#endif
    }
    // elementwise division
    xvec_d& operator/=(xvec_d const& b)
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
    // vector-scalar division
    xvec_d& operator/=(scalar const& b) requires (n > 1)
    {
#if defined(IMPL_SSE2)
      _mm_store_ps(t, _mm_div_ps(_mm_load_ps(t), _mm_load_ps1(b.t)));
#endif
      return *this;
    }
    // scalar-vector division
    xvec_d div_reverse(scalar const& a) const requires (n > 1)
    {
#if defined(IMPL_SSE2)
      xvec_d<float, n> r(nullptr);
      _mm_store_ps(r.t, _mm_div_ps(_mm_load_ps1(a.t), _mm_load_ps(t)));
      return r;
#endif
    }

    explicit operator float() const requires (n == 1)
    {
      return t[0];
    }
    explicit operator xvec<float, n>() const requires (n > 1)
    {
      xvec<float, n> r;
      for(size_t i = 0; i < n; ++i)
        r(i) = t[i];
      return r;
    }
    explicit operator int32_t() const requires (n == 1)
    {
#if defined(IMPL_SSE2)
      return _mm_cvtt_ss2si(_mm_load_ss(t));
#endif
    }
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
    xquat_d operator-() const
    {
#if defined(IMPL_SSE2)
      xquat_d<float> r;
      _mm_store_ps(r.t, _mm_xor_ps(_mm_load_ps(t), _mm_set_epi32(0, 0x80000000, 0x80000000, 0x80000000)));
      return r;
#endif
    }

    explicit operator xquat<float>() const
    {
      return xquat<float>((xvec<float, 4>)*this);
    }
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

  // constants
  template <typename T>
  xvec_d<T, 1> const pi_d = xvec_d<T, 1>(std::numbers::pi_v<T>);
  template <typename T>
  xvec_d<T, 1> const pi_half_d = pi_d<T> / xvec_d<T, 1>(2);

  // operations
  template <typename T, size_t n> requires (n > 1) xvec_d<T, 1> dot(xvec_d<T, n> const& a, xvec_d<T, n> const& b);
  template <typename T, size_t n> requires (n > 1) xvec_d<T, 1> length2(xvec_d<T, n> const& a)
  {
    return dot(a, a);
  }
  template <typename T, size_t n> requires (n > 1) xvec_d<T, 1> length(xvec_d<T, n> const& a)
  {
    return sqrt(length2(a));
  }
  template <typename T, size_t n> requires (n > 1) xvec_d<T, n> normalize(xvec_d<T, n> const& a)
  {
    return a / length(a);
  }
  template <typename T> xvec_d<T, 3> cross(xvec_d<T, 3> const& a, xvec_d<T, 3> const& b);
  template <typename T> xvec_d<T, 1> abs(xvec_d<T, 1> const& a);
  template <typename T> xvec_d<T, 1> floor(xvec_d<T, 1> const& a);
  template <typename T> xvec_d<T, 1> ceil(xvec_d<T, 1> const& a);
  template <typename T> xvec_d<T, 1> trunc(xvec_d<T, 1> const& a);
  template <typename T> xvec_d<T, 1> sqrt(xvec_d<T, 1> const& a);
  template <typename T> xvec_d<T, 1> sin(xvec_d<T, 1> a)
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
  template <typename T> xvec_d<T, 1> cos(xvec_d<T, 1> const& a)
  {
    return sin(a + pi_half_d<T>);
  }
  template <typename T> xquat_d<T> operator*(xquat_d<T> const& a, xquat_d<T> const& b);
  // operation specializations
  template <> xvec_d<float, 1> dot(xvec_d<float, 2> const& a, xvec_d<float, 2> const& b)
  {
#if defined(IMPL_SSE2)
    xvec_d<float, 1> r(nullptr);
    auto p = _mm_mul_ps(_mm_load_ps(a.t), _mm_load_ps(b.t));
    _mm_store_ss(r.t, _mm_add_ss(p, _mm_shuffle_ps(p, p, _MM_SHUFFLE(0, 0, 0, 1))));
    return r;
#endif
  }
  template <> xvec_d<float, 1> dot(xvec_d<float, 3> const& a, xvec_d<float, 3> const& b)
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
  template <> xvec_d<float, 1> dot(xvec_d<float, 4> const& a, xvec_d<float, 4> const& b)
  {
#if defined(IMPL_SSE2)
    xvec_d<float, 1> r(nullptr);
    auto p = _mm_mul_ps(_mm_load_ps(a.t), _mm_load_ps(b.t));
    p = _mm_add_ps(p, _mm_shuffle_ps(p, p, _MM_SHUFFLE(0, 0, 3, 2)));
    _mm_store_ss(r.t, _mm_add_ss(p, _mm_shuffle_ps(p, p, _MM_SHUFFLE(0, 0, 0, 1))));
    return r;
#endif
  }
  template <> xvec_d<float, 3> cross(xvec_d<float, 3> const& a, xvec_d<float, 3> const& b)
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
  template <> xvec_d<float, 1> abs(xvec_d<float, 1> const& a)
  {
#if defined(IMPL_SSE2)
    xvec_d<float, 1> r(nullptr);
    _mm_store_ss(r.t, _mm_and_ps(_mm_load_ss(a.t), _mm_set_epi32(0, 0, 0, 0x7fffffff)));
    return r;
#endif
  }
  template <> xvec_d<float, 1> floor(xvec_d<float, 1> const& a)
  {
#if defined(IMPL_SSE2)
    auto p = _mm_load_ss(a.t);
    auto q = _mm_cvtepi32_ps(_mm_cvttps_epi32(p));
    xvec_d<float, 1> r(nullptr);
    _mm_store_ss(r.t, _mm_sub_ps(q, _mm_and_ps(_mm_cmplt_ps(p, q), _mm_set_ss(1.0f))));
    return r;
#endif
  }
  template <> xvec_d<float, 1> ceil(xvec_d<float, 1> const& a)
  {
#if defined(IMPL_SSE2)
    auto p = _mm_load_ss(a.t);
    auto q = _mm_cvtepi32_ps(_mm_cvttps_epi32(p));
    xvec_d<float, 1> r(nullptr);
    _mm_store_ss(r.t, _mm_add_ps(q, _mm_and_ps(_mm_cmpgt_ps(p, q), _mm_set_ss(1.0f))));
    return r;
#endif
  }
  template <> xvec_d<float, 1> trunc(xvec_d<float, 1> const& a)
  {
#if defined(IMPL_SSE2)
    xvec_d<float, 1> r(nullptr);
    _mm_store_ss(r.t, _mm_cvtepi32_ps(_mm_cvttps_epi32(_mm_load_ss(a.t))));
    return r;
#endif
  }
  template <> xvec_d<float, 1> sqrt(xvec_d<float, 1> const& a)
  {
#if defined(IMPL_SSE2)
    xvec_d<float, 1> r(nullptr);
    _mm_store_ss(r.t, _mm_sqrt_ss(_mm_load_ss(a.t)));
    return r;
#endif
  }
  template <> xquat_d<float> operator*(xquat_d<float> const& a, xquat_d<float> const& b)
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

  // explicit instantiations

  template class xvec_d<float, 1>;
  template class xvec_d<float, 2>;
  template class xvec_d<float, 3>;
  template class xvec_d<float, 4>;

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

  // convenience synonyms
  using float_d = xvec_d<float, 1>;
  using vec1_d = float_d;
  using vec2_d = xvec_d<float, 2>;
  using vec3_d = xvec_d<float, 3>;
  using vec4_d = xvec_d<float, 4>;
  using quat_d = xquat_d<float>;
}
