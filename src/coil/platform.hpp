#pragma once

#include "input.hpp"
#include "math.hpp"
#include <functional>
#include <string>

namespace Coil
{
  class GraphicsPresenter;

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
    // Get size of drawable surface in pixels.
    virtual ivec2 GetDrawableSize() const = 0;
    // Get DPI scale. 1 means 96 dpi, 1.5 means 144 dpi, etc.
    virtual float GetDPIScale() const = 0;
    // Get input manager.
    virtual InputManager& GetInputManager() = 0;

    // Run window loop.
    virtual void Run(std::function<void()> const& loop) = 0;

    void SetPresenter(GraphicsPresenter* presenter);
    // Set mouse lock state.
    void SetMouseLock(bool mouseLock);
    // Set hardware (OS) mouse cursor visibility.
    void SetCursorVisible(bool cursorVisible);

    // Is window not minimized?
    bool IsVisible() const;

    // Set position of hardware mouse cursor (relative to window).
    virtual void PlaceCursor(int x, int y) = 0;

    // Stop window loop.
    void Stop();
    // Set loop-only-visible mode, when loop is not called if window is not visible.
    void SetLoopOnlyVisible(bool loopOnlyVisible);

  protected:
    virtual void _UpdateMouseLock() = 0;
    virtual void _UpdateCursorVisible() = 0;

    GraphicsPresenter* _presenter = nullptr;
    bool _mouseLock = false;
    bool _cursorVisible = true;
    bool _running = false;
    bool _visible = true;
    bool _loopOnlyVisible = false;
  };

  // Window system.
  class WindowSystem
  {
  public:
    virtual Window& CreateWindow(Book& book, std::string const& title, int width, int height) = 0;
  };
}
