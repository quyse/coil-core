#include "input.hpp"

namespace Coil
{
  InputEvent const* InputFrame::NextEvent()
  {
    while(_nextEvent < _events.size())
    {
      // process next event in state
      InputEvent const& event = _events[_nextEvent++];

      if(!std::visit([&](auto const& event) -> bool
      {
        using E = std::decay_t<decltype(event)>;
        if constexpr(std::is_same_v<E, InputKeyboardEvent>)
        {
          return std::visit([&](auto const& event) -> bool
          {
            using E = std::decay_t<decltype(event)>;
            if constexpr(std::is_same_v<E, InputKeyboardKeyEvent>)
            {
              // if key is already in this state, skip
              if(_state[event.key] == event.isPressed) return false;
              // otherwise apply state
              _state[event.key] = event.isPressed;
              _ProcessKeyboardVirtualEvents(event);
              return true;
            }
            if constexpr(std::is_same_v<E, InputKeyboardCharacterEvent>)
            {
              return true;
            }
          }, event);
        }
        if constexpr(std::is_same_v<E, InputMouseEvent>)
        {
          return std::visit([&](auto const& event) -> bool
          {
            using E = std::decay_t<decltype(event)>;
            if constexpr(std::is_same_v<E, InputMouseButtonEvent>)
            {
              // if button already in this state, skip
              if(_state[event.button] == event.isPressed) return false;
              // otherwise apply state
              _state[event.button] = event.isPressed;
              return true;
            }
            if constexpr(std::is_same_v<E, InputMouseRawMoveEvent>)
            {
              // no state for raw move
              return true;
            }
            if constexpr(std::is_same_v<E, InputMouseCursorMoveEvent>)
            {
              _state.cursor = event.cursor;
              return true;
            }
          }, event);
        }
        if constexpr(std::is_same_v<E, InputControllerEvent>)
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

  State const& InputFrame::GetCurrentState() const
  {
    return _state;
  }

  void InputFrame::ForwardEvents()
  {
    while(NextEvent());
  }

  void InputFrame::Reset()
  {
    _events.clear();
    _nextEvent = 0;
  }

  void InputFrame::AddEvent(InputEvent const& event)
  {
    _events.push_back(event);
  }

  void InputFrame::_ProcessKeyboardVirtualEvents(InputKeyboardKeyEvent const& event)
  {
    InputKey virtualKey;
    bool newPressed;
    switch(event.key)
    {
    case InputKey::ShiftL:
    case InputKey::ShiftR:
      virtualKey = InputKey::Shift;
      newPressed = _state[InputKey::ShiftL] || _state[InputKey::ShiftR];
      break;
    case InputKey::ControlL:
    case InputKey::ControlR:
      virtualKey = InputKey::Control;
      newPressed = _state[InputKey::ControlL] || _state[InputKey::ControlR];
      break;
    case InputKey::AltL:
    case InputKey::AltR:
      virtualKey = InputKey::Alt;
      newPressed = _state[InputKey::AltL] || _state[InputKey::AltR];
      break;
    default:
      return;
    }

    bool oldPressed = _state[virtualKey];

    if(newPressed != oldPressed)
    {
      InputEvent virtualEvent = InputKeyboardEvent
      {
        InputKeyboardKeyEvent
        {
          .key = virtualKey,
          .isPressed = newPressed,
        }
      };
      _events.push_back(virtualEvent);
    }
  }

  InputControllerId InputController::GetId() const
  {
    return _id;
  }

  InputController::InputController(InputControllerId id)
  : _id(id) {}

  void InputManager::Update()
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

      const State& state = _internalFrame->GetCurrentState();
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

  InputFrame& InputManager::GetCurrentFrame() const
  {
    return *_currentFrame;
  }

  void InputManager::ReleaseButtonsOnUpdate()
  {
    _releaseButtonsOnUpdate = true;
  }

  void InputManager::StartTextInput()
  {
    _textInputEnabled = true;
  }

  void InputManager::StopTextInput()
  {
    _textInputEnabled = false;
  }

  bool InputManager::IsTextInput() const
  {
    return _textInputEnabled;
  }

  void InputManager::AddEvent(InputEvent const& event)
  {
    // skip text input events if text input is disabled
    if(!_textInputEnabled && !std::visit([&](auto const& event) -> bool
    {
      using E = std::decay_t<decltype(event)>;
      if constexpr(std::is_same_v<E, InputKeyboardEvent>)
      {
        return std::visit([&](auto const& event) -> bool
        {
          using E = std::decay_t<decltype(event)>;
          if constexpr(std::is_same_v<E, InputKeyboardCharacterEvent>)
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
}
