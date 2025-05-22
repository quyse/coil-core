module;

#include <SDL3/SDL.h>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

export module coil.core.sdl;

import coil.core.base;
import coil.core.input;
import coil.core.math;
import coil.core.platform;
import coil.core.unicode;

namespace Coil
{
  Exception SdlException(char const* message)
  {
    return Exception{message} << ": " << SDL_GetError();
  }
}

export namespace Coil
{
  class SdlGlobal
  {
  public:
    SdlGlobal()
    {
      SDL_Init(0);
    }

    ~SdlGlobal()
    {
      SDL_Quit();
    }
  };

  class Sdl
  {
  public:
    Sdl(uint32_t flags)
    : _flags(flags)
    {
      // apparently, that's the best way
      // initializes once on the first run of this constructor
      // de-initializes at exit of the process
      static SdlGlobal sdlGlobal;

      SDL_InitSubSystem(_flags);
    }
    ~Sdl()
    {
      SDL_QuitSubSystem(_flags);
    }

  private:
    uint32_t _flags;
  };

  class SdlInputManager final : public InputManager
  {
  public:
    void ProcessEvent(SDL_Event const& sdlEvent)
    {
      switch(sdlEvent.type)
      {
      case SDL_EVENT_KEY_DOWN:
      case SDL_EVENT_KEY_UP:
        AddEvent(InputKeyboardKeyEvent
        {
          .key = ConvertKey(sdlEvent.key.scancode),
          .isPressed = sdlEvent.type == SDL_EVENT_KEY_DOWN,
        });
        break;
      case SDL_EVENT_TEXT_INPUT:
        for(Unicode::Iterator<char, char32_t, char const*> i(sdlEvent.text.text); *i; ++i)
        {
          AddEvent(InputKeyboardCharacterEvent
          {
            .character = *i,
          });
        }
        break;
      case SDL_EVENT_MOUSE_MOTION:
        // raw move event
        AddEvent(InputMouseRawMoveEvent
        {
          .rawMove =
          {
            (float)sdlEvent.motion.xrel,
            (float)sdlEvent.motion.yrel,
            0,
          },
        });
        // cursor move event
        _lastCursor =
        {
          (int32_t)((float)sdlEvent.motion.x * _scale.x()),
          (int32_t)((float)sdlEvent.motion.y * _scale.y()),
        };
        AddEvent(InputMouseCursorMoveEvent
        {
          .cursor = _lastCursor,
          .wheel = 0,
        });
        break;
      case SDL_EVENT_MOUSE_BUTTON_DOWN:
      case SDL_EVENT_MOUSE_BUTTON_UP:
        {
          InputMouseButton button;
          bool ok = true;
          switch(sdlEvent.button.button)
          {
          case SDL_BUTTON_LEFT:
            button = InputMouseButton::Left;
            break;
          case SDL_BUTTON_MIDDLE:
            button = InputMouseButton::Middle;
            break;
          case SDL_BUTTON_RIGHT:
            button = InputMouseButton::Right;
            break;
          default:
            ok = false;
            break;
          }
          if(ok)
          {
            AddEvent(InputMouseButtonEvent
            {
              .button = button,
              .isPressed = sdlEvent.type == SDL_EVENT_MOUSE_BUTTON_DOWN,
            });
          }
        }
        break;
      case SDL_EVENT_MOUSE_WHEEL:
        // raw move event
        AddEvent(InputMouseRawMoveEvent
        {
          .rawMove =
          {
            0,
            0,
            (float)(sdlEvent.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -sdlEvent.wheel.y : sdlEvent.wheel.y),
          },
        });
        // cursor move event
        AddEvent(InputMouseCursorMoveEvent
        {
          .cursor = _lastCursor,
          .wheel = sdlEvent.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -sdlEvent.wheel.y : sdlEvent.wheel.y,
        });
        break;
      case SDL_EVENT_GAMEPAD_ADDED:
        // device added event
        {
          // open controller
          SDL_Gamepad* sdlController = SDL_OpenGamepad(sdlEvent.cdevice.which);
          if(sdlController)
          {
            auto controller = std::make_unique<SdlController>(sdlController);
            InputControllerId controllerId = controller->GetId();
            _controllers.insert({ controllerId, std::move(controller) });

            AddEvent(InputControllerEvent
            {
              .controllerId = controllerId,
              .event = InputControllerEvent::ControllerEvent
              {
                .isAdded = true,
              },
            });
          }
        }
        break;
      case SDL_EVENT_GAMEPAD_REMOVED:
        // device removed event
        {
          // try to find controller by id, and remove it from map
          auto i = _controllers.find(sdlEvent.cdevice.which);
          if(i != _controllers.end())
          {
            i->second->Close();
            _controllers.erase(i);

            // emit event
            AddEvent(InputControllerEvent
            {
              .controllerId = (InputControllerId)sdlEvent.cdevice.which,
              .event = InputControllerEvent::ControllerEvent
              {
                .isAdded = false,
              },
            });
          }
        }
        break;
      case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
      case SDL_EVENT_GAMEPAD_BUTTON_UP:
        // controller button down/up event
        {
          InputControllerButton button;
          bool ok = true;
          switch(sdlEvent.gbutton.button)
          {
#define B(a, b) case SDL_GAMEPAD_BUTTON_##a: button = InputControllerButton::b; break
          B(SOUTH, A);
          B(EAST, B);
          B(WEST, X);
          B(NORTH, Y);
          B(BACK, Back);
          B(GUIDE, Guide);
          B(START, Start);
          B(LEFT_STICK, LeftStick);
          B(RIGHT_STICK, RightStick);
          B(LEFT_SHOULDER, LeftShoulder);
          B(RIGHT_SHOULDER, RightShoulder);
          B(DPAD_UP, DPadUp);
          B(DPAD_DOWN, DPadDown);
          B(DPAD_LEFT, DPadLeft);
          B(DPAD_RIGHT, DPadRight);
#undef B
          default: ok = false; break;
          }
          if(ok)
          {
            AddEvent(InputControllerEvent
            {
              .controllerId = (InputControllerId)sdlEvent.gbutton.which,
              .event = InputControllerEvent::ButtonEvent
              {
                .button = button,
                .isPressed = sdlEvent.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN,
              },
            });
          }
        }
        break;
      case SDL_EVENT_GAMEPAD_AXIS_MOTION:
        // controller axis motion event
        {
          InputControllerAxis axis;
          bool ok = true;
          switch(sdlEvent.gaxis.axis)
          {
#define A(a, b) case SDL_GAMEPAD_AXIS_##a: axis = InputControllerAxis::b; break
          A(LEFTX, LeftX);
          A(LEFTY, LeftY);
          A(RIGHTX, RightX);
          A(RIGHTY, RightY);
          A(LEFT_TRIGGER, TriggerLeft);
          A(RIGHT_TRIGGER, TriggerRight);
#undef A
          default: ok = false; break;
          }
          if(ok)
          {
            AddEvent(InputControllerEvent
            {
              .controllerId = (InputControllerId)sdlEvent.gaxis.which,
              .event = InputControllerEvent::AxisMotionEvent
              {
                .axis = axis,
                .axisValue = sdlEvent.gaxis.value,
              },
            });
          }
        }
        break;
      }
    }

    void SetVirtualScale(vec2 const& scale)
    {
      _scale = scale;
    }

  private:
    static InputKey ConvertKey(SDL_Scancode code)
    {
      InputKey key;
      switch(code)
      {
#define C(c, k) case SDL_SCANCODE_##c: key = InputKey::k; break

        C(BACKSPACE, BackSpace);
        C(TAB, Tab);
        C(CLEAR, Clear);
        C(RETURN, Return);
        C(PAUSE, Pause);
        C(SCROLLLOCK, ScrollLock);
        C(SYSREQ, SysReq);
        C(ESCAPE, Escape);
        C(INSERT, Insert);
        C(DELETE, Delete);

        C(SPACE, Space);

        C(COMMA, Comma);
        C(MINUS, Hyphen);
        C(PERIOD, Period);
        C(SLASH, Slash);

        C(0, _0);
        C(1, _1);
        C(2, _2);
        C(3, _3);
        C(4, _4);
        C(5, _5);
        C(6, _6);
        C(7, _7);
        C(8, _8);
        C(9, _9);

        C(A, A);
        C(B, B);
        C(C, C);
        C(D, D);
        C(E, E);
        C(F, F);
        C(G, G);
        C(H, H);
        C(I, I);
        C(J, J);
        C(K, K);
        C(L, L);
        C(M, M);
        C(N, N);
        C(O, O);
        C(P, P);
        C(Q, Q);
        C(R, R);
        C(S, S);
        C(T, T);
        C(U, U);
        C(V, V);
        C(W, W);
        C(X, X);
        C(Y, Y);
        C(Z, Z);

        C(HOME, Home);
        C(LEFT, Left);
        C(UP, Up);
        C(RIGHT, Right);
        C(DOWN, Down);
        C(PAGEUP, PageUp);
        C(PAGEDOWN, PageDown);
        C(END, End);
        C(GRAVE, Grave);

        C(KP_SPACE, NumPadSpace);
        C(KP_TAB, NumPadTab);
        C(KP_ENTER, NumPadEnter);
        C(KP_EQUALS, NumPadEqual);
        C(KP_MULTIPLY, NumPadMultiply);
        C(KP_PLUS, NumPadAdd);
        C(KP_MINUS, NumPadSubtract);
        C(KP_DECIMAL, NumPadDecimal);
        C(KP_DIVIDE, NumPadDivide);

        C(KP_0, NumPad0);
        C(KP_1, NumPad1);
        C(KP_2, NumPad2);
        C(KP_3, NumPad3);
        C(KP_4, NumPad4);
        C(KP_5, NumPad5);
        C(KP_6, NumPad6);
        C(KP_7, NumPad7);
        C(KP_8, NumPad8);
        C(KP_9, NumPad9);

        C(F1, F1);
        C(F2, F2);
        C(F3, F3);
        C(F4, F4);
        C(F5, F5);
        C(F6, F6);
        C(F7, F7);
        C(F8, F8);
        C(F9, F9);
        C(F10, F10);
        C(F11, F11);
        C(F12, F12);

        C(LSHIFT, ShiftL);
        C(RSHIFT, ShiftR);
        C(LCTRL, ControlL);
        C(RCTRL, ControlR);
        C(CAPSLOCK, CapsLock);
        C(LGUI, SuperL);
        C(RGUI, SuperR);
        C(LALT, AltL);
        C(RALT, AltR);

#undef C

      default:
        key = InputKey::_Unknown;
        break;
      }

      return key;
    }

    vec2 _scale = { 1, 1 };
    ivec2 _lastCursor;

    class SdlController final : public InputController
    {
    public:
      SdlController(SDL_Gamepad* controller)
      : SdlController(controller, SDL_GetGamepadJoystick(controller))
      {}
      ~SdlController()
      {
        Close();
      }

      bool IsActive() const
      {
        return !!_controller;
      }

      void RunHapticLeftRight(float left, float right)
      {
        if(!_haptic) return;

        _hapticEffect.leftright.length = SDL_HAPTIC_INFINITY;
        _hapticEffect.leftright.large_magnitude = uint16_t(left * 0xffff);
        _hapticEffect.leftright.small_magnitude = uint16_t(right * 0xffff);

        if(_hapticEffectIndex >= 0 && _hapticEffect.type == SDL_HAPTIC_LEFTRIGHT)
        {
          SDL_StopHapticEffect(_haptic, _hapticEffectIndex);
          SDL_UpdateHapticEffect(_haptic, _hapticEffectIndex, &_hapticEffect);
        }
        else
        {
          if(_hapticEffectIndex >= 0)
          {
            SDL_StopHapticEffect(_haptic, _hapticEffectIndex);
            SDL_DestroyHapticEffect(_haptic, _hapticEffectIndex);
          }

          _hapticEffect.type = SDL_HAPTIC_LEFTRIGHT;
          _hapticEffectIndex = SDL_CreateHapticEffect(_haptic, &_hapticEffect);
        }

        SDL_RunHapticEffect(_haptic, _hapticEffectIndex, 1);
      }

      void StopHaptic()
      {
        if(_hapticEffectIndex >= 0)
        {
          SDL_StopHapticEffect(_haptic, _hapticEffectIndex);
        }
      }

      void Close()
      {
        if(_hapticEffectIndex >= 0)
        {
          SDL_StopHapticEffect(_haptic, _hapticEffectIndex);
          SDL_DestroyHapticEffect(_haptic, _hapticEffectIndex);
          _hapticEffectIndex = -1;
        }

        if(_haptic)
        {
          SDL_CloseHaptic(_haptic);
          _haptic = nullptr;
        }

        if(_controller)
        {
          SDL_CloseGamepad(_controller);
          _controller = nullptr;
        }
      }

    private:
      SdlController(SDL_Gamepad* controller, SDL_Joystick* joystick)
      : InputController(SDL_GetJoystickID(joystick))
      , _controller(controller)
      , _haptic(SDL_OpenHapticFromJoystick(joystick))
      {}

      SDL_Gamepad* _controller = nullptr;
      SDL_Haptic* _haptic = nullptr;
      SDL_HapticEffect _hapticEffect;
      int _hapticEffectIndex = -1;
    };

    std::unordered_map<InputControllerId, std::unique_ptr<SdlController>> _controllers;
  };

  class SdlWindow final : public Window
  {
  public:
    SdlWindow(SDL_Window* window)
    : _window(window), _windowId(SDL_GetWindowID(_window))
    {
      SDL_GetWindowSize(_window, &_virtualSize.x(), &_virtualSize.y());
      SDL_GetWindowSizeInPixels(_window, &_clientSize.x(), &_clientSize.y());
      _dpiScale = float(_clientSize.x()) / float(_virtualSize.x());
      UpdateInputManager();
    }

    ~SdlWindow()
    {
      Close();
    }

    void SetTitle(std::string const& title) override
    {
      SDL_SetWindowTitle(_window, title.c_str());
    }

    void Close() override
    {
      if(_window)
      {
        if(_presenter)
        {
          _presenter = nullptr;
        }
        SDL_DestroyWindow(_window);
        _window = nullptr;
      }
      Stop();
    }

    void SetFullScreen(bool fullScreen) override
    {
      if(_fullScreen == fullScreen) return;
      _fullScreen = fullScreen;
      SDL_SetWindowFullscreen(_window, _fullScreen);
    }

    ivec2 GetDrawableSize() const override
    {
      return _clientSize;
    }

    float GetDPIScale() const override
    {
      return _dpiScale;
    }

    SdlInputManager& GetInputManager() override
    {
      return _inputManager;
    }

    void Run(std::function<void()> const& loop) override
    {
      _running = true;

      while(_running)
      {
        SDL_Event event;
        bool wait = _loopOnlyVisible && !_visible;
        while((wait ? SDL_WaitEvent : SDL_PollEvent)(&event))
        {
          _inputManager.ProcessEvent(event);

          switch(event.type)
          {
          case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            SDL_GetWindowSize(_window, &_virtualSize.x(), &_virtualSize.y());
            SDL_GetWindowSizeInPixels(_window, &_clientSize.x(), &_clientSize.y());
            _dpiScale = float(_clientSize.x()) / float(_virtualSize.x());
            UpdateInputManager();
            if(_presenter)
            {
              _presenter->Resize(GetDrawableSize());
            }
            break;
          case SDL_EVENT_WINDOW_MINIMIZED:
            _visible = false;
            break;
          case SDL_EVENT_WINDOW_RESTORED:
          case SDL_EVENT_WINDOW_MAXIMIZED:
            _visible = true;
            break;
          case SDL_EVENT_WINDOW_FOCUS_LOST:
            _inputManager.ReleaseButtonsOnUpdate();
            break;
          case SDL_EVENT_QUIT:
            Stop();
            break;
          }
        }

        _inputManager.Update();

        if(_visible || !_loopOnlyVisible)
        {
          loop();
        }
      }
    }

    void PlaceCursor(ivec2 const& cursor) override
    {
      SDL_WarpMouseInWindow(_window, cursor.x(), cursor.y());
    }

    SDL_Window* GetSdlWindow() const
    {
      return _window;
    }

  protected:
    void _UpdateMouseLock() override
    {
      SDL_SetWindowRelativeMouseMode(_window, _mouseLock);
    }

    void _UpdateCursorVisible() override
    {
      if(_cursorVisible)
      {
        SDL_ShowCursor();
      }
      else
      {
        SDL_HideCursor();
      }
    }

  private:
    void UpdateInputManager()
    {
      _inputManager.SetVirtualScale(
      {
        (float)_clientSize.x() / (float)_virtualSize.x(),
        (float)_clientSize.y() / (float)_virtualSize.y(),
      });
    }

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
    SdlWindow& CreateWindow(Book& book, std::string const& title, ivec2 const& size) override
    {
      SDL_Window* window = SDL_CreateWindow(title.c_str(), size.x(), size.y(),
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_VULKAN
      );
      if(!window)
        throw SdlException("failed to create SDL window");
      return book.Allocate<SdlWindow>(window);
    }
  };
}
