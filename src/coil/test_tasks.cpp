#include "entrypoint.hpp"
#include "tasks_sync.hpp"
#include "math.hpp"
#include <iostream>
using namespace Coil;

class Tester
{
public:
  Tester(size_t threadsCount)
  {
    // register tests

    // void result
    {
      uint32_t const n = 10;
      struct Test
      {
        static Task<void> f(uint32_t i, uint32_t& r)
        {
          if(i < n)
          {
            ++r;
            co_await f(i + 1, r);
          }
        }
      };

      AddTest([]() -> Task<bool>
      {
        uint32_t r = 0;
        co_await Test::f(0, r);
        co_return r == n;
      }());
    }

    // simple recursion
    {
      uint64_t const n = 1 << 16;
      struct Test
      {
        static Task<uint64_t> f(uint64_t i)
        {
          if(i < n - 1)
          {
            auto a = f(i * 2 + 1);
            auto b = f(i * 2 + 2);
            co_return co_await a + co_await b;
          }
          co_return i - n + 1;
        }
      };

      AddTest([]() -> Task<bool>
      {
        auto r = co_await Test::f(0);
        co_return r == (n - 1) * n / 2;
      }());
    }

    // multiple listeners
    {
      struct Test
      {
        static Task<uint64_t> f(std::vector<Task<uint64_t>> deps)
        {
          uint64_t r = 1;
          for(size_t j = 0; j < deps.size(); ++j)
            r += co_await deps[j];
          co_return r;
        }
      };

      uint64_t const n = 1 << 16;
      std::vector<Task<uint64_t>> tasks;
      tasks.reserve(n);
      for(size_t i = 0; i < n; ++i)
      {
        std::vector<Task<uint64_t>> deps;
        for(size_t j = 1; j < i; ++j)
          if(i % j == 0)
            deps.push_back(tasks[j]);
        tasks.push_back(Test::f(std::move(deps)));
      }

      AddTest([](std::vector<Task<uint64_t>> tasks) -> Task<bool>
      {
        for(size_t i = 0; i < tasks.size(); ++i)
          co_await tasks[i];
        co_return true;
      }(std::move(tasks)));
    }

    // exceptions
    {
      uint32_t const n = 10;
      struct Test
      {
        static Task<uint32_t> f(uint32_t i)
        {
          if(i < n) co_return co_await f(i + 1);
          else throw 123;
        }
      };

      // try-catch
      AddTest([]() -> Task<bool>
      {
        try
        {
          co_await Test::f(0);
          co_return false;
        }
        catch(int e)
        {
          co_return e == 123;
        }
      }());

      // unhandled exception
      // minimum 2 threads needed (1 thread will be blocked)
      if(threadsCount >= 2)
      {
        AddTest([]() -> Task<bool>
        {
          try
          {
            Test::f(0).Get();
            co_return false;
          }
          catch(int e)
          {
            co_return e == 123;
          }
        }());
      }
    }

    // semaphores
    {
      // n tasks, each acquires and releases semaphore m times
      size_t const n = 100;
      size_t const m = 100;
      auto s = std::make_shared<Semaphore>(3);
      struct Test
      {
        static Task<void> f(std::shared_ptr<Semaphore> s, size_t index)
        {
          for(size_t i = 0; i < m; ++i)
          {
            co_await s->Acquire();
            s->Release();
          }
        }
      };
      std::vector<Task<void>> tasks;
      tasks.reserve(n);
      for(size_t i = 0; i < n; ++i)
        tasks.push_back(Test::f(s, i));
      AddTest([](std::vector<Task<void>> tasks) -> Task<bool>
      {
        for(size_t i = 0; i < tasks.size(); ++i)
          co_await tasks[i];
        co_return true;
      }(std::move(tasks)));
    }
  }

  ivec2 Run()
  {
    size_t const okCount = [&]() -> Task<size_t>
    {
      size_t okCount = 0;
      for(size_t i = 0; i < _tests.size(); ++i)
        if(co_await _tests[i])
          ++okCount;
      co_return okCount;
    }().Get();
    return { (int32_t)_tests.size(), (int32_t)okCount };
  }

private:
  void AddTest(Task<bool>&& test)
  {
    _tests.push_back(std::move(test));
  };

  std::vector<Task<bool>> _tests;
};

int COIL_ENTRY_POINT(std::vector<std::string>&& args)
{
  ivec2 counts;

  // run tests on increasing number of threads
  size_t threadsCounts[] = { 1, 2, 4 };
  size_t currentThreadsCount = 0;
  for(size_t i = 0; i < sizeof(threadsCounts) / sizeof(threadsCounts[0]); ++i)
  {
    for(size_t j = currentThreadsCount; j < threadsCounts[i]; ++j)
      TaskEngine::GetInstance().AddThread();
    counts += Tester(threadsCounts[i]).Run();
  }

  if(counts.x() == counts.y())
  {
    std::cout << "OK tests: " << counts.y() << "\n";
    return 0;
  }
  else
  {
    std::cout << "FAILED tests: " << (counts.x() - counts.y()) << " out of " << counts.x() << "\n";
    return 1;
  }
}
