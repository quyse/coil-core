#include "input_debug.hpp"
#include "math_debug.hpp"
#include "unicode.hpp"

namespace Coil
{
  std::ostream& operator<<(std::ostream& stream, InputEvent const& event)
  {
    std::visit([&](auto const& event)
    {
      using E = std::decay_t<decltype(event)>;
      if constexpr(std::is_same_v<E, InputKeyboardEvent>)
      {
        std::visit([&](auto const& event)
        {
          using E = std::decay_t<decltype(event)>;
          if constexpr(std::is_same_v<E, InputKeyboardKeyEvent>)
          {
            stream << (event.isPressed ? "KEYDOWN " : "KEYUP ") << (uint32_t)event.key;
          }
          if constexpr(std::is_same_v<E, InputKeyboardCharacterEvent>)
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
      if constexpr(std::is_same_v<E, InputMouseEvent>)
      {
        std::visit([&](auto const& event)
        {
          using E = std::decay_t<decltype(event)>;
          if constexpr(std::is_same_v<E, InputMouseButtonEvent>)
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
          if constexpr(std::is_same_v<E, InputMouseRawMoveEvent>)
          {
            stream << "MOUSERAWMOVE " << event.rawMove;
          }
          if constexpr(std::is_same_v<E, InputMouseCursorMoveEvent>)
          {
            stream << "MOUSECURSORMOVE " << event.cursor << ' ' << event.wheel;
          }
        }, event);
      }
      if constexpr(std::is_same_v<E, InputControllerEvent>)
      {
        stream << "CONTROLLER " << event.controllerId << ' ';
        std::visit([&](auto const& event)
        {
          using E = std::decay_t<decltype(event)>;
          if constexpr(std::is_same_v<E, InputControllerEvent::ControllerEvent>)
          {
            stream << (event.isAdded ? "ADDED" : "REMOVED");
          }
          if constexpr(std::is_same_v<E, InputControllerEvent::ButtonEvent>)
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
          if constexpr(std::is_same_v<E, InputControllerEvent::AxisMotionEvent>)
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
