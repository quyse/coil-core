#include "windows.hpp"

namespace Coil
{
  Handle::Handle(HANDLE handle)
  : _handle(handle) {}

  Handle::Handle(Handle&& other)
  {
    std::swap(_handle, other._handle);
  }

  Handle::~Handle()
  {
    if(_handle)
    {
      CloseHandle(_handle);
    }
  }

  Handle& Handle::operator=(Handle&& other)
  {
    std::swap(_handle, other._handle);
    return *this;
  }

  Handle::operator HANDLE() const
  {
    return _handle;
  }

  ComThread::ComThread(bool multiThreaded)
  {
    if(FAILED(CoInitializeEx(NULL, multiThreaded ? COINIT_MULTITHREADED : COINIT_APARTMENTTHREADED)))
    {
      throw Exception("COM initialization failed");
    }
  }

  ComThread::~ComThread()
  {
    CoUninitialize();
  }

  void CheckHResult(HRESULT hr, char const* message)
  {
    if(FAILED(hr))
    {
      throw Exception(message);
    }
  }
}
