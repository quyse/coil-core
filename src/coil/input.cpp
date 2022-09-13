#include "input.hpp"

namespace Coil
{
  InputEvent const* InputFrame::NextEvent()
  {
    while(_nextEvent < _events.size())
    {
      // process next event in state
      InputEvent const& event = _events[_nextEvent++];

      switch(event.device)
      {
      case InputEvent::deviceKeyboard:
        switch(event.keyboard.type)
        {
        case InputEvent::Keyboard::typeKeyDown:
          // if key already pressed, skip
          if(_state.keyboard[event.keyboard.key])
            continue;
          _state.keyboard[event.keyboard.key] = 1;
          break;
        case InputEvent::Keyboard::typeKeyUp:
          // if key already released, skip
          if(!_state.keyboard[event.keyboard.key])
            continue;
          _state.keyboard[event.keyboard.key] = 0;
          break;
        case InputEvent::Keyboard::typeCharacter:
          break;
        }
        _ProcessKeyboardVirtualEvents(event);
        break;
      case InputEvent::deviceMouse:
        switch(event.mouse.type)
        {
        case InputEvent::Mouse::typeButtonDown:
          // if mouse button is already pressed, skip
          if(_state.mouseButtons[event.mouse.button])
            continue;
          _state.mouseButtons[event.mouse.button] = true;
          break;
        case InputEvent::Mouse::typeButtonUp:
          // if mouse button is already released, skip
          if(!_state.mouseButtons[event.mouse.button])
            continue;
          _state.mouseButtons[event.mouse.button] = false;
          break;
        case InputEvent::Mouse::typeRawMove:
          // there is no state for raw move
          break;
        case InputEvent::Mouse::typeCursorMove:
          _state.cursorX = event.mouse.cursorX;
          _state.cursorY = event.mouse.cursorY;
          break;
        case InputEvent::Mouse::typeDoubleClick:
          break;
        }
        break;
      case InputEvent::deviceController:
        // TODO: controller state
        break;
      }

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

  void InputFrame::_ProcessKeyboardVirtualEvents(InputEvent const& event)
  {
    switch(event.keyboard.type)
    {
    case InputEvent::Keyboard::typeKeyDown:
    case InputEvent::Keyboard::typeKeyUp:
      break;
    default:
      return;
    }

    InputKey virtualKey;
    bool newPressed;
    switch(event.keyboard.key)
    {
    case InputKey::ShiftL:
    case InputKey::ShiftR:
      virtualKey = InputKey::Shift;
      newPressed = _state.keyboard[InputKey::ShiftL] || _state.keyboard[InputKey::ShiftR];
      break;
    case InputKey::ControlL:
    case InputKey::ControlR:
      virtualKey = InputKey::Control;
      newPressed = _state.keyboard[InputKey::ControlL] || _state.keyboard[InputKey::ControlR];
      break;
    case InputKey::AltL:
    case InputKey::AltR:
      virtualKey = InputKey::Alt;
      newPressed = _state.keyboard[InputKey::AltL] || _state.keyboard[InputKey::AltR];
      break;
    default:
      return;
    }

    bool oldPressed = !!_state.keyboard[virtualKey];

    if(newPressed != oldPressed)
    {
      InputEvent virtualEvent;
      virtualEvent.device = InputEvent::deviceKeyboard;
      virtualEvent.keyboard.type = newPressed ? InputEvent::Keyboard::typeKeyDown : InputEvent::Keyboard::typeKeyUp;
      virtualEvent.keyboard.key = virtualKey;
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
      for(size_t i = 0; i < sizeof(state.keyboard) / sizeof(state.keyboard[0]); ++i)
        if(state.keyboard[i])
        {
          InputEvent event;
          event.device = InputEvent::deviceKeyboard;
          event.keyboard.type = InputEvent::Keyboard::typeKeyUp;
          event.keyboard.key = InputKey(i);
          _internalFrame->AddEvent(event);
        }
      for(size_t i = 0; i < sizeof(state.mouseButtons) / sizeof(state.mouseButtons[0]); ++i)
        if(state.mouseButtons[i])
        {
          InputEvent event;
          event.device = InputEvent::deviceMouse;
          event.mouse.type = InputEvent::Mouse::typeButtonUp;
          event.mouse.button = InputEvent::Mouse::Button(i);
          _internalFrame->AddEvent(event);
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
    if(
      event.device == InputEvent::deviceKeyboard &&
      event.keyboard.type == InputEvent::Keyboard::typeCharacter &&
      !_textInputEnabled) return;

    _internalFrame->AddEvent(event);
  }
}
