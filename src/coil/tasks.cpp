#include "tasks.hpp"

namespace Coil
{
  TaskEngine::~TaskEngine()
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

  void TaskEngine::Queue(std::coroutine_handle<> coroutine)
  {
    std::unique_lock lock(_mutex);
    _coroutines.push(coroutine);
    lock.unlock();

    _cv.notify_one();
  }

  void TaskEngine::AddThread()
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
        auto coroutine = std::move(_coroutines.front());
        _coroutines.pop();
        lock.unlock();

        coroutine.resume();
      }
    });
  }

  void TaskEngine::AddThreads(size_t reserveThreadsCount, size_t minimumThreadsCount)
  {
    auto threadsCount = std::max((size_t)std::jthread::hardware_concurrency() - reserveThreadsCount, minimumThreadsCount);
    for(size_t i = 0; i < threadsCount; ++i)
      AddThread();
  }

  void TaskEngine::Run()
  {
    for(;;)
    {
      std::unique_lock lock(_mutex);
      if(_coroutines.empty()) break;
      auto coroutine = std::move(_coroutines.front());
      _coroutines.pop();
      lock.unlock();

      coroutine.resume();
    }
  }

  TaskEngine& TaskEngine::GetInstance()
  {
    static TaskEngine instance;
    return instance;
  }
}
