module;

#include <concepts>
#include <condition_variable>
#include <coroutine>
#include <exception>
#include <latch>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>
#include <type_traits>
#include <variant>
#include <vector>

export module coil.core.tasks;

import coil.core.base;

export namespace Coil
{
  template <typename R>
  class [[nodiscard]] Task;

  template <typename R>
  class TaskPromise;

  // singleton class, controls thread pool
  class TaskEngine
  {
  private:
    TaskEngine() = default;
    ~TaskEngine()
    {
      // stop threads first; this will wait for them to end
      _threads.clear();

      // destroy all remaining tasks, as they won't destroy themselves
      while(!_coroutines.empty())
      {
        _coroutines.front().destroy();
        _coroutines.pop();
      }
    }

  public:
    // schedule coroutine to run
    void Queue(std::coroutine_handle<> coroutine)
    {
      std::unique_lock lock(_mutex);
      _coroutines.push(coroutine);
      lock.unlock();

      _cv.notify_one();
    }

    // add one thread to pool
    void AddThread()
    {
      _threads.emplace_back([this](std::stop_token const& stopToken)
      {
        for(;;)
        {
          std::unique_lock lock(_mutex);
          if(!_cv.wait(lock, stopToken, [&]()
          {
            return !_coroutines.empty();
          })) break;
          std::coroutine_handle<> coroutine = _coroutines.front();
          _coroutines.pop();
          lock.unlock();

          coroutine.resume();
        }
      });
    }
    // add numbers of threads calculated from number of hardware cores
    void AddThreads(size_t reserveThreadsCount = 1, size_t minimumThreadsCount = 1)
    {
      auto threadsCount = std::max((size_t)std::jthread::hardware_concurrency() - reserveThreadsCount, minimumThreadsCount);
      for(size_t i = 0; i < threadsCount; ++i)
        AddThread();
    }
    // run coroutines in the current thread only until there are any
    void Run()
    {
      for(;;)
      {
        std::unique_lock lock(_mutex);
        if(_coroutines.empty()) break;
        std::coroutine_handle<> coroutine = _coroutines.front();
        _coroutines.pop();
        lock.unlock();

        coroutine.resume();
      }
    }

    static TaskEngine& GetInstance()
    {
      static TaskEngine instance;
      return instance;
    }

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
      bool await_ready() const
      {
        return false;
      }
      void await_suspend(std::coroutine_handle<> const& coroutine) const
      {
        TaskEngine::GetInstance().Queue(coroutine);
      }
      void await_resume() const
      {
      }
    };

    // final awaiter class
    // destroys coroutine after final suspend
    class FinalAwaiter
    {
    public:
      bool await_ready() const noexcept
      {
        return false;
      }
      void await_suspend(std::coroutine_handle<> const& coroutine) const noexcept
      {
        coroutine.destroy();
      }
      void await_resume() const noexcept
      {
      }
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
      return {};
    }
    FinalAwaiter final_suspend() noexcept
    {
      return {};
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

      void SetValue(Value const& value)
      {
        SetResult([&]()
        {
          _result.emplace(value);
        });
      }
      void SetValue(Value&& value)
      {
        SetResult([&]()
        {
          _result.emplace(std::move(value));
        });
      }

      void SetException()
      {
        SetResult([&]()
        {
          _result.emplace(std::current_exception());
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
        return std::visit([&]<typename V>(V const& result) -> R
        {
          if constexpr(std::same_as<V, Value>)
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
    void return_value(R const& value)
    {
      this->_pResult->SetValue(value);
    }
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
