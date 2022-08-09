#pragma once

#include "input.hpp"
#include <functional>
#include <string>

namespace Coil
{
  // Window interface.
  class Window
  {
  public:
    // Set window title.
    virtual void SetTitle(std::string const& title) = 0;
    // Close window.
    virtual void Close() = 0;
    // Set fullscreen (borderless) mode.
    virtual void SetFullScreen(bool fullScreen) = 0;
    // Get DPI scale. 1 means 96 dpi, 1.5 means 144 dpi, etc.
    virtual float GetDPIScale() const = 0;

    // Run window loop.
    virtual void Run(std::function<void()> const& loop) = 0;

    // Set mouse lock state.
    void SetMouseLock(bool mouseLock);
    // Set hardware (OS) mouse cursor visibility.
    void SetCursorVisible(bool cursorVisible);

    // Set position of hardware mouse cursor (relative to window).
    virtual void PlaceCursor(int x, int y) = 0;

    // Stop window loop.
    void Stop();
    // Set prevent-user-close mode.
    void SetPreventUserClose(bool preventUserClose);

  protected:
    virtual void _UpdateMouseLock() = 0;
    virtual void _UpdateCursorVisible() = 0;

    bool _mouseLock = false;
    bool _cursorVisible = true;
    bool _running = false;
    bool _preventUserClose = false;
  };

  // Window system.
  class WindowSystem
  {
  public:
    virtual Window& CreateWindow(Book& book, std::string const& title, int width, int height) = 0;
  };
}
