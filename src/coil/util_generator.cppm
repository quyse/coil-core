module;

#include <coroutine>
#include <iterator>
#include <optional>

export module coil.core.util.generator;

export namespace Coil
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
      std::suspend_always yield_value(T const& value)
      {
        _value = value;
        return {};
      }
      void return_void() {}

    private:
      std::optional<T> _value;

      friend Generator;
    };

    std::optional<T> operator()() const
    {
      auto& promise = _coroutine.promise();
      promise._value = {};
      if(!_coroutine.done())
      {
        _coroutine();
      }
      return promise._value;
    }

    Generator(Generator const&) = delete;
    Generator(Generator&& other)
    {
      *this = std::move(other);
    }
    Generator& operator=(Generator const&) = delete;
    Generator& operator=(Generator&& other)
    {
      std::swap(_coroutine, other._coroutine);
      return *this;
    }

    ~Generator()
    {
      if(_coroutine)
      {
        _coroutine.destroy();
      }
    }

    class Iterator
    {
    public:
      Iterator(Generator const& generator)
      : _pGenerator(&generator)
      {
        Step();
      }
      // sentinel
      Iterator() = default;
      Iterator(Iterator const&) = default;
      Iterator(Iterator&&) = default;
      Iterator& operator=(Iterator const&) = default;
      Iterator& operator=(Iterator&&) = default;

      Iterator& operator++()
      {
        Step();
        return *this;
      }
      Iterator operator++(int)
      {
        Iterator i = *this;
        Step();
        return std::move(i);
      }

      T const& operator*() const
      {
        return _value.value();
      }

      friend bool operator==(Iterator const& a, Iterator const& b)
      {
        return a._value.has_value() == b._value.has_value();
      }

      using difference_type = ptrdiff_t;
      using value_type = T;
      using pointer = value_type*;
      using reference = value_type&;
      using iterator_category = std::forward_iterator_tag;
      using iterator_concept = std::forward_iterator_tag;

    private:
      void Step()
      {
        _value = (*_pGenerator)();
      }

      Generator const* _pGenerator = nullptr;
      std::optional<T> _value;
    };

    // range methods
    Iterator begin() const
    {
      return *this;
    }
    Iterator end() const
    {
      return {};
    }

  private:
    Generator(std::coroutine_handle<promise_type> coroutine)
    : _coroutine(coroutine) {}

    std::coroutine_handle<promise_type> _coroutine;
  };
}
