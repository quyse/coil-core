module;

#if defined(COIL_PLATFORM_WINDOWS)
#include "windows.hpp"
#endif
#include <curl/curl.h>

#include <coroutine>
#include <stop_token>
#include <thread>

export module coil.core.curl;

import coil.core.base;
import coil.core.tasks.streams;
import coil.core.tasks.sync;
import coil.core.tasks;

namespace Coil
{
  struct CurlGlobal
  {
    CurlGlobal()
    {
      curl_global_init(CURL_GLOBAL_ALL);
    }

    ~CurlGlobal()
    {
      curl_global_cleanup();
    }
  };
}

export namespace Coil
{
  class CurlManager : public std::enable_shared_from_this<CurlManager>
  {
  public:
    static std::shared_ptr<CurlManager> Create()
    {
      auto pManager = std::make_shared<CurlManager>();
      pManager->Init();
      return pManager;
    }

  private:
    void Init()
    {
      static CurlGlobal global;
      _thread = std::jthread{[&](std::stop_token stopToken)
      {
        ThreadHandler(stopToken);
      }};
    }

  public:
    struct RequestInfo
    {
      std::string url;
      // optional configuration function
      std::function<void(CURL*)> configure;
      // HTTP headers
      std::vector<std::string> headers;
      // curl's default window size is used here
      size_t requestBufferSize = 32 * 1024 * 1024;
      size_t responseBufferSize = 32 * 1024 * 1024;
    };

    class Request final : public std::enable_shared_from_this<Request>, public SuspendableInputStream, public SuspendableOutputStream
    {
    public:
      Request(std::shared_ptr<CurlManager> pManager, RequestInfo const& info)
      : _pManager(std::move(pManager))
      , _requestPipe(info.requestBufferSize, true)
      , _responsePipe(info.responseBufferSize, true)
      {
        _curl = curl_easy_init();
        // set error buffer
        curl_easy_setopt(_curl, CURLOPT_ERRORBUFFER, _curlErrorBuffer);
        // set private pointer
        curl_easy_setopt(_curl, CURLOPT_PRIVATE, this);
        // set URL
        curl_easy_setopt(_curl, CURLOPT_URL, info.url.c_str());
        // set headers
        for(size_t i = 0; i < info.headers.size(); ++i)
          _headers = curl_slist_append(_headers, info.headers[i].c_str());
        curl_easy_setopt(_curl, CURLOPT_HTTPHEADER, _headers);
        // enable all supported encodings
        curl_easy_setopt(_curl, CURLOPT_ACCEPT_ENCODING, "");
        // set callbacks
        curl_easy_setopt(_curl, CURLOPT_READFUNCTION, &StaticReadCallback);
        curl_easy_setopt(_curl, CURLOPT_READDATA, this);
        curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, &StaticWriteCallback);
        curl_easy_setopt(_curl, CURLOPT_WRITEDATA, this);
        // run configuration
        if(info.configure)
        {
          info.configure(_curl);
        }
      }

      ~Request()
      {
        curl_easy_cleanup(_curl);
        curl_slist_free_all(_headers);
      }

      // SuspendableInputStream methods

      std::optional<size_t> TryRead(Buffer const& buffer) override
      {
        return _responsePipe.TryRead(buffer);
      }

      Task<void> WaitForRead() override
      {
        return _responsePipe.WaitForRead();
      }

      // SuspendableOutputStream methods

      bool TryWrite(Buffer const& buffer) override
      {
        return _requestPipe.TryWrite(buffer);
      }

      Task<void> WaitForWrite(size_t size) override
      {
        return _requestPipe.WaitForWrite(size);
      }

      Task<void> WaitForFinish()
      {
        std::unique_lock lock{_mutex};
        while(!_optResult.has_value())
        {
          co_await _cvResult.Wait(lock);
        }
        // return success
        if(_optResult.value() == CURLE_OK) co_return;
        // throw error, presuming error buffer has the message
        throw Exception("curl error: ") << _curlErrorBuffer;
      }

      uint32_t GetResponseCode() const
      {
        long code;
        curl_easy_getinfo(_curl, CURLINFO_RESPONSE_CODE, &code);
        return (uint32_t)code;
      }

      CURL* GetCURL() const
      {
        return _curl;
      }

    private:
      void Start()
      {
        // reference itself to prevent removal
        _pSelf = shared_from_this();

        _pManager->AddTask([pSelf = _pSelf](CURLM* curlm)
        {
          curl_multi_add_handle(curlm, pSelf->_curl);
        });
      }

      size_t ReadCallback(char* data, size_t size)
      {
        // try to read
        auto result = _requestPipe.TryRead(Buffer(data, size));

        // end of stream?
        if(!result.has_value()) return 0;

        // read some data?
        if(result.value()) return result.value();

        // otherwise no data to read yet
        // pause reading, unpause on readiness to read
        _pManager->AddTask([pSelf = shared_from_this()](CURLM*)
        {
          (void)[](std::shared_ptr<Request> pSelf) -> Task<void>
          {
            co_await pSelf->_requestPipe.WaitForRead();
            pSelf->_pManager->AddTask([pSelf](CURLM*)
            {
              pSelf->_requestReadPaused = false;
              pSelf->UpdatePausedState();
            });
          }(std::move(pSelf));
        });

        _requestReadPaused = true;
        return CURL_READFUNC_PAUSE;
      }

      static size_t StaticReadCallback(char* buffer, size_t size, size_t nitems, void* userdata)
      {
        return ((Request*)userdata)->ReadCallback(buffer, size * nitems);
      }

      size_t WriteCallback(const char* data, size_t size)
      {
        // try to write
        if(_responsePipe.TryWrite(Buffer(data, size)))
        {
          return size;
        }

        // if not succeeded, pause writing, unpause on readiness to write
        _pManager->AddTask([pSelf = shared_from_this(), size](CURLM*)
        {
          (void)[](std::shared_ptr<Request> pSelf, size_t size) -> Task<void>
          {
            co_await pSelf->_responsePipe.WaitForWrite(size);
            pSelf->_pManager->AddTask([pSelf](CURLM*)
            {
              pSelf->_responseWritePaused = false;
              pSelf->UpdatePausedState();
            });
          }(std::move(pSelf), size);
        });

        _responseWritePaused = true;
        return CURL_WRITEFUNC_PAUSE;
      }

      static size_t StaticWriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata)
      {
        return ((Request*)userdata)->WriteCallback(ptr, size * nmemb);
      }

      void UpdatePausedState()
      {
        curl_easy_pause(_curl, (_requestReadPaused ? CURLPAUSE_SEND : 0) | (_responseWritePaused ? CURLPAUSE_RECV : 0));
      }

      void OnDone(CURLcode code)
      {
        // indicate response end
        _responsePipe.TryWrite({});

        // save result
        {
          std::unique_lock lock{_mutex};
          _optResult = code;
        }

        // notify subscribers
        _cvResult.NotifyAll();

        // remove self-reference
        _pSelf = nullptr;
      }

      std::shared_ptr<CurlManager> _pManager;
      CURL* _curl = nullptr;
      curl_slist* _headers = nullptr;
      SuspendablePipe _requestPipe;
      SuspendablePipe _responsePipe;
      ConditionVariable _cvResult;
      std::mutex _mutex;
      std::optional<CURLcode> _optResult;
      char _curlErrorBuffer[CURL_ERROR_SIZE] = {};
      // reference to prevent removal
      std::shared_ptr<Request> _pSelf;
      // accessed only in curl thread
      bool _requestReadPaused = false;
      bool _responseWritePaused = false;

      friend CurlManager;
    };

    std::shared_ptr<Request> CreateRequest(RequestInfo const& info)
    {
      auto pRequest = std::make_shared<Request>(shared_from_this(), info);
      pRequest->Start();
      return pRequest;
    }

  private:
    void ThreadHandler(std::stop_token stopToken)
    {
      CURLM* curlm = curl_multi_init();

      std::stop_callback stopCallback{stopToken, [curlm]()
      {
        curl_multi_wakeup(curlm);
      }};

      std::vector<std::function<void(CURLM*)>> tempTasks;

      while(!stopToken.stop_requested())
      {
        // perform work
        int runningHandlesCount;
        if(curl_multi_perform(curlm, &runningHandlesCount))
        {
          _errored = true;
          break;
        }

        // read messages
        int remainingMsgsCount;
        while(CURLMsg const* pMsg = curl_multi_info_read(curlm, &remainingMsgsCount))
        {
          switch(pMsg->msg)
          {
          case CURLMSG_DONE:
            {
              // retrieve data
              CURL* curl = pMsg->easy_handle;
              CURLcode result = pMsg->data.result;
              Request* request = nullptr;
              curl_easy_getinfo(curl, CURLINFO_PRIVATE, (char**)&request);
              // remove handle from multi object
              curl_multi_remove_handle(curlm, curl);

              // notify request
              request->OnDone(result);
            }
            break;
          default:
            break;
          }
        }

        // perform tasks
        {
          std::unique_lock lock{_mutex};
          std::swap(_tasks, tempTasks);
          lock.unlock();

          for(auto const& task : tempTasks)
            task(curlm);
          tempTasks.clear();
        }

        // poll for events
        if(curl_multi_poll(curlm, nullptr, 0, 1000, nullptr))
        {
          _errored = true;
          break;
        }
      }

      curl_multi_cleanup(curlm);
    }

    void AddTask(std::function<void(CURLM*)>&& task)
    {
      std::unique_lock lock{_mutex};
      _tasks.push_back(std::move(task));
    }

    std::jthread _thread;
    std::mutex _mutex;
    std::vector<std::function<void(CURLM*)>> _tasks;
    std::atomic<bool> _errored = false;
  };
}
