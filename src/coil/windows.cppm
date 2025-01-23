module;

#include "windows.hpp"

export module coil.core.windows;

import coil.core.base;

export namespace Coil
{
  class Handle
  {
  public:
    Handle(HANDLE handle = nullptr)
    : _handle(handle) {}

    Handle(Handle const&) = delete;

    Handle(Handle&& other)
    {
      std::swap(_handle, other._handle);
    }

    ~Handle()
    {
      if(_handle)
      {
        CloseHandle(_handle);
      }
    }

    Handle& operator=(Handle const&) = delete;

    Handle& operator=(Handle&& other)
    {
      std::swap(_handle, other._handle);
      return *this;
    }

    operator HANDLE() const
    {
      return _handle;
    }

  private:
    HANDLE _handle = nullptr;
  };

  // CoInitializeEx thread initializer
  class ComThread
  {
  public:
    ComThread(bool multiThreaded)
    {
      if(FAILED(CoInitializeEx(NULL, multiThreaded ? COINIT_MULTITHREADED : COINIT_APARTMENTTHREADED)))
      {
        throw Exception("COM initialization failed");
      }
    }

    ~ComThread()
    {
      CoUninitialize();
    }
  };

  // throw exception on unsuccessful HRESULT
  void CheckHResult(HRESULT hr, char const* message)
  {
    if(FAILED(hr))
    {
      throw Exception(message);
    }
  }

  // pointer to COM object
  template <typename T>
  class ComPtr
  {
  public:
    ComPtr(T* object = nullptr)
    : _object(object) {}

    ComPtr(ComPtr const& other)
    : _object(other._object)
    {
      if(_object)
      {
        _object->AddRef();
      }
    }

    ComPtr(ComPtr&& other)
    {
      std::swap(_object, other._object);
    }

    ~ComPtr()
    {
      if(_object)
      {
        _object->Release();
      }
    }

    ComPtr& operator=(ComPtr const& other)
    {
      if(_object)
      {
        _object->Release();
      }
      _object = other._object;
      if(_object)
      {
        _object->AddRef();
      }
      return *this;
    }

    ComPtr& operator=(ComPtr&& other)
    {
      std::swap(_object, other._object);
      return *this;
    }

    operator T*() const
    {
      return _object;
    }
    T* operator->() const
    {
      return _object;
    }

    // only for use for COM object-creation functions expecting pointer to pointer
    // releases current object to not lose it
    // new object is expected to have single reference
    T** operator&()
    {
      if(_object)
      {
        _object->Release();
        _object = nullptr;
      }
      return &_object;
    }

  private:
    T* _object = nullptr;
  };

  // pointer to memory allocated with CoTaskMemAlloc
  template <typename T>
  class CoTaskMemPtr
  {
  public:
    CoTaskMemPtr(T* object = nullptr)
    : _object(object) {}
    CoTaskMemPtr(CoTaskMemPtr const&) = delete;
    CoTaskMemPtr(CoTaskMemPtr&& other)
    {
      std::swap(_object, other._object);
    }

    ~CoTaskMemPtr()
    {
      if(_object)
      {
        CoTaskMemFree(_object);
      }
    }

    CoTaskMemPtr& operator=(CoTaskMemPtr const&) = delete;
    CoTaskMemPtr& operator=(CoTaskMemPtr&& other)
    {
      std::swap(_object, other._object);
      return *this;
    }

    T& operator*() const
    {
      return *_object;
    }

    operator T*() const
    {
      return _object;
    }
    T* operator->() const
    {
      return _object;
    }

    // only for use for COM allocation functions expecting pointer to pointer
    // frees current object to not lose it
    T** operator&()
    {
      if(_object)
      {
        CoTaskMemFree(_object);
        _object = nullptr;
      }
      return &_object;
    }

  private:
    T* _object = nullptr;
  };


  template <typename F>
  struct UserAPCHandlerBase;
  template <typename O>
  struct UserAPCHandlerBase<void (O::*)()>
  {
    using Object = O;
    template <void (Object::*handler)()>
    static void Handler(ULONG_PTR param)
    {
      (((Object*)param)->*handler)();
    }
  };
  template <auto handler>
  struct UserAPCHandler
  {
    using Base = UserAPCHandlerBase<decltype(handler)>;
    using Object = typename Base::Object;
    static constexpr auto Handler = &Base::template Handler<handler>;

    static void Queue(std::thread& thread, Object* object)
    {
      QueueUserAPC(Handler, thread.native_handle(), (ULONG_PTR)object);
    }
  };
}
