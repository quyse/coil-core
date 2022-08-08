#include "platform.hpp"

namespace Coil
{
  void Window::SetMouseLock(bool mouseLock)
  {
    if(_mouseLock != mouseLock)
    {
      _mouseLock = mouseLock;
      _UpdateMouseLock();
    }
  }

  void Window::SetCursorVisible(bool cursorVisible)
  {
    if(_cursorVisible != cursorVisible)
    {
      _cursorVisible = cursorVisible;
      _UpdateCursorVisible();
    }
  }

  void Window::Stop()
  {
    _running = false;
  }

  void Window::SetPreventUserClose(bool preventUserClose)
  {
    _preventUserClose = preventUserClose;
  }
}
