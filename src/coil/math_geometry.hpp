#pragma once

#include "math.hpp"

namespace Coil
{
  template <typename T>
  constexpr xmat<T, 4, 4> AffineTranslation(xvec<T, 3> const& t)
  {
    return
    {
      1,  0,  0,  t.x(),
      0,  1,  0,  t.y(),
      0,  0,  1,  t.z(),
      0,  0,  0,  1,
    };
  }

  template <typename T>
  constexpr xmat<T, 4, 4> AffineScaling(xvec<T, 3> const& s)
  {
    return
    {
      s.x(),  0,      0,      0,
      0,      s.y(),  0,      0,
      0,      0,      s.z(),  0,
      0,      0,      0,      1,
    };
  }

  // View matrix for looking at a target.
  // Assumes upper-left screen origin, and positive Z towards target.
  template <typename T>
  constexpr xmat<T, 4, 4> AffineViewLookAt(xvec<T, 3> const& eye, xvec<T, 3> const& target, xvec<T, 3> const& up)
  {
    xvec<T, 3> z = normalize(target - eye);
    xvec<T, 3> x = normalize(cross(z, up));
    xvec<T, 3> y = cross(z, x);
    return
    {
      x.x(),  x.y(),  x.z(),  -dot(x, eye),
      y.x(),  y.y(),  y.z(),  -dot(y, eye),
      z.x(),  z.y(),  z.z(),  -dot(z, eye),
      0,      0,      0,      1,
    };
  }

  // Orthographic projection matrix.
  // width, height - view-space size of screen
  // z0, z1 - view-space Z mapped to 0 and 1 respectively
  template <typename T>
  constexpr xmat<T, 4, 4> ProjectionOrtho(T w, T h, T z0, T z1)
  {
    return
    {
      2 / w,  0,      0,              0,
      0,      2 / h,  0,              0,
      0,      0,      1 / (z1 - z0),  z0 / (z0 - z1),
      0,      0,      0,              1,
    };
  }

  // Perspective projection matrix.
  // Assumes view-space positive Z towards target.
  template <typename T>
  constexpr xmat<T, 4, 4> ProjectionPerspectiveFov(T fovY, T aspect, T z0, T z1)
  {
    T ys = 1 / std::tan(fovY / 2);
    T xs = ys / aspect;
    return
    {
      xs,  0,   0,               0,
      0,   ys,  0,               0,
      0,   0,   z1 / (z1 - z0),  z0 * z1 / (z0 - z1),
      0,   0,   1,               0,
    };
  }

  // Quaternion representing rotation around axis.
  template <typename T>
  constexpr xquat<T> QuatAxisRotation(xvec<T, 3> const& axis, T angle)
  {
    T halfAngle = angle / 2;
    T sinHalfAngle = std::sin(halfAngle);
    T cosHalfAngle = std::cos(halfAngle);
    return
    {
      axis.x() * sinHalfAngle,
      axis.y() * sinHalfAngle,
      axis.z() * sinHalfAngle,
      cosHalfAngle,
    };
  }

  // Convert quaternion to matrix.
  template <typename T>
  constexpr xmat<T, 4, 4> AffineFromQuat(xquat<T> const& q)
  {
    T ww  = q.w() * q.w();
    T xx  = q.x() * q.x();
    T yy  = q.y() * q.y();
    T zz  = q.z() * q.z();
    T wx2 = q.w() * q.x() * 2;
    T wy2 = q.w() * q.y() * 2;
    T wz2 = q.w() * q.z() * 2;
    T xy2 = q.x() * q.y() * 2;
    T xz2 = q.x() * q.z() * 2;
    T yz2 = q.y() * q.z() * 2;
    return
    {
      (ww + xx - yy - zz), (xy2 - wz2),         (xz2 + wy2),         0,
      (xy2 + wz2),         (ww - xx + yy - zz), (yz2 - wx2),         0,
      (xz2 - wy2),         (yz2 + wx2),         (ww - xx - yy + zz), 0,
      0,                   0,                   0,                   1,
    };
  }
}
