#pragma once

#include "platform.hpp"
#include "input.hpp"
#include <unordered_map>
#include <SDL2/SDL.h>

namespace Coil
{
  class Sdl
  {
  public:
    Sdl(uint32_t flags);
    ~Sdl();

    static void Init(Book& book, uint32_t flags);

  private:
    uint32_t _flags;
  };

  class SdlInputManager : public InputManager
  {
  public:
    void Update() override;

    void ProcessEvent(SDL_Event const& event);
    void SetVirtualScale(float widthScale, float heightScale);

  private:
    static InputKey ConvertKey(SDL_Scancode code);

    float _widthScale = 1, _heightScale = 1;
    int _lastCursorX = 0, _lastCursorY = 0;

    class SdlController : public InputController
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

  class SdlWindow : public Window
  {
  public:
    SdlWindow(SDL_Window* window);
    ~SdlWindow();

    void SetTitle(std::string const& title) override;
    void Close() override;
    void SetFullScreen(bool fullScreen) override;
    float GetDPIScale() const override;
    void Run(std::function<void()> const& loop) override;
    void PlaceCursor(int x, int y) override;

  protected:
    void _UpdateMouseLock() override;
    void _UpdateCursorVisible() override;

  private:
    SDL_Window* _window = nullptr;
    uint32_t _windowId = 0;
    SdlInputManager* _inputManager = nullptr;
    int _virtualWidth = 0, _virtualHeight = 0;
    int _clientWidth = 0, _clientHeight = 0;
    float _dpiScale = 0;
    bool _fullScreen = false;
  };

  class SdlWindowSystem : public WindowSystem
  {
  public:
    SdlWindow& CreateWindow(Book& book, std::string const& title, int width, int height) override;
  };
}
