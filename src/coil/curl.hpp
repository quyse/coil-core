#pragma once

#include "tasks_streams.hpp"
#include <atomic>
#include <memory>
#include <functional>
#include <stop_token>
#include <thread>
#include <curl/curl.h>

namespace Coil
{
  class CurlManager : public std::enable_shared_from_this<CurlManager>
  {
  public:
    static std::shared_ptr<CurlManager> Create();

  private:
    void Init();

  public:
    struct RequestInfo
    {
      std::string url;
      std::vector<std::string> headers;
      // curl's default window size is used here
      size_t requestBufferSize = 32 * 1024 * 1024;
      size_t responseBufferSize = 32 * 1024 * 1024;
    };

    class Request final : public std::enable_shared_from_this<Request>, public SuspendableInputStream, public SuspendableOutputStream
    {
    public:
      Request(std::shared_ptr<CurlManager> pManager, RequestInfo const& info);
      ~Request();

      // SuspendableInputStream methods
      std::optional<size_t> TryRead(Buffer const& buffer) override;
      Task<void> WaitForRead() override;
      // SuspendableOutputStream methods
      bool TryWrite(Buffer const& buffer) override;
      Task<void> WaitForWrite(size_t size) override;

      Task<void> WaitForFinish();
      uint32_t GetResponseCode() const;

    private:
      void Start();

      size_t ReadCallback(char* data, size_t size);
      static size_t StaticReadCallback(char* buffer, size_t size, size_t nitems, void* userdata);
      size_t WriteCallback(const char* data, size_t size);
      static size_t StaticWriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata);

      void UpdatePausedState();

      void OnDone(CURLcode code);

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

    std::shared_ptr<Request> CreateRequest(RequestInfo const& info);

  private:
    void ThreadHandler(std::stop_token stopToken);

    void AddTask(std::function<void(CURLM*)>&& task);

    std::jthread _thread;
    std::mutex _mutex;
    std::vector<std::function<void(CURLM*)>> _tasks;
    std::atomic<bool> _errored = false;
  };
}
