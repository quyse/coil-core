#pragma once

#include <coroutine>
#include <optional>

namespace Coil
{
  // Generator coroutine.
  template <typename T>
  class Generator
  {
  public:
    class promise_type
    {
    public:
      std::suspend_always initial_suspend()
      {
        return {};
      }
      std::suspend_always final_suspend() noexcept
      {
        return {};
      }
      Generator get_return_object()
      {
        return std::coroutine_handle<promise_type>::from_promise(*this);
      }
      void unhandled_exception()
      {
      }
      std::suspend_always yield_value(T&& value)
      {
        _value = std::move(value);
        return {};
      }
      void return_void() {}

    private:
      std::optional<T> _value;

      friend Generator;
    };

    std::optional<T> operator()() const
    {
      if(_coroutine.done()) return {};
      _coroutine();
      return _coroutine.promise()._value;
    }

  private:
    Generator(std::coroutine_handle<promise_type> coroutine)
    : _coroutine(coroutine) {}

    Generator(Generator const&) = delete;
    Generator(Generator&&) = default;
    Generator& operator=(Generator const&) = delete;
    Generator& operator=(Generator&&) = default;

  public:
    ~Generator()
    {
      _coroutine.destroy();
    }

  private:
    std::coroutine_handle<promise_type> const _coroutine;
  };
}
