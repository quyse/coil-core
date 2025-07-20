module;

#include <coroutine>
#include <memory>
#include <mutex>
#include <queue>

export module coil.core.tasks.sync;

import coil.core.tasks;

export namespace Coil
{
  // condition variable which works with tasks
  class ConditionVariable
  {
  public:
    ConditionVariable() = default;

    // implementation node: we lock and unlock _cv._mutex manually, instead of using std::unique_lock
    // the problem with std::unique_lock is that its unlock() method is not atomic
    // at least in libc++ https://github.com/llvm/llvm-project/issues/49709
    // and after unlock the awaiter can be deleted by other thread before the owning boolean flag is set
    class WaitAwaiter
    {
    public:
      WaitAwaiter(ConditionVariable& cv, std::unique_lock<std::mutex>& userLock)
      : _cv{cv}, _userLock{userLock}
      {
        _cv._mutex.lock();
        _cvLocked = true;

        _userLock.unlock();
      }

      ~WaitAwaiter()
      {
        if(_cvLocked)
        {
          _cv._mutex.unlock();
        }
      }

      bool await_ready() const
      {
        return false;
      }

      void await_suspend(std::coroutine_handle<> coroutine)
      {
        _cv._waiters.push(coroutine);
        // reset the flag before unlocking the mutex
        _cvLocked = false;
        _cv._mutex.unlock();
        // after unlocking the mutex awaiter can be deleted by other thread
      }

      void await_resume() const
      {
        _userLock.lock();
      }

    private:
      ConditionVariable& _cv;
      std::unique_lock<std::mutex>& _userLock;
      bool _cvLocked = false;
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
