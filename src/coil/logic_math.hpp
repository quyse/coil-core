#pragma once

#include "math.hpp"

namespace Coil
{
  // soften changes
  // slows down speed exponentially (with negative coefficient)
  template <typename T, typename S = typename VectorTraits<T>::Scalar>
  class Softener
  {
  public:
    Softener(S slowCoef, S maxSpeed)
    : _slowCoef(slowCoef), _maxSpeed(maxSpeed) {}

    T GetSpeed() const
    {
      return _speed;
    }

    void ApplyIntent(T const& diff)
    {
      _speed = _speed + diff;
    }

    void Tick(S time)
    {
      // slow down exponentialy
      _speed = _speed * std::exp(time * _slowCoef);
      // limit
      if constexpr(std::is_same_v<T, S>)
      {
        if(std::abs(_speed) > _maxSpeed)
        {
          _speed = _speed * (_maxSpeed / abs(_speed));
        }
      }
      else
      {
        S speedLen = length(_speed);
        if(speedLen > _maxSpeed)
        {
          _speed = _speed * (_maxSpeed / speedLen);
        }
      }
    }

  private:
    T _speed = {};
    S const _slowCoef;
    S const _maxSpeed;
  };
}
