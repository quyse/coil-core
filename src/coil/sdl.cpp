#include "sdl.hpp"
#include "graphics.hpp"
#include "unicode.hpp"

namespace Coil
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

  Sdl::Sdl(uint32_t flags)
  : _flags(flags)
  {
    // apparently, that's the best way
    // initializes once on the first run of this constructor
    // de-initializes at exit of the process
    static SdlGlobal sdlGlobal;

    SDL_InitSubSystem(_flags);
  }

  Sdl::~Sdl()
  {
    SDL_QuitSubSystem(_flags);
  }

  void SdlInputManager::ProcessEvent(SDL_Event const& sdlEvent)
  {
    switch(sdlEvent.type)
    {
    case SDL_KEYDOWN:
    case SDL_KEYUP:
      AddEvent(InputKeyboardKeyEvent
      {
        .key = ConvertKey(sdlEvent.key.keysym.scancode),
        .isPressed = sdlEvent.type == SDL_KEYDOWN,
      });
      break;
    case SDL_TEXTINPUT:
      for(Unicode::Iterator<char, char32_t, char const*> i(sdlEvent.text.text); *i; ++i)
      {
        AddEvent(InputKeyboardCharacterEvent
        {
          .character = *i,
        });
      }
      break;
    case SDL_MOUSEMOTION:
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
        (int32_t)(sdlEvent.motion.x * _scale.x()),
        (int32_t)(sdlEvent.motion.y * _scale.y()),
      };
      AddEvent(InputMouseCursorMoveEvent
      {
        .cursor = _lastCursor,
        .wheel = 0,
      });
      break;
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
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
            .isPressed = sdlEvent.type == SDL_MOUSEBUTTONDOWN,
          });
        }
      }
      break;
    case SDL_MOUSEWHEEL:
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
    case SDL_CONTROLLERDEVICEADDED:
      // device added event
      {
        // open controller
        SDL_GameController* sdlController = SDL_GameControllerOpen(sdlEvent.cdevice.which);
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
    case SDL_CONTROLLERDEVICEREMOVED:
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
    case SDL_CONTROLLERBUTTONDOWN:
    case SDL_CONTROLLERBUTTONUP:
      // controller button down/up event
      {
        InputControllerButton button;
        bool ok = true;
        switch(sdlEvent.cbutton.button)
        {
#define B(a, b) case SDL_CONTROLLER_BUTTON_##a: button = InputControllerButton::b; break
        B(A, A);
        B(B, B);
        B(X, X);
        B(Y, Y);
        B(BACK, Back);
        B(GUIDE, Guide);
        B(START, Start);
        B(LEFTSTICK, LeftStick);
        B(RIGHTSTICK, RightStick);
        B(LEFTSHOULDER, LeftShoulder);
        B(RIGHTSHOULDER, RightShoulder);
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
            .controllerId = (InputControllerId)sdlEvent.cbutton.which,
            .event = InputControllerEvent::ButtonEvent
            {
              .button = button,
              .isPressed = sdlEvent.type == SDL_CONTROLLERBUTTONDOWN,
            },
          });
        }
      }
      break;
    case SDL_CONTROLLERAXISMOTION:
      // controller axis motion event
      {
        InputControllerAxis axis;
        bool ok = true;
        switch(sdlEvent.caxis.axis)
        {
#define A(a, b) case SDL_CONTROLLER_AXIS_##a: axis = InputControllerAxis::b; break
        A(LEFTX, LeftX);
        A(LEFTY, LeftY);
        A(RIGHTX, RightX);
        A(RIGHTY, RightY);
        A(TRIGGERLEFT, TriggerLeft);
        A(TRIGGERRIGHT, TriggerRight);
#undef A
        default: ok = false; break;
        }
        if(ok)
        {
          AddEvent(InputControllerEvent
          {
            .controllerId = (InputControllerId)sdlEvent.caxis.which,
            .event = InputControllerEvent::AxisMotionEvent
            {
              .axis = axis,
              .axisValue = sdlEvent.caxis.value,
            },
          });
        }
      }
      break;
    }
  }

  void SdlInputManager::SetVirtualScale(vec2 const& scale)
  {
    _scale = scale;
  }

  InputKey SdlInputManager::ConvertKey(SDL_Scancode code)
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

      // C(PLUS, Plus);
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

  SdlInputManager::SdlController::SdlController(SDL_GameController* controller, SDL_Joystick* joystick)
  : InputController(SDL_JoystickInstanceID(joystick))
  , _controller(controller)
  , _haptic(SDL_HapticOpenFromJoystick(joystick))
  , _hapticEffectIndex(-1)
  {}

  SdlInputManager::SdlController::SdlController(SDL_GameController* controller)
  : SdlController(controller, SDL_GameControllerGetJoystick(controller))
  {}

  SdlInputManager::SdlController::~SdlController()
  {
    Close();
  }

  bool SdlInputManager::SdlController::IsActive() const
  {
    return !!_controller;
  }

  void SdlInputManager::SdlController::RunHapticLeftRight(float left, float right)
  {
    if(!_haptic) return;

    _hapticEffect.leftright.length = SDL_HAPTIC_INFINITY;
    _hapticEffect.leftright.large_magnitude = uint16_t(left * 0xffff);
    _hapticEffect.leftright.small_magnitude = uint16_t(right * 0xffff);

    if(_hapticEffectIndex >= 0 && _hapticEffect.type == SDL_HAPTIC_LEFTRIGHT)
    {
      SDL_HapticStopEffect(_haptic, _hapticEffectIndex);
      SDL_HapticUpdateEffect(_haptic, _hapticEffectIndex, &_hapticEffect);
    }
    else
    {
      if(_hapticEffectIndex >= 0)
      {
        SDL_HapticStopEffect(_haptic, _hapticEffectIndex);
        SDL_HapticDestroyEffect(_haptic, _hapticEffectIndex);
      }

      _hapticEffect.type = SDL_HAPTIC_LEFTRIGHT;
      _hapticEffectIndex = SDL_HapticNewEffect(_haptic, &_hapticEffect);
    }

    SDL_HapticRunEffect(_haptic, _hapticEffectIndex, 1);
  }

  void SdlInputManager::SdlController::StopHaptic()
  {
    if(_hapticEffectIndex >= 0)
    {
      SDL_HapticStopEffect(_haptic, _hapticEffectIndex);
    }
  }

  void SdlInputManager::SdlController::Close()
  {
    if(_hapticEffectIndex >= 0)
    {
      SDL_HapticStopEffect(_haptic, _hapticEffectIndex);
      SDL_HapticDestroyEffect(_haptic, _hapticEffectIndex);
      _hapticEffectIndex = -1;
    }

    if(_haptic)
    {
      SDL_HapticClose(_haptic);
      _haptic = nullptr;
    }

    if(_controller)
    {
      SDL_GameControllerClose(_controller);
      _controller = nullptr;
    }
  }

  SdlWindow::SdlWindow(SDL_Window* window)
  : _window(window), _windowId(SDL_GetWindowID(_window))
  {
    SDL_GetWindowSize(_window, &_virtualSize.x(), &_virtualSize.y());
    SDL_GL_GetDrawableSize(_window, &_clientSize.x(), &_clientSize.y());
    _dpiScale = float(_clientSize.x()) / float(_virtualSize.x());
    UpdateInputManager();
  }

  SdlWindow::~SdlWindow()
  {
    Close();
  }

  void SdlWindow::SetTitle(std::string const& title)
  {
    SDL_SetWindowTitle(_window, title.c_str());
  }

  void SdlWindow::Close()
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

  void SdlWindow::SetFullScreen(bool fullScreen)
  {
    if(_fullScreen == fullScreen) return;
    _fullScreen = fullScreen;
    SDL_SetWindowFullscreen(_window, _fullScreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
  }

  ivec2 SdlWindow::GetDrawableSize() const
  {
    return _clientSize;
  }

  float SdlWindow::GetDPIScale() const
  {
    return _dpiScale;
  }

  SdlInputManager& SdlWindow::GetInputManager()
  {
    return _inputManager;
  }

  void SdlWindow::Run(std::function<void()> const& loop)
  {
    _running = true;

    while(_running)
    {
      SDL_Event event;
      while((_loopOnlyVisible && !_visible ? SDL_WaitEvent : SDL_PollEvent)(&event))
      {
        _inputManager.ProcessEvent(event);

        switch(event.type)
        {
        case SDL_WINDOWEVENT:
          switch(event.window.event)
          {
          case SDL_WINDOWEVENT_SIZE_CHANGED:
            SDL_GetWindowSize(_window, &_virtualSize.x(), &_virtualSize.y());
            SDL_GL_GetDrawableSize(_window, &_clientSize.x(), &_clientSize.y());
            _dpiScale = float(_clientSize.x()) / float(_virtualSize.x());
            UpdateInputManager();
            if(_presenter)
            {
              _presenter->Resize(GetDrawableSize());
            }
            break;
          case SDL_WINDOWEVENT_MINIMIZED:
            _visible = false;
            break;
          case SDL_WINDOWEVENT_RESTORED:
          case SDL_WINDOWEVENT_MAXIMIZED:
            _visible = true;
            break;
          case SDL_WINDOWEVENT_FOCUS_LOST:
            _inputManager.ReleaseButtonsOnUpdate();
            break;
          }
          break;
        case SDL_QUIT:
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

  void SdlWindow::PlaceCursor(ivec2 const& cursor)
  {
    SDL_WarpMouseInWindow(_window, cursor.x(), cursor.y());
  }

  SDL_Window* SdlWindow::GetSdlWindow() const
  {
    return _window;
  }

  void SdlWindow::_UpdateMouseLock()
  {
    SDL_SetRelativeMouseMode(_mouseLock ? SDL_TRUE : SDL_FALSE);
  }

  void SdlWindow::_UpdateCursorVisible()
  {
    SDL_ShowCursor(_cursorVisible ? SDL_ENABLE : SDL_DISABLE);
  }

  void SdlWindow::UpdateInputManager()
  {
    _inputManager.SetVirtualScale(
    {
      (float)_clientSize.x() / (float)_virtualSize.x(),
      (float)_clientSize.y() / (float)_virtualSize.y(),
    });
  }

  SdlWindow& SdlWindowSystem::CreateWindow(Book& book, std::string const& title, ivec2 const& size)
  {
    SDL_Window* window = SDL_CreateWindow(title.c_str(),
      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      size.x(), size.y(),
      SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_VULKAN
    );
    if(!window)
      throw Exception("failed to create SDL window");
    return book.Allocate<SdlWindow>(window);
  }
}
