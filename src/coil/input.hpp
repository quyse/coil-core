#pragma once

#include "math.hpp"
#include "input_keys.hpp"
#include <variant>
#include <bitset>

namespace Coil
{
  // Controller ID.
  using InputControllerId = uint64_t;

  // keyboard event
  struct InputKeyboardKeyEvent
  {
    InputKey key;
    bool isPressed;
  };
  struct InputKeyboardCharacterEvent
  {
    char32_t character;
  };
  using InputKeyboardEvent = std::variant<InputKeyboardKeyEvent, InputKeyboardCharacterEvent>;

  // mouse event
  enum class InputMouseButton : uint8_t
  {
    Left,
    Right,
    Middle,
  };
  struct InputMouseButtonEvent
  {
    InputMouseButton button;
    bool isPressed;
  };
  struct InputMouseRawMoveEvent
  {
    // raw relative movement in device units
    vec3 rawMove;
  };
  struct InputMouseCursorMoveEvent
  {
    // absolute cursor position
    ivec2 cursor;
    // relative mouse wheel change
    int32_t wheel;
  };
  using InputMouseEvent = std::variant<InputMouseButtonEvent, InputMouseRawMoveEvent, InputMouseCursorMoveEvent>;

  // controller event
  enum class InputControllerButton : uint8_t
  {
    A,
    B,
    X,
    Y,
    Back,
    Guide,
    Start,
    LeftStick,
    RightStick,
    LeftShoulder,
    RightShoulder,
    DPadUp,
    DPadDown,
    DPadLeft,
    DPadRight,
  };
  enum class InputControllerAxis : uint8_t
  {
    LeftX,
    LeftY,
    RightX,
    RightY,
    TriggerLeft,
    TriggerRight,
  };
  struct InputControllerEvent
  {
    InputControllerId controllerId;
    struct ControllerEvent
    {
      bool isAdded;
    };
    struct ButtonEvent
    {
      InputControllerButton button;
      bool isPressed;
    };
    struct AxisMotionEvent
    {
      InputControllerAxis axis;
      int32_t axisValue;
    };
    std::variant<ControllerEvent, ButtonEvent, AxisMotionEvent> event;
  };

  using InputEvent = std::variant<InputKeyboardEvent, InputMouseEvent, InputControllerEvent>;

  // Input state.
  struct State
  {
    // Keyboard keys state.
    std::bitset<256> keyboard;
    // Mouse buttons state.
    bool mouseButtons[3] = { false };
    // Mouse cursor position.
    ivec2 cursor;

    auto operator[](InputKey key)
    {
      return keyboard[(size_t)key];
    }
    auto operator[](InputKey key) const
    {
      return keyboard[(size_t)key];
    }

    bool& operator[](InputMouseButton button)
    {
      return mouseButtons[(size_t)button];
    }
    bool operator[](InputMouseButton button) const
    {
      return mouseButtons[(size_t)button];
    }
  };

  // Input frame.
  class InputFrame
  {
  public:
    InputEvent const* NextEvent();
    State const& GetCurrentState() const;
    void ForwardEvents();

    void Reset();
    void AddEvent(InputEvent const& event);

  private:
    void _ProcessKeyboardVirtualEvents(InputKeyboardKeyEvent const& event);

    std::vector<InputEvent> _events;
    size_t _nextEvent = 0;
    State _state;
  };

  // Input controller (gamepad).
  class InputController
  {
  public:
    // Get controller id.
    InputControllerId GetId() const;

    // Return if controller is still connected.
    virtual bool IsActive() const = 0;
    virtual void RunHapticLeftRight(float left, float right) = 0;
    virtual void StopHaptic() = 0;

  protected:
    InputController(InputControllerId id);

    // Controller id to distinguish events for this controller.
    InputControllerId _id;
  };

  class InputManager
  {
  public:
    // Go to the next frame.
    virtual void Update();
    // Get current input frame.
    InputFrame& GetCurrentFrame() const;

    // Send up events for all keys on next Update.
    void ReleaseButtonsOnUpdate();

    // Start text input.
    virtual void StartTextInput();
    // Stop text input.
    virtual void StopTextInput();
    // Is text input enabled.
    bool IsTextInput() const;

  protected:
    void AddEvent(InputEvent const& event);

    InputFrame _frames[2];
    InputFrame* _currentFrame = &_frames[0];
    InputFrame* _internalFrame = &_frames[1];

    bool _textInputEnabled = false;
    bool _releaseButtonsOnUpdate = false;
  };
}
