#include "player_input_native.hpp"
#include <concepts>

namespace Coil
{
  NativePlayerInputManager::NativePlayerInputManager(InputManager& inputManager)
  : _inputManager(inputManager)
  {
    // add controller 0 for keyboard+mouse
    _controllers[_nextControllerId++] = {};
  }

  void NativePlayerInputManager::SetMapping(NativePlayerInputMapping const& mapping)
  {
    for(auto const& [actionSetName, mappingActionSet] : mapping.actionSets)
    {
      auto& actionSet = _actionSets[GetActionSetId(actionSetName.c_str())];

      // keyboard
      for(auto const& [inputKey, buttonActionName] : mappingActionSet.keyboard)
      {
        actionSet.inputKeyToButtonActionId[inputKey] = GetButtonActionId(buttonActionName.c_str());
      }
      // mouse
      for(auto const& [inputMouseButton, buttonActionName] : mappingActionSet.mouse.buttons)
      {
        actionSet.mouseButtonToButtonActionId[inputMouseButton] = GetButtonActionId(buttonActionName.c_str());
      }
      if(mappingActionSet.mouse.move.has_value())
      {
        actionSet.mouseMoveAnalogActionId = GetAnalogActionId(mappingActionSet.mouse.move.value().c_str());
      }
      if(mappingActionSet.mouse.cursor.has_value())
      {
        actionSet.mouseCursorAnalogActionId = GetAnalogActionId(mappingActionSet.mouse.cursor.value().c_str());
      }
    }
  }

  PlayerInputManager::ActionSetId NativePlayerInputManager::GetActionSetId(char const* name)
  {
    auto actionSetId = _actionSetsIds.insert({ name, _actionSetsIds.size() }).first->second;
    if(actionSetId >= _actionSets.size())
    {
      _actionSets.resize(actionSetId + 1);
    }
    return actionSetId;
  }

  PlayerInputManager::ButtonActionId NativePlayerInputManager::GetButtonActionId(char const* name)
  {
    return _buttonActionsIds.insert({ name, _buttonActionsIds.size() }).first->second;
  }

  PlayerInputManager::AnalogActionId NativePlayerInputManager::GetAnalogActionId(char const* name)
  {
    return _analogActionsIds.insert({ name, _analogActionsIds.size() }).first->second;
  }

  void NativePlayerInputManager::Update()
  {
    // reset states
    for(auto& [controllerId, controller] : _controllers)
    {
      // reset "just changed" button states
      for(size_t i = 0; i < controller.buttonActionsStates.size(); ++i)
        controller.buttonActionsStates[i].isJustChanged = false;
      // reset relative analog states
      for(size_t i = 0; i < controller.analogActionsStates.size(); ++i)
      {
        auto& analogActionState = controller.analogActionsStates[i];
        if(!analogActionState.absolute)
        {
          analogActionState.x = 0;
          analogActionState.y = 0;
        }
      }
    }

    // process events
    auto& inputFrame = _inputManager.GetCurrentFrame();
    while(auto const* event = inputFrame.NextEvent())
    {
      std::visit([&]<typename E1>(E1 const& event)
      {
        if constexpr(std::same_as<E1, InputKeyboardEvent>)
        {
          std::visit([&]<typename E2>(E2 const& event)
          {
            if constexpr(std::same_as<E2, InputKeyboardKeyEvent>)
            {
              auto& controller = _controllers[(ControllerId)SpecialControllerId::keyboardMouse];
              if(controller.actionSetId.has_value())
              {
                ActionSet const& actionSet = _actionSets[controller.actionSetId.value()];

                auto i = actionSet.inputKeyToButtonActionId.find(event.key);
                if(i != actionSet.inputKeyToButtonActionId.end())
                {
                  auto& buttonActionState = controller.GetButtonActionState(i->second);
                  buttonActionState.isJustChanged = buttonActionState.isPressed != event.isPressed;
                  buttonActionState.isPressed = event.isPressed;
                }
              }
            }
          }, event);
        }
        if constexpr(std::same_as<E1, InputMouseEvent>)
        {
          std::visit([&]<typename E2>(E2 const& event)
          {
            if constexpr(std::same_as<E2, InputMouseButtonEvent>)
            {
              auto& controller = _controllers[(ControllerId)SpecialControllerId::keyboardMouse];
              if(controller.actionSetId.has_value())
              {
                ActionSet const& actionSet = _actionSets[controller.actionSetId.value()];

                auto i = actionSet.mouseButtonToButtonActionId.find(event.button);
                if(i != actionSet.mouseButtonToButtonActionId.end())
                {
                  auto& buttonActionState = controller.GetButtonActionState(i->second);
                  buttonActionState.isJustChanged = buttonActionState.isPressed != event.isPressed;
                  buttonActionState.isPressed = event.isPressed;
                }
              }
            }
            if constexpr(std::same_as<E2, InputMouseRawMoveEvent>)
            {
              auto& controller = _controllers[(ControllerId)SpecialControllerId::keyboardMouse];
              if(controller.actionSetId.has_value())
              {
                ActionSet const& actionSet = _actionSets[controller.actionSetId.value()];

                if(actionSet.mouseMoveAnalogActionId.has_value())
                {
                  auto& analogActionState = controller.GetAnalogActionState(actionSet.mouseMoveAnalogActionId.value());
                  analogActionState.x = event.rawMove.x();
                  analogActionState.y = event.rawMove.y();
                  analogActionState.absolute = false;
                }
              }
            }
            if constexpr(std::same_as<E2, InputMouseCursorMoveEvent>)
            {
              auto& controller = _controllers[(ControllerId)SpecialControllerId::keyboardMouse];
              if(controller.actionSetId.has_value())
              {
                ActionSet const& actionSet = _actionSets[controller.actionSetId.value()];

                if(actionSet.mouseCursorAnalogActionId.has_value())
                {
                  auto& analogActionState = controller.GetAnalogActionState(actionSet.mouseCursorAnalogActionId.value());
                  analogActionState.x = event.cursor.x();
                  analogActionState.y = event.cursor.y();
                  analogActionState.absolute = true;
                }
              }
            }
          }, event);
        }
        if constexpr(std::same_as<E1, InputControllerEvent>)
        {
          InputControllerId const inputControllerId = event.controllerId;
          std::visit([&]<typename E2>(E2 const& event)
          {
            if constexpr(std::same_as<E2, InputControllerEvent::ControllerEvent>)
            {
              if(event.isAdded)
              {
                ControllerId const controllerId = _nextControllerId++;
                _controllers[controllerId] =
                {
                  .inputControllerId = inputControllerId,
                };
                _inputControllerIdToControllerId[inputControllerId] = controllerId;
              }
              else
              {
                auto i = _inputControllerIdToControllerId.find(inputControllerId);
                _controllers.erase(i->second);
                _inputControllerIdToControllerId.erase(i);
              }
            }
            // TODO: buttons, triggers, sticks
          }, event.event);
        }
      }, *event);
    }

    // update controllers ids
    _controllersIds.clear();
    for(auto const& i : _controllers)
      _controllersIds.push_back(i.first);
  }

  void NativePlayerInputManager::ActivateActionSet(ControllerId controllerId, ActionSetId actionSetId)
  {
    auto i = _controllers.find(controllerId);
    if(i == _controllers.end()) return;
    i->second.actionSetId = actionSetId;
  }

  PlayerInputButtonActionState NativePlayerInputManager::GetButtonActionState(ControllerId controllerId, ButtonActionId actionId) const
  {
    auto i = _controllers.find(controllerId);
    if(i == _controllers.end()) return {};
    auto const& buttonActionsStates = i->second.buttonActionsStates;
    if(actionId >= buttonActionsStates.size()) return {};
    return buttonActionsStates[actionId];
  }

  PlayerInputAnalogActionState NativePlayerInputManager::GetAnalogActionState(ControllerId controllerId, AnalogActionId actionId) const
  {
    auto i = _controllers.find(controllerId);
    if(i == _controllers.end()) return {};
    auto const& analogActionsStates = i->second.analogActionsStates;
    if(actionId >= analogActionsStates.size()) return {};
    return analogActionsStates[actionId];
  }

  PlayerInputButtonActionState& NativePlayerInputManager::Controller::GetButtonActionState(ButtonActionId actionId)
  {
    if(actionId >= buttonActionsStates.size())
    {
      buttonActionsStates.resize(actionId + 1);
    }
    return buttonActionsStates[actionId];
  }

  PlayerInputAnalogActionState& NativePlayerInputManager::Controller::GetAnalogActionState(AnalogActionId actionId)
  {
    if(actionId >= analogActionsStates.size())
    {
      analogActionsStates.resize(actionId + 1);
    }
    return analogActionsStates[actionId];
  }

  NativePlayerInputMapping JsonDecoder<NativePlayerInputMapping>::Decode(json const& j)
  {
    return
    {
      .actionSets = JsonDecodeField<std::unordered_map<std::string, NativePlayerInputMapping::ActionSet>>(j, "actionSets", {}),
    };
  }

  template <>
  struct JsonDecoder<NativePlayerInputMapping::ActionSet> : public JsonDecoderBase<NativePlayerInputMapping::ActionSet>
  {
    static NativePlayerInputMapping::ActionSet Decode(json const& j)
    {
      return
      {
        .keyboard = JsonDecodeField<std::unordered_map<InputKey, std::string>>(j, "keyboard", {}),
        .mouse = JsonDecodeField<NativePlayerInputMapping::ActionSet::Mouse>(j, "mouse", {}),
      };
    }
  };

  template <>
  struct JsonDecoder<NativePlayerInputMapping::ActionSet::Mouse> : public JsonDecoderBase<NativePlayerInputMapping::ActionSet::Mouse>
  {
    static NativePlayerInputMapping::ActionSet::Mouse Decode(json const& j)
    {
      return
      {
        .buttons = JsonDecodeField<std::unordered_map<InputMouseButton, std::string>>(j, "buttons", {}),
        .move = JsonDecodeField<std::optional<std::string>>(j, "move", {}),
        .cursor = JsonDecodeField<std::optional<std::string>>(j, "cursor", {}),
      };
    }
  };

  template <>
  struct JsonDecoder<std::unordered_map<InputKey, std::string>> : public JsonDecoderBase<std::unordered_map<InputKey, std::string>>
  {
    static std::unordered_map<InputKey, std::string> Decode(json const& j)
    {
      std::unordered_map<InputKey, std::string> r;
      for(auto const& [k, v] : j.items())
      {
        r.insert(
        {
          FromString<InputKey>(JsonDecode<std::string>(k)),
          JsonDecode<std::string>(v),
        });
      }
      return r;
    }
  };

  template <>
  struct JsonDecoder<std::unordered_map<InputMouseButton, std::string>> : public JsonDecoderBase<std::unordered_map<InputMouseButton, std::string>>
  {
    static std::unordered_map<InputMouseButton, std::string> Decode(json const& j)
    {
      std::unordered_map<InputMouseButton, std::string> r;
      for(auto const& [k, v] : j.items())
      {
        r.insert(
        {
          FromString<InputMouseButton>(JsonDecode<std::string>(k)),
          JsonDecode<std::string>(v),
        });
      }
      return r;
    }
  };
}
