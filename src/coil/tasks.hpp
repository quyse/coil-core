#pragma once

#include "base.hpp"
#include <coroutine>
#include <optional>
#include <memory>
#include <thread>
#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <variant>
#include <latch>
#include <type_traits>

namespace Coil
{
  template <typename R>
  class Task;

  template <typename R>
  class TaskPromise;

  // singleton class, controls thread pool
  class TaskEngine
  {
  private:
    TaskEngine() = default;
    ~TaskEngine();

  public:
    // schedule coroutine to run
    void Queue(std::coroutine_handle<> coroutine);

    // add one thread to pool
    void AddThread();
    // add numbers of threads calculated from number of hardware cores
    void AddThreads(size_t reserveThreadsCount = 1, size_t minimumThreadsCount = 1);
    // run coroutines in the current thread only until there are any
    void Run();

    static TaskEngine& GetInstance();

  private:
    std::mutex _mutex;
    std::condition_variable_any _cv;
    std::queue<std::coroutine_handle<>> _coroutines;
    std::vector<std::jthread> _threads;
  };

  // coroutine promise first base class
  class TaskPromiseBase1
  {
  protected:
    // initial awaiter class
    // queues task in the engine after initial suspend
    class InitialAwaiter
    {
    public:
      InitialAwaiter(std::coroutine_handle<>&& coroutine)
      : _coroutine(std::move(coroutine)) {}

      bool await_ready() const
      {
        return false;
      }
      void await_suspend(std::coroutine_handle<> const&) const
      {
        TaskEngine::GetInstance().Queue(_coroutine);
      }
      void await_resume() const
      {
      }

    private:
      std::coroutine_handle<> _coroutine;
    };

    // final awaiter class
    // destroys coroutine after final suspend
    class FinalAwaiter
    {
    public:
      FinalAwaiter(std::coroutine_handle<>&& coroutine)
      : _coroutine(std::move(coroutine)) {}

      bool await_ready() const noexcept
      {
        return false;
      }
      void await_suspend(std::coroutine_handle<> const&) const noexcept
      {
        _coroutine.destroy();
      }
      void await_resume() const noexcept
      {
      }

    private:
      std::coroutine_handle<> _coroutine;
    };
  };

  // coroutine promise second base class
  // implements everything except void/non-void distinction
  template <typename R>
  class TaskPromiseBase2 : public TaskPromiseBase1
  {
  public:
    InitialAwaiter initial_suspend()
    {
      return { std::coroutine_handle<TaskPromise<R>>::from_promise(static_cast<TaskPromise<R>&>(*this)) };
    }
    FinalAwaiter final_suspend() noexcept
    {
      return { std::coroutine_handle<TaskPromise<R>>::from_promise(static_cast<TaskPromise<R>&>(*this)) };
    }

    Task<R> get_return_object()
    {
      return Task<R>(_pResult);
    }

    void unhandled_exception()
    {
      _pResult->SetException();
    }

  protected:
    struct Void {};
    using Value = std::conditional_t<std::same_as<R, void>, Void, R>;

    // storage class
    // stores result so promise can be destroyed separately
    class Result
    {
    public:
      Result() : _latch(1) {}

      void SetValue(Value&& value)
      {
        SetResult([&]()
        {
          _result = std::move(value);
        });
      }

      void SetException()
      {
        SetResult([&]()
        {
          _result = std::current_exception();
        });
      }

      template <typename F>
      void SetResult(F const& f)
      {
        // set result, and extract list of listeners
        std::unique_lock lock(_mutex);
        f();
        std::vector<std::coroutine_handle<>> listeners;
        std::swap(listeners, _listeners);
        lock.unlock();

        // notify listeners
        for(size_t i = 0; i < listeners.size(); ++i)
          TaskEngine::GetInstance().Queue(listeners[i]);
        // unsnap latch
        _latch.count_down();
      }

      bool AwaitReady() const
      {
        // skip suspend if there's value already
        std::unique_lock lock(_mutex);
        return _result.has_value();
      }

      bool AwaitSuspend(std::coroutine_handle<> coroutine)
      {
        // unsuspend if there's value already
        // otherwise add current coroutine to listeners
        std::unique_lock lock(_mutex);
        if(_result.has_value()) return false;
        _listeners.push_back(coroutine);
        return true;
      }

      R AwaitResume() const
      {
        // return value or rethrow exception
        return std::visit([&](auto const& result) -> R
        {
          if constexpr(std::same_as<std::decay_t<decltype(result)>, Value>)
          {
            if constexpr(!std::same_as<R, void>)
            {
              return result;
            }
          }
          else
          {
            std::rethrow_exception(result);
          }
        }, _result.value());
      }

      // wait for result
      R Get() const
      {
        _latch.wait();
        return AwaitResume();
      }

    private:
      std::latch _latch;
      mutable std::mutex _mutex;
      std::optional<std::variant<Value, std::exception_ptr>> _result;
      std::vector<std::coroutine_handle<>> _listeners;
    };

    // task awaiter class
    // just wraps shared_ptr to result class
    class ResultAwaiter
    {
    public:
      ResultAwaiter(std::shared_ptr<Result> const& pResult)
      : _pResult(pResult) {}

      bool await_ready() const
      {
        return _pResult->AwaitReady();
      }

      bool await_suspend(std::coroutine_handle<> coroutine)
      {
        return _pResult->AwaitSuspend(coroutine);
      }

      R await_resume() const
      {
        return _pResult->AwaitResume();
      }

    private:
      std::shared_ptr<Result> _pResult;
    };

    std::shared_ptr<Result> _pResult = std::make_shared<Result>();

    friend class Task<R>;
  };

  // non-void task promise class
  template <typename R>
  class TaskPromise : public TaskPromiseBase2<R>
  {
  public:
    void return_value(R&& value)
    {
      this->_pResult->SetValue(std::move(value));
    }
  };
  // void task promise class
  template <>
  class TaskPromise<void> : public TaskPromiseBase2<void>
  {
  public:
    void return_void()
    {
      this->_pResult->SetValue({});
    }
  };

  // task class
  // just wraps shared_ptr to result class
  template <typename R>
  class Task
  {
  public:
    using promise_type = TaskPromise<R>;

    Task() = delete;

    typename TaskPromise<R>::ResultAwaiter operator co_await() const
    {
      return _pResult;
    }

    // block and wait for result
    R Get() const
    {
      return _pResult->Get();
    }

  private:
    explicit Task(std::shared_ptr<typename TaskPromise<R>::Result> const& pResult)
    : _pResult(pResult) {}

    std::shared_ptr<typename TaskPromise<R>::Result> _pResult;

    friend class TaskPromiseBase2<R>;
  };
}
