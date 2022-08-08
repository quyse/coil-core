#pragma once

#include "base.hpp"

namespace Coil::Input
{
  // Key numbers.
  struct Keys
  {
    enum _
    {
      _Unknown = 0,

      BackSpace = 0x08,
      Tab = 0x09,
      LineFeed = 0x0a,
      Clear = 0x0b,
      Return = 0x0d,
      Pause = 0x13,
      ScrollLock = 0x14,
      SysReq = 0x15,
      Escape = 0x1b,
      Insert = 0x1c,
      Delete = 0x1d,

      Space = ' ', // 0x20

      Plus = '+', // 0x2b
      Comma = ',', // 0x2c
      Hyphen = '-', // 0x2d
      Period = '.', // 0x2e
      Slash = '/', // 0x2f

      _0 = '0', // 0x30
      _1 = '1',
      _2 = '2',
      _3 = '3',
      _4 = '4',
      _5 = '5',
      _6 = '6',
      _7 = '7',
      _8 = '8',
      _9 = '9', // 0x39

      A = 'A', // 0x41
      B = 'B',
      C = 'C',
      D = 'D',
      E = 'E',
      F = 'F',
      G = 'G',
      H = 'H',
      I = 'I',
      J = 'J',
      K = 'K',
      L = 'L',
      M = 'M',
      N = 'N',
      O = 'O',
      P = 'P',
      Q = 'Q',
      R = 'R',
      S = 'S',
      T = 'T',
      U = 'U',
      V = 'V',
      W = 'W',
      X = 'X',
      Y = 'Y',
      Z = 'Z', // 0x5A

      Home = 0x60,
      Left = 0x61,
      Up = 0x62,
      Right = 0x63,
      Down = 0x64,
      PageUp = 0x65,
      PageDown = 0x66,
      End = 0x67,
      Begin = 0x68,
      Grave = 0x7e,

      // keypad
      NumLock = 0x7f,
      NumPadSpace = 0x80,
      NumPadTab = 0x89,
      NumPadEnter = 0x8d,
      NumPadF1 = 0x91,
      NumPadF2 = 0x92,
      NumPadF3 = 0x93,
      NumPadF4 = 0x94,
      NumPadHome = 0x95,
      NumPadLeft = 0x96,
      NumPadUp = 0x97,
      NumPadRight = 0x98,
      NumPadDown = 0x99,
      NumPadPageUp = 0x9a,
      NumPadPageDown = 0x9b,
      NumPadEnd = 0x9c,
      NumPadBegin = 0x9d,
      NumPadInsert = 0x9e,
      NumPadDelete = 0x9f,
      NumPadEqual = 0xbd,
      NumPadMultiply = 0xaa,
      NumPadAdd = 0xab,
      NumPadSeparator = 0xac, // comma
      NumPadSubtract = 0xad,
      NumPadDecimal = 0xae,
      NumPadDivide = 0xaf,

      NumPad0 = 0xb0,
      NumPad1 = 0xb1,
      NumPad2 = 0xb2,
      NumPad3 = 0xb3,
      NumPad4 = 0xb4,
      NumPad5 = 0xb5,
      NumPad6 = 0xb6,
      NumPad7 = 0xb7,
      NumPad8 = 0xb8,
      NumPad9 = 0xb9,

      F1 = 0xbe,
      F2 = 0xbf,
      F3 = 0xc0,
      F4 = 0xc1,
      F5 = 0xc2,
      F6 = 0xc3,
      F7 = 0xc4,
      F8 = 0xc5,
      F9 = 0xc6,
      F10 = 0xc7,
      F11 = 0xc8,
      F12 = 0xc9,

      ShiftL = 0xe1,
      ShiftR = 0xe2,
      ControlL = 0xe3,
      ControlR = 0xe4,
      CapsLock = 0xe5,
      AltL = 0xe9,
      AltR = 0xea,
      SuperL = 0xeb,
      SuperR = 0xec,

      // virtual keys
      Shift = 0xed,
      Control = 0xee,
      Alt = 0xef
    };
  };
  using Key = Keys::_;

  // Controller ID.
  using ControllerId = uint64_t;

  // Input event.
  struct Event
  {
    enum Device
    {
      deviceKeyboard,
      deviceMouse,
      deviceController
    } device;

    struct Keyboard
    {
      enum Type
      {
        typeKeyDown,
        typeKeyUp,
        typeCharacter
      } type;

      union
      {
        // in case type == typeKey{Down,Up}
        Key key;
        // in case type == typeCharacter
        char32_t character;
      };
    };

    struct Mouse
    {
      enum Type
      {
        typeButtonDown,
        typeButtonUp,
        typeRawMove,
        typeCursorMove,
        typeDoubleClick
      } type;

      enum Button
      {
        buttonLeft,
        buttonRight,
        buttonMiddle
      };

      union
      {
        // in case type == typeButton{Down,Up,DoubleClick}
        Button button;
        // in case type == typeRawMove
        struct
        {
          //*** Raw relative moving.
          /** In "device" arbitrary units, no kind of acceleration applied. */
          float rawMoveX;
          float rawMoveY;
          float rawMoveZ;
        };
        // in case type == typeCursorMove
        struct
        {
          //*** Absolute cursor position (in window coordinates).
          int cursorX;
          int cursorY;
          //*** Relative mouse wheel movement.
          int cursorZ;
        };
      };
    };

    struct Controller
    {
      enum Type
      {
        typeDeviceAdded,
        typeDeviceRemoved,
        typeButtonDown,
        typeButtonUp,
        typeAxisMotion
      } type;

      enum Button
      {
        buttonA,
        buttonB,
        buttonX,
        buttonY,
        buttonBack,
        buttonGuide,
        buttonStart,
        buttonLeftStick,
        buttonRightStick,
        buttonLeftShoulder,
        buttonRightShoulder,
        buttonDPadUp,
        buttonDPadDown,
        buttonDPadLeft,
        buttonDPadRight
      };

      enum Axis
      {
        axisLeftX,
        axisLeftY,
        axisRightX,
        axisRightY,
        axisTriggerLeft,
        axisTriggerRight
      };

      // Device id (for all event types).
      ControllerId device;

      union
      {
        // in case type == typeButton{Down,Up}
        Button button;
        // in case type == typeAxisMotion
        struct
        {
          Axis axis;
          int axisValue;
        };
      };
    };

    union
    {
      // in case device == deviceKeyboard
      Keyboard keyboard;
      // in case device == deviceMouse
      Mouse mouse;
      // in case device == deviceController
      Controller controller;
    };
  };

  // Input state.
  struct State
  {
    // Keyboard keys state.
    uint8_t keyboard[256] = { 0 };
    // Mouse buttons state.
    bool mouseButtons[3] = { false };
    // Mouse cursor position.
    int cursorX = 0, cursorY = 0;
  };

  // Input frame.
  class Frame
  {
  public:
    Event const* NextEvent();
    State const& GetCurrentState() const;
    void ForwardEvents();

    void Reset();
    void AddEvent(Event const& event);

  private:
    void _ProcessKeyboardVirtualEvents(Event const& event);

    std::vector<Event> _events;
    size_t _nextEvent = 0;
    State _state;
  };

  // Game controller.
  class Controller
  {
  public:
    // Get controller id.
    ControllerId GetId() const;

    // Return if controller is still connected.
    virtual bool IsActive() const = 0;
    virtual void RunHapticLeftRight(float left, float right) = 0;
    virtual void StopHaptic() = 0;

  protected:
    Controller(ControllerId id);

    // Controller id to distinguish events for this controller.
    ControllerId _id;
  };

  class Manager
  {
  public:
    // Go to the next frame.
    virtual void Update();
    // Get current input frame.
    Frame const& GetCurrentFrame() const;

    // Send up events for all keys on next Update.
    void ReleaseButtonsOnUpdate();

    // Start text input.
    virtual void StartTextInput();
    // Stop text input.
    virtual void StopTextInput();
    // Is text input enabled.
    bool IsTextInput() const;

  protected:
    void AddEvent(Event const& event);

    Frame _frames[2];
    Frame* _currentFrame = &_frames[0];
    Frame* _internalFrame = &_frames[1];

    bool _textInputEnabled = false;
    bool _releaseButtonsOnUpdate = false;
  };
}
