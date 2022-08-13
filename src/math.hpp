#pragma once

namespace Coil
{
  template <typename T, size_t n>
  struct xvec
  {
    T t[n];

    xvec() : t({}) {}

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
  struct xvec<T, 2>
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
  struct xvec<T, 3>
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
  struct xvec<T, 4>
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
    T t[n][m];

    xmat() : t({}) {}

    T& operator()(size_t i, size_t j)
    {
      return t[i][j];
    }

    T operator()(size_t i, size_t j) const
    {
      return t[i][j];
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

  template <typename T>
  using xquat = xvec<T, 4>;
  using quat = xquat<float>;
  using dquat = xquat<double>;
}
