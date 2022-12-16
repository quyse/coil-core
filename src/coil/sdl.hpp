#pragma once

#include "platform.hpp"
#include "input.hpp"
#include <memory>
#include <unordered_map>
#include <SDL2/SDL.h>

namespace Coil
{
  class Sdl
  {
  public:
    Sdl(uint32_t flags);
    ~Sdl();

  private:
    uint32_t _flags;
  };

  class SdlInputManager final : public InputManager
  {
  public:
    void ProcessEvent(SDL_Event const& event);
    void SetVirtualScale(vec2 const& scale);

  private:
    static InputKey ConvertKey(SDL_Scancode code);

    vec2 _scale = { 1, 1 };
    ivec2 _lastCursor;

    class SdlController final : public InputController
    {
    public:
      SdlController(SDL_GameController* controller);
      ~SdlController();

      bool IsActive() const;
      void RunHapticLeftRight(float left, float right);
      void StopHaptic();

      void Close();

    private:
      SdlController(SDL_GameController* controller, SDL_Joystick* joystick);

      SDL_GameController* _controller = nullptr;
      SDL_Haptic* _haptic = nullptr;
      SDL_HapticEffect _hapticEffect;
      int _hapticEffectIndex = -1;
    };

    std::unordered_map<InputControllerId, std::unique_ptr<SdlController>> _controllers;
  };

  class SdlWindow final : public Window
  {
  public:
    SdlWindow(SDL_Window* window);
    ~SdlWindow();

    void SetTitle(std::string const& title) override;
    void Close() override;
    void SetFullScreen(bool fullScreen) override;
    ivec2 GetDrawableSize() const override;
    float GetDPIScale() const override;
    SdlInputManager& GetInputManager() override;
    void Run(std::function<void()> const& loop) override;
    void PlaceCursor(ivec2 const& cursor) override;

    SDL_Window* GetSdlWindow() const;

  protected:
    void _UpdateMouseLock() override;
    void _UpdateCursorVisible() override;

  private:
    void UpdateInputManager();

    SDL_Window* _window = nullptr;
    uint32_t _windowId = 0;
    SdlInputManager _inputManager;
    ivec2 _virtualSize, _clientSize;
    float _dpiScale = 0;
    bool _fullScreen = false;
  };

  class SdlWindowSystem final : public WindowSystem
  {
  public:
    SdlWindow& CreateWindow(Book& book, std::string const& title, ivec2 const& size) override;
  };
}
