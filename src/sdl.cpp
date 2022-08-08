#include "sdl.hpp"
#include "utf8.hpp"

namespace Coil::Platform
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

  void Sdl::Init(Book& book, uint32_t flags)
  {
    if(flags)
    {
      book.Allocate<Sdl>(flags);
    }
  }

  void SdlInputManager::Update()
  {
  }

  void SdlInputManager::ProcessEvent(SDL_Event const& sdlEvent)
  {
    // using Input::Event;

    switch(sdlEvent.type)
    {
    case SDL_KEYDOWN:
    case SDL_KEYUP:
      {
        Input::Event event;
        event.device = Input::Event::deviceKeyboard;
        event.keyboard.type = sdlEvent.type == SDL_KEYDOWN ? Input::Event::Keyboard::typeKeyDown : Input::Event::Keyboard::typeKeyUp;
        event.keyboard.key = ConvertKey(sdlEvent.key.keysym.scancode);
        AddEvent(event);
      }
      break;
    case SDL_TEXTINPUT:
      for(Utf8::Char32Iterator i(sdlEvent.text.text); *i; ++i)
      {
        Input::Event event;
        event.device = Input::Event::deviceKeyboard;
        event.keyboard.type = Input::Event::Keyboard::typeCharacter;
        event.keyboard.character = *i;
        AddEvent(event);
      }
      break;
    case SDL_MOUSEMOTION:
      // raw move event
      {
        Input::Event event;
        event.device = Input::Event::deviceMouse;
        event.mouse.type = Input::Event::Mouse::typeRawMove;
        event.mouse.rawMoveX = sdlEvent.motion.xrel;
        event.mouse.rawMoveY = sdlEvent.motion.yrel;
        event.mouse.rawMoveZ = 0;
        AddEvent(event);
      }
      // cursor move event
      {
        Input::Event event;
        event.device = Input::Event::deviceMouse;
        event.mouse.type = Input::Event::Mouse::typeCursorMove;
        _lastCursorX = event.mouse.cursorX = (int)(sdlEvent.motion.x * _widthScale);
        _lastCursorY = event.mouse.cursorY = (int)(sdlEvent.motion.y * _heightScale);
        event.mouse.cursorZ = 0;
        AddEvent(event);
      }
      break;
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
      {
        Input::Event event;
        event.device = Input::Event::deviceMouse;
        event.mouse.type = sdlEvent.type == SDL_MOUSEBUTTONDOWN ? Input::Event::Mouse::typeButtonDown : Input::Event::Mouse::typeButtonUp;
        bool ok = true;
        switch(sdlEvent.button.button)
        {
        case SDL_BUTTON_LEFT:
          event.mouse.button = Input::Event::Mouse::buttonLeft;
          break;
        case SDL_BUTTON_MIDDLE:
          event.mouse.button = Input::Event::Mouse::buttonMiddle;
          break;
        case SDL_BUTTON_RIGHT:
          event.mouse.button = Input::Event::Mouse::buttonRight;
          break;
        default:
          ok = false;
          break;
        }
        if(ok)
        {
          AddEvent(event);
          // send double click event for second button up
          if(event.mouse.type == Input::Event::Mouse::typeButtonUp && sdlEvent.button.clicks == 2)
          {
            event.mouse.type = Input::Event::Mouse::typeDoubleClick;
            AddEvent(event);
          }
        }
      }
      break;
    case SDL_MOUSEWHEEL:
      // raw move event
      {
        Input::Event event;
        event.device = Input::Event::deviceMouse;
        event.mouse.type = Input::Event::Mouse::typeRawMove;
        event.mouse.rawMoveX = 0;
        event.mouse.rawMoveY = 0;
        event.mouse.rawMoveZ = sdlEvent.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -sdlEvent.wheel.y : sdlEvent.wheel.y;
        AddEvent(event);
      }
      // cursor move event
      {
        Input::Event event;
        event.device = Input::Event::deviceMouse;
        event.mouse.type = Input::Event::Mouse::typeCursorMove;
        event.mouse.cursorX = _lastCursorX;
        event.mouse.cursorY = _lastCursorY;
        event.mouse.cursorZ = sdlEvent.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -sdlEvent.wheel.y : sdlEvent.wheel.y;
        AddEvent(event);
      }
      break;
    case SDL_CONTROLLERDEVICEADDED:
      // device added event
      {
        // open controller
        SDL_GameController* sdlController = SDL_GameControllerOpen(sdlEvent.cdevice.which);
        if(sdlController)
        {
          auto controller = std::make_unique<SdlController>(sdlController);
          Input::ControllerId controllerId = controller->GetId();
          _controllers.insert({ controllerId, std::move(controller) });

          Input::Event event;
          event.device = Input::Event::deviceController;
          event.controller.type = Input::Event::Controller::typeDeviceAdded;
          event.controller.device = controllerId;
          AddEvent(event);
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
          Input::Event event;
          event.device = Input::Event::deviceController;
          event.controller.type = Input::Event::Controller::typeDeviceRemoved;
          event.controller.device = sdlEvent.cdevice.which;
          AddEvent(event);
        }
      }
      break;
    case SDL_CONTROLLERBUTTONDOWN:
    case SDL_CONTROLLERBUTTONUP:
      // controller button down/up event
      {
        Input::Event::Controller::Button button;
        bool ok = true;
        switch(sdlEvent.cbutton.button)
        {
#define B(a, b) case SDL_CONTROLLER_BUTTON_##a: button = Input::Event::Controller::button##b; break
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
          Input::Event event;
          event.device = Input::Event::deviceController;
          event.controller.type = sdlEvent.type == SDL_CONTROLLERBUTTONDOWN ? Input::Event::Controller::typeButtonDown : Input::Event::Controller::typeButtonUp;
          event.controller.device = sdlEvent.cbutton.which;
          event.controller.button = button;
          AddEvent(event);
        }
      }
      break;
    case SDL_CONTROLLERAXISMOTION:
      // controller axis motion event
      {
        Input::Event::Controller::Axis axis;
        bool ok = true;
        switch(sdlEvent.caxis.axis)
        {
#define A(a, b) case SDL_CONTROLLER_AXIS_##a: axis = Input::Event::Controller::axis##b; break
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
          Input::Event event;
          event.device = Input::Event::deviceController;
          event.controller.type = Input::Event::Controller::typeAxisMotion;
          event.controller.device = sdlEvent.caxis.which;
          event.controller.axis = axis;
          event.controller.axisValue = sdlEvent.caxis.value;
          AddEvent(event);
        }
      }
      break;
    }
  }

  void SdlInputManager::SetVirtualScale(float widthScale, float heightScale)
  {
    _widthScale = widthScale;
    _heightScale = heightScale;
  }

  Input::Key SdlInputManager::ConvertKey(SDL_Scancode code)
  {
    Input::Key key;
    switch(code)
    {
  #define C(c, k) case SDL_SCANCODE_##c: key = Input::Key::k; break

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
      key = Input::Key::_Unknown;
      break;
    }

    return key;
  }

  SdlInputManager::SdlController::SdlController(SDL_GameController* controller, SDL_Joystick* joystick)
  : Controller(SDL_JoystickInstanceID(joystick))
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
    SDL_GetWindowSize(_window, &_virtualWidth, &_virtualHeight);
    SDL_GL_GetDrawableSize(_window, &_clientWidth, &_clientHeight);
    _dpiScale = float(_clientWidth) / float(_virtualWidth);
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
      SDL_DestroyWindow(_window);
      _window = nullptr;
    }
  }

  void SdlWindow::SetFullScreen(bool fullScreen)
  {
    if(_fullScreen == fullScreen) return;
    _fullScreen = fullScreen;
    SDL_SetWindowFullscreen(_window, _fullScreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
  }

  float SdlWindow::GetDPIScale() const
  {
    return _dpiScale;
  }

  void SdlWindow::Run(std::function<void()> const& loop)
  {
    _running = true;

    while(_running)
    {
      SDL_Event event;
      while(SDL_PollEvent(&event))
      {
        if(_inputManager)
        {
          _inputManager->ProcessEvent(event);
        }

        switch(event.type)
        {
        case SDL_WINDOWEVENT:
          switch(event.window.event)
          {
          case SDL_WINDOWEVENT_SIZE_CHANGED:
            SDL_GetWindowSize(_window, &_virtualWidth, &_virtualHeight);
            SDL_GL_GetDrawableSize(_window, &_clientWidth, &_clientHeight);
            _dpiScale = float(_clientWidth) / float(_virtualWidth);
            if(_inputManager)
            {
              _inputManager->SetVirtualScale((float)_clientWidth / (float)_virtualWidth, (float)_clientHeight / (float)_virtualHeight);
            }
            // if(_presenter)
            // {
            //   _presenter->Resize(_clientWidth, _clientHeight);
            // }
            break;
          case SDL_WINDOWEVENT_CLOSE:
            if(_preventUserClose) Stop();
            else Close();
            break;
          case SDL_WINDOWEVENT_FOCUS_LOST:
            if(_inputManager)
            {
              _inputManager->ReleaseButtonsOnUpdate();
            }
            break;
          }
          break;
        case SDL_QUIT:
          Close();
          break;
        }
      }

      if(_inputManager)
      {
        _inputManager->Update();
      }

      loop();
    }
  }

  void SdlWindow::PlaceCursor(int x, int y)
  {
    SDL_WarpMouseInWindow(_window, x, y);
  }

  void SdlWindow::_UpdateMouseLock()
  {
    SDL_SetRelativeMouseMode(_mouseLock ? SDL_TRUE : SDL_FALSE);
  }

  void SdlWindow::_UpdateCursorVisible()
  {
    SDL_ShowCursor(_cursorVisible ? SDL_ENABLE : SDL_DISABLE);
  }

  SdlWindow& SdlWindowSystem::CreateWindow(Book& book, std::string const& title, int width, int height)
  {
    SDL_Window* window = SDL_CreateWindow(title.c_str(),
      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      width, height,
      SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
    );
    if(!window)
      throw Exception("failed to create SDL window");
    return book.Allocate<SdlWindow>(window);
  }
}
