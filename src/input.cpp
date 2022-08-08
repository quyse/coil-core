#include "input.hpp"

namespace Coil::Input
{
  Event const* Frame::NextEvent()
  {
    while(_nextEvent < _events.size())
    {
      // process next event in state
      Event const& event = _events[_nextEvent++];

      switch(event.device)
      {
      case Event::deviceKeyboard:
        switch(event.keyboard.type)
        {
        case Event::Keyboard::typeKeyDown:
          // if key already pressed, skip
          if(_state.keyboard[event.keyboard.key])
            continue;
          _state.keyboard[event.keyboard.key] = 1;
          break;
        case Event::Keyboard::typeKeyUp:
          // if key already released, skip
          if(!_state.keyboard[event.keyboard.key])
            continue;
          _state.keyboard[event.keyboard.key] = 0;
          break;
        case Event::Keyboard::typeCharacter:
          break;
        }
        _ProcessKeyboardVirtualEvents(event);
        break;
      case Event::deviceMouse:
        switch(event.mouse.type)
        {
        case Event::Mouse::typeButtonDown:
          // if mouse button is already pressed, skip
          if(_state.mouseButtons[event.mouse.button])
            continue;
          _state.mouseButtons[event.mouse.button] = true;
          break;
        case Event::Mouse::typeButtonUp:
          // if mouse button is already released, skip
          if(!_state.mouseButtons[event.mouse.button])
            continue;
          _state.mouseButtons[event.mouse.button] = false;
          break;
        case Event::Mouse::typeRawMove:
          // there is no state for raw move
          break;
        case Event::Mouse::typeCursorMove:
          _state.cursorX = event.mouse.cursorX;
          _state.cursorY = event.mouse.cursorY;
          break;
        case Event::Mouse::typeDoubleClick:
          break;
        }
        break;
      case Event::deviceController:
        // TODO: controller state
        break;
      }

      // if we are here, event changes something
      return &event;
    }

    return nullptr;
  }

  State const& Frame::GetCurrentState() const
  {
    return _state;
  }

  void Frame::ForwardEvents()
  {
    while(NextEvent());
  }

  void Frame::Reset()
  {
    _events.clear();
    _nextEvent = 0;
  }

  void Frame::AddEvent(Event const& event)
  {
    _events.push_back(event);
  }

  void Frame::_ProcessKeyboardVirtualEvents(Event const& event)
  {
    switch(event.keyboard.type)
    {
    case Event::Keyboard::typeKeyDown:
    case Event::Keyboard::typeKeyUp:
      break;
    default:
      return;
    }

    Key virtualKey;
    bool newPressed;
    switch(event.keyboard.key)
    {
    case Key::ShiftL:
    case Key::ShiftR:
      virtualKey = Key::Shift;
      newPressed = _state.keyboard[Key::ShiftL] || _state.keyboard[Key::ShiftR];
      break;
    case Key::ControlL:
    case Key::ControlR:
      virtualKey = Key::Control;
      newPressed = _state.keyboard[Key::ControlL] || _state.keyboard[Key::ControlR];
      break;
    case Key::AltL:
    case Key::AltR:
      virtualKey = Key::Alt;
      newPressed = _state.keyboard[Key::AltL] || _state.keyboard[Key::AltR];
      break;
    default:
      return;
    }

    bool oldPressed = !!_state.keyboard[virtualKey];

    if(newPressed != oldPressed)
    {
      Event virtualEvent;
      virtualEvent.device = Event::deviceKeyboard;
      virtualEvent.keyboard.type = newPressed ? Event::Keyboard::typeKeyDown : Event::Keyboard::typeKeyUp;
      virtualEvent.keyboard.key = virtualKey;
      _events.push_back(virtualEvent);
    }
  }

  ControllerId Controller::GetId() const
  {
    return _id;
  }

  Controller::Controller(ControllerId id)
  : _id(id) {}

  void Manager::Update()
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
          Event event;
          event.device = Event::deviceKeyboard;
          event.keyboard.type = Event::Keyboard::typeKeyUp;
          event.keyboard.key = Key(i);
          _internalFrame->AddEvent(event);
        }
      for(size_t i = 0; i < sizeof(state.mouseButtons) / sizeof(state.mouseButtons[0]); ++i)
        if(state.mouseButtons[i])
        {
          Event event;
          event.device = Event::deviceMouse;
          event.mouse.type = Event::Mouse::typeButtonUp;
          event.mouse.button = Event::Mouse::Button(i);
          _internalFrame->AddEvent(event);
        }
    }
  }

  Frame const& Manager::GetCurrentFrame() const
  {
    return *_currentFrame;
  }

  void Manager::ReleaseButtonsOnUpdate()
  {
    _releaseButtonsOnUpdate = true;
  }

  void Manager::StartTextInput()
  {
    _textInputEnabled = true;
  }

  void Manager::StopTextInput()
  {
    _textInputEnabled = false;
  }

  bool Manager::IsTextInput() const
  {
    return _textInputEnabled;
  }

  void Manager::AddEvent(Event const& event)
  {
    // skip text input events if text input is disabled
    if(
      event.device == Event::deviceKeyboard &&
      event.keyboard.type == Event::Keyboard::typeCharacter &&
      !_textInputEnabled) return;

    _internalFrame->AddEvent(event);
  }
}
