#include "platform.hpp"

namespace Coil
{
  void Window::SetPresenter(GraphicsPresenter* presenter)
  {
    _presenter = presenter;
  }

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

  bool Window::IsVisible() const
  {
    return _visible;
  }

  void Window::Stop()
  {
    _running = false;
  }

  void Window::SetLoopOnlyVisible(bool loopOnlyVisible)
  {
    _loopOnlyVisible = loopOnlyVisible;
  }
}
