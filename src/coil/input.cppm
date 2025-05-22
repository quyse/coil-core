module;

#include <bitset>
#include <concepts>
#include <unordered_map>
#include <variant>
#include <vector>

export module coil.core.input;

import coil.core.base;
import coil.core.math;

export namespace Coil
{
  // Controller ID.
  using InputControllerId = uint64_t;

  // Key numbers.
  enum class InputKey : uint8_t
  {
#define K(key, str, value) key = value,
#include "input_keys.hpp"
#undef K
  };

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
    float wheel;
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
  struct InputState
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
    InputEvent const* NextEvent()
    {
      while(_nextEvent < _events.size())
      {
        // process next event in state
        InputEvent const& event = _events[_nextEvent++];

        if(!std::visit([&]<typename E1>(E1 const& event) -> bool
        {
          if constexpr(std::same_as<E1, InputKeyboardEvent>)
          {
            return std::visit([&]<typename E2>(E2 const& event) -> bool
            {
              if constexpr(std::same_as<E2, InputKeyboardKeyEvent>)
              {
                // if key is already in this state, skip
                if(_state[event.key] == event.isPressed) return false;
                // otherwise apply state
                _state[event.key] = event.isPressed;
                return true;
              }
              if constexpr(std::same_as<E2, InputKeyboardCharacterEvent>)
              {
                return true;
              }
            }, event);
          }
          if constexpr(std::same_as<E1, InputMouseEvent>)
          {
            return std::visit([&]<typename E2>(E2 const& event) -> bool
            {
              if constexpr(std::same_as<E2, InputMouseButtonEvent>)
              {
                // if button already in this state, skip
                if(_state[event.button] == event.isPressed) return false;
                // otherwise apply state
                _state[event.button] = event.isPressed;
                return true;
              }
              if constexpr(std::same_as<E2, InputMouseRawMoveEvent>)
              {
                // no state for raw move
                return true;
              }
              if constexpr(std::same_as<E2, InputMouseCursorMoveEvent>)
              {
                _state.cursor = event.cursor;
                return true;
              }
            }, event);
          }
          if constexpr(std::same_as<E1, InputControllerEvent>)
          {
            // TODO: controller state
            return true;
          }
        }, event)) continue;

        // if we are here, event changes something
        return &event;
      }

      return nullptr;
    }

    InputState const& GetCurrentState() const
    {
      return _state;
    }

    void ForwardEvents()
    {
      while(NextEvent());
    }

    void Reset()
    {
      _events.clear();
      _nextEvent = 0;
    }

    void AddEvent(InputEvent const& event)
    {
      _events.push_back(event);
    }

  private:
    std::vector<InputEvent> _events;
    size_t _nextEvent = 0;
    InputState _state;
  };

  // Input controller (gamepad).
  class InputController
  {
  public:
    // Get controller id.
    InputControllerId GetId() const
    {
      return _id;
    }

    // Return if controller is still connected.
    virtual bool IsActive() const = 0;
    virtual void RunHapticLeftRight(float left, float right) = 0;
    virtual void StopHaptic() = 0;

  protected:
    InputController(InputControllerId id)
    : _id(id) {}

    // Controller id to distinguish events for this controller.
    InputControllerId _id;
  };

  class InputManager
  {
  public:
    // Go to the next frame.
    virtual void Update()
    {
      // swap frames
      std::swap(_currentFrame, _internalFrame);
      // old internal frame (new current frame) should be copied to new internal frame,
      // and state is rewinded forward
      *_internalFrame = *_currentFrame;
      _internalFrame->ForwardEvents();
      _internalFrame->Reset();

      // add release events to internal frame if needed
      if(_releaseButtonsOnUpdate)
      {
        _releaseButtonsOnUpdate = false;

        const InputState& state = _internalFrame->GetCurrentState();
        for(size_t i = 0; i < state.keyboard.size(); ++i)
          if(state.keyboard[i])
          {
            _internalFrame->AddEvent(InputKeyboardEvent
            {
              InputKeyboardKeyEvent
              {
                .key = InputKey(i),
                .isPressed = false,
              }
            });
          }
        for(size_t i = 0; i < sizeof(state.mouseButtons) / sizeof(state.mouseButtons[0]); ++i)
          if(state.mouseButtons[i])
          {
            _internalFrame->AddEvent(InputMouseEvent
            {
              InputMouseButtonEvent
              {
                .button = InputMouseButton(i),
                .isPressed = false,
              }
            });
          }
      }
    }

    // Get current input frame.
    InputFrame& GetCurrentFrame() const
    {
      return *_currentFrame;
    }

    // Send up events for all keys on next Update.
    void ReleaseButtonsOnUpdate()
    {
      _releaseButtonsOnUpdate = true;
    }

    // Start text input.
    virtual void StartTextInput()
    {
      _textInputEnabled = true;
    }

    // Stop text input.
    virtual void StopTextInput()
    {
      _textInputEnabled = false;
    }

    // Is text input enabled.
    bool IsTextInput() const
    {
      return _textInputEnabled;
    }

  protected:
    void AddEvent(InputEvent const& event)
    {
      // skip text input events if text input is disabled
      if(!_textInputEnabled && !std::visit([&]<typename E1>(E1 const& event) -> bool
      {
        if constexpr(std::same_as<E1, InputKeyboardEvent>)
        {
          return std::visit([&]<typename E2>(E2 const& event) -> bool
          {
            if constexpr(std::same_as<E2, InputKeyboardCharacterEvent>)
            {
              return false;
            }
            return true;
          }, event);
        }
        return true;
      }, event)) return;

      _internalFrame->AddEvent(event);
    }

    InputFrame _frames[2];
    InputFrame* _currentFrame = &_frames[0];
    InputFrame* _internalFrame = &_frames[1];

    bool _textInputEnabled = false;
    bool _releaseButtonsOnUpdate = false;
  };

  template <> std::string ToString(InputKey const& key)
  {
    switch(key)
    {
#define K(key, str, value) case InputKey::key: return #str;
#include "input_keys.hpp"
#undef K
    default: return {};
    }
  }
  template <> InputKey FromString(std::string_view str)
  {
    static std::unordered_map<std::string_view, InputKey> const keys =
    {
#define K(key, str, value) { #str, InputKey::key },
#include "input_keys.hpp"
#undef K
    };

    auto i = keys.find(str);
    if(i == keys.end()) return InputKey::_Unknown;
    return i->second;
  }

  template <> std::string ToString(InputMouseButton const& button)
  {
    switch(button)
    {
#define B(b) case InputMouseButton::b: return #b;
    B(Left)
    B(Right)
    B(Middle)
#undef B
    default: return {};
    }
  }
  template <> InputMouseButton FromString(std::string_view str)
  {
    static std::unordered_map<std::string_view, InputMouseButton> const buttons =
    {
#define B(b) { #b, InputMouseButton::b },
    B(Left)
    B(Right)
    B(Middle)
#undef B
    };

    auto i = buttons.find(str);
    if(i == buttons.end())
      throw Exception("unknown mouse button");
    return i->second;
  }
}
