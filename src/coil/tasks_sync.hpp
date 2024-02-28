#pragma once

#include "tasks.hpp"
#include <memory>

namespace Coil
{
  // condition variable which works with tasks
  class ConditionVariable
  {
  public:
    ConditionVariable() = default;

    class WaitAwaiter
    {
    public:
      WaitAwaiter(ConditionVariable& cv, std::unique_lock<std::mutex>& userLock)
      : _cv{cv}, _userLock{userLock}, _lock{_cv._mutex}
      {
        _userLock.unlock();
      }

      bool await_ready() const
      {
        return false;
      }

      void await_suspend(std::coroutine_handle<> coroutine)
      {
        _cv._waiters.push(coroutine);
        _lock.unlock();
      }

      void await_resume() const
      {
        _userLock.lock();
      }

    private:
      ConditionVariable& _cv;
      std::unique_lock<std::mutex>& _userLock;
      std::unique_lock<std::mutex> _lock;
    };

    // unlocks lock, waits until notified, then locks the lock back
    // never suspends while holding a lock because it does not start
    // its own coroutine, but returns simple awaiter
    WaitAwaiter Wait(std::unique_lock<std::mutex>& userLock)
    {
      return {*this, userLock};
    }

    void NotifyOne()
    {
      std::unique_lock<std::mutex> lock{_mutex};
      if(_waiters.empty()) return;

      std::coroutine_handle<> coroutine = _waiters.front();
      _waiters.pop();
      lock.unlock();

      TaskEngine::GetInstance().Queue(coroutine);
    }

    void NotifyAll()
    {
      std::vector<std::coroutine_handle<>> waiters;

      std::unique_lock<std::mutex> lock{_mutex};
      if(_waiters.empty()) return;

      waiters.reserve(_waiters.size());
      while(!_waiters.empty())
      {
        waiters.push_back(_waiters.front());
        _waiters.pop();
      }
      lock.unlock();

      for(size_t i = 0; i < waiters.size(); ++i)
      {
        TaskEngine::GetInstance().Queue(waiters[i]);
      }
    }

  private:
    std::mutex _mutex;
    std::queue<std::coroutine_handle<>> _waiters;
  };

  class Semaphore
  {
  public:
    Semaphore(size_t initialCounter = 0)
    : _counter(initialCounter) {}

    Task<void> Acquire()
    {
      std::unique_lock lock{_mutex};
      while(!(_counter > 0))
      {
        co_await _cv.Wait(lock);
      }
      --_counter;
    }

    void Release(size_t increaseCounter = 1)
    {
      std::unique_lock lock{_mutex};
      _counter += increaseCounter;
      lock.unlock();
      for(size_t i = 0; i < increaseCounter; ++i)
      {
        _cv.NotifyOne();
      }
    }

  private:
    ConditionVariable _cv;
    std::mutex _mutex;
    size_t _counter;
  };
}
