module;

#include <concepts>
#include <ostream>

export module coil.core.input.debug;

import coil.core.input;
import coil.core.math.debug;
import coil.core.unicode;

export namespace Coil
{
  std::ostream& operator<<(std::ostream& stream, InputEvent const& event)
  {
    std::visit([&]<typename E1>(E1 const& event)
    {
      if constexpr(std::same_as<E1, InputKeyboardEvent>)
      {
        std::visit([&]<typename E2>(E2 const& event)
        {
          if constexpr(std::same_as<E2, InputKeyboardKeyEvent>)
          {
            stream << (event.isPressed ? "KEYDOWN " : "KEYUP ") << (uint32_t)event.key;
          }
          if constexpr(std::same_as<E2, InputKeyboardCharacterEvent>)
          {
            char32_t s[] = { event.character, 0 };
            stream << "KEYPRESS ";
            char r[8];
            char* p = r;
            for(Unicode::Iterator<char32_t, char, char32_t const*> i(s); *i; ++i)
              *p++ = *i;
            *p = 0;
            stream << r;
          }
        }, event);
      }
      if constexpr(std::same_as<E1, InputMouseEvent>)
      {
        std::visit([&]<typename E2>(E2 const& event)
        {
          if constexpr(std::same_as<E2, InputMouseButtonEvent>)
          {
            stream << (event.isPressed ? "MOUSEDOWN " : "MOUSEUP ");
            switch(event.button)
            {
            case InputMouseButton::Left:
              stream << "LEFT";
              break;
            case InputMouseButton::Right:
              stream << "RIGHT";
              break;
            case InputMouseButton::Middle:
              stream << "MIDDLE";
              break;
            }
          }
          if constexpr(std::same_as<E2, InputMouseRawMoveEvent>)
          {
            stream << "MOUSERAWMOVE " << event.rawMove;
          }
          if constexpr(std::same_as<E2, InputMouseCursorMoveEvent>)
          {
            stream << "MOUSECURSORMOVE " << event.cursor << ' ' << event.wheel;
          }
        }, event);
      }
      if constexpr(std::same_as<E1, InputControllerEvent>)
      {
        stream << "CONTROLLER " << event.controllerId << ' ';
        std::visit([&]<typename E2>(E2 const& event)
        {
          if constexpr(std::same_as<E2, InputControllerEvent::ControllerEvent>)
          {
            stream << (event.isAdded ? "ADDED" : "REMOVED");
          }
          if constexpr(std::same_as<E2, InputControllerEvent::ButtonEvent>)
          {
            stream << (event.isPressed ? "DOWN " : "UP ");
            switch(event.button)
            {
            case InputControllerButton::A:             stream << "A"; break;
            case InputControllerButton::B:             stream << "B"; break;
            case InputControllerButton::X:             stream << "X"; break;
            case InputControllerButton::Y:             stream << "Y"; break;
            case InputControllerButton::Back:          stream << "BACK"; break;
            case InputControllerButton::Guide:         stream << "GUIDE"; break;
            case InputControllerButton::Start:         stream << "START"; break;
            case InputControllerButton::LeftStick:     stream << "LEFTSTICK"; break;
            case InputControllerButton::RightStick:    stream << "RIGHTSTICK"; break;
            case InputControllerButton::LeftShoulder:  stream << "LEFTSHOULDER"; break;
            case InputControllerButton::RightShoulder: stream << "RIGHTSHOULDER"; break;
            case InputControllerButton::DPadUp:        stream << "DPADUP"; break;
            case InputControllerButton::DPadDown:      stream << "DPADDOWN"; break;
            case InputControllerButton::DPadLeft:      stream << "DPADLEFT"; break;
            case InputControllerButton::DPadRight:     stream << "DPADRIGHT"; break;
            }
          }
          if constexpr(std::same_as<E2, InputControllerEvent::AxisMotionEvent>)
          {
            switch(event.axis)
            {
            case InputControllerAxis::LeftX:        stream << "AXISLEFTX"; break;
            case InputControllerAxis::LeftY:        stream << "AXISLEFTY"; break;
            case InputControllerAxis::RightX:       stream << "AXISRIGHTX"; break;
            case InputControllerAxis::RightY:       stream << "AXISRIGHTY"; break;
            case InputControllerAxis::TriggerLeft:  stream << "AXISTRIGGERLEFT"; break;
            case InputControllerAxis::TriggerRight: stream << "AXISTRIGGERRIGHT"; break;
            }
            stream << ' ' << event.axisValue;
          }
        }, event.event);
      }
    }, event);

    return stream;
  }
}
