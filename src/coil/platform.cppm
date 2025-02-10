module;

#include <functional>
#include <string>

export module coil.core.platform;

import coil.core.base.generator;
import coil.core.base;
import coil.core.graphics;
import coil.core.input;
import coil.core.math;

export namespace Coil
{
  // Window interface.
  class Window : public GraphicsWindow
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

    // Helper function, running window loop with generator.
    // It is OK to pass lambda coroutine here
    template <typename CreateGenerator>
    void RunGenerator(CreateGenerator&& createGenerator)
    {
      auto generator = createGenerator();
      Run([&]()
      {
        if(!generator())
        {
          Stop();
        }
      });
    }

    // Set mouse lock state.
    void SetMouseLock(bool mouseLock)
    {
      if(_mouseLock != mouseLock)
      {
        _mouseLock = mouseLock;
        _UpdateMouseLock();
      }
    }
    // Set hardware (OS) mouse cursor visibility.
    void SetCursorVisible(bool cursorVisible)
    {
      if(_cursorVisible != cursorVisible)
      {
        _cursorVisible = cursorVisible;
        _UpdateCursorVisible();
      }
    }

    // Is window not minimized?
    bool IsVisible() const
    {
      return _visible;
    }

    // Set position of hardware mouse cursor (relative to window).
    virtual void PlaceCursor(ivec2 const& cursor) = 0;

    // Stop window loop.
    void Stop()
    {
      _running = false;
    }
    // Set loop-only-visible mode, when loop is not called if window is not visible.
    void SetLoopOnlyVisible(bool loopOnlyVisible)
    {
      _loopOnlyVisible = loopOnlyVisible;
    }

  protected:
    virtual void _UpdateMouseLock() = 0;
    virtual void _UpdateCursorVisible() = 0;

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
    virtual Window& CreateWindow(Book& book, std::string const& title, ivec2 const& size) = 0;
  };
}
