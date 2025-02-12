module;

#include <algorithm>
#include <cmath>

export module coil.core.logic.math;

import coil.core.math;

export namespace Coil
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
      if constexpr(std::same_as<T, S>)
      {
        if(std::abs(_speed) > _maxSpeed)
        {
          _speed = _speed * (_maxSpeed / std::abs(_speed));
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

  template <std::signed_integral I, typename T>
  class LogicSwitch
  {
  public:
    LogicSwitch(I maxValue, T switchSpeed)
    : _maxValue(maxValue), _switchSpeed(switchSpeed) {}

    T GetValue() const
    {
      return (T)_value + _subValue;
    }

    void ApplyIntent(I direction)
    {
      if(direction > 0 && _value >= _maxValue) return;
      if(direction < 0 && _value <= 0) return;
      _direction = direction;
    }

    void Tick(T time)
    {
      if(_direction > 0)
      {
        _subValue += _switchSpeed * time;
        if(_subValue >= 1)
        {
          _value = std::min(_value + 1, _maxValue);
          _subValue = {};
          _direction = 0;
        }
      }
      if(_direction < 0)
      {
        _subValue -= _switchSpeed * time;
        if(_subValue <= -1)
        {
          _value = std::max(_value - 1, 0);
          _subValue = {};
          _direction = 0;
        }
      }
    }

  private:
    I _value = 0;
    T _subValue = {};
    I _direction = 0;
    I const _maxValue;
    T const _switchSpeed;
  };
}
