#include "input_debug.hpp"
#include "unicode.hpp"

namespace Coil
{
  std::ostream& operator<<(std::ostream& stream, InputEvent const& event)
  {
    switch(event.device)
    {
    case InputEvent::deviceKeyboard:
      switch(event.keyboard.type)
      {
      case InputEvent::Keyboard::typeKeyDown:
        stream << "KEYDOWN " << event.keyboard.key;
        break;
      case InputEvent::Keyboard::typeKeyUp:
        stream << "KEYUP " << event.keyboard.key;
        break;
      case InputEvent::Keyboard::typeCharacter:
        {
          char32_t s[] = { event.keyboard.character, 0 };
          stream << "KEYPRESS ";
          for(Unicode::Iterator<char32_t, char, char32_t const*> i(s); *i; ++i)
            stream << *i;
        }
        break;
      }
      break;
    case InputEvent::deviceMouse:
      switch(event.mouse.type)
      {
        case InputEvent::Mouse::typeButtonDown:
          stream << "MOUSEDOWN";
          break;
        case InputEvent::Mouse::typeButtonUp:
          stream << "MOUSEUP";
          break;
        case InputEvent::Mouse::typeRawMove:
          stream << "MOUSERAWMOVE";
          break;
        case InputEvent::Mouse::typeCursorMove:
          stream << "MOUSECURSORMOVE";
          break;
        case InputEvent::Mouse::typeDoubleClick:
          stream << "MOUSEDOUBLECLICK";
          break;
      }
      stream << ' ';
      switch(event.mouse.type)
      {
        case InputEvent::Mouse::typeButtonDown:
        case InputEvent::Mouse::typeButtonUp:
        case InputEvent::Mouse::typeDoubleClick:
          switch(event.mouse.button)
          {
          case InputEvent::Mouse::buttonLeft:
            stream << "LEFT";
            break;
          case InputEvent::Mouse::buttonRight:
            stream << "RIGHT";
            break;
          case InputEvent::Mouse::buttonMiddle:
            stream << "MIDDLE";
            break;
          }
          break;
        case InputEvent::Mouse::typeRawMove:
          stream << event.mouse.rawMoveX << ' ' << event.mouse.rawMoveY << ' ' << event.mouse.rawMoveZ;
          break;
        case InputEvent::Mouse::typeCursorMove:
          stream << event.mouse.cursorX << ' ' << event.mouse.cursorY << ' ' << event.mouse.cursorZ;
          break;
      }
      break;
    case InputEvent::deviceController:
      switch(event.controller.type)
      {
      case InputEvent::Controller::typeDeviceAdded:
        stream << "CONTROLLERADDED";
        break;
      case InputEvent::Controller::typeDeviceRemoved:
        stream << "CONTROLLERREMOVED";
        break;
      case InputEvent::Controller::typeButtonDown:
        stream << "CONTROLLERBUTTONDOWN";
        break;
      case InputEvent::Controller::typeButtonUp:
        stream << "CONTROLLERBUTTONUP";
        break;
      case InputEvent::Controller::typeAxisMotion:
        stream << "CONTROLLERAXISMOTION";
        break;
      }
      stream << " D" << event.controller.device;
      switch(event.controller.type)
      {
      case InputEvent::Controller::typeDeviceAdded:
      case InputEvent::Controller::typeDeviceRemoved:
        break;
      case InputEvent::Controller::typeButtonDown:
      case InputEvent::Controller::typeButtonUp:
        stream << ' ';
        switch(event.controller.button)
        {
        case InputEvent::Controller::buttonA:             stream << "A"; break;
        case InputEvent::Controller::buttonB:             stream << "B"; break;
        case InputEvent::Controller::buttonX:             stream << "X"; break;
        case InputEvent::Controller::buttonY:             stream << "Y"; break;
        case InputEvent::Controller::buttonBack:          stream << "BACK"; break;
        case InputEvent::Controller::buttonGuide:         stream << "GUIDE"; break;
        case InputEvent::Controller::buttonStart:         stream << "START"; break;
        case InputEvent::Controller::buttonLeftStick:     stream << "LEFTSTICK"; break;
        case InputEvent::Controller::buttonRightStick:    stream << "RIGHTSTICK"; break;
        case InputEvent::Controller::buttonLeftShoulder:  stream << "LEFTSHOULDER"; break;
        case InputEvent::Controller::buttonRightShoulder: stream << "RIGHTSHOULDER"; break;
        case InputEvent::Controller::buttonDPadUp:        stream << "DPADUP"; break;
        case InputEvent::Controller::buttonDPadDown:      stream << "DPADDOWN"; break;
        case InputEvent::Controller::buttonDPadLeft:      stream << "DPADLEFT"; break;
        case InputEvent::Controller::buttonDPadRight:     stream << "DPADRIGHT"; break;
        }
        break;
      case InputEvent::Controller::typeAxisMotion:
        stream << ' ';
        switch(event.controller.axis)
        {
        case InputEvent::Controller::axisLeftX:        stream << "AXISLEFTX"; break;
        case InputEvent::Controller::axisLeftY:        stream << "AXISLEFTY"; break;
        case InputEvent::Controller::axisRightX:       stream << "AXISRIGHTX"; break;
        case InputEvent::Controller::axisRightY:       stream << "AXISRIGHTY"; break;
        case InputEvent::Controller::axisTriggerLeft:  stream << "AXISTRIGGERLEFT"; break;
        case InputEvent::Controller::axisTriggerRight: stream << "AXISTRIGGERRIGHT"; break;
        }
        stream << ' ' << event.controller.axisValue;
        break;
      }
    }

    return stream;
  }
}
