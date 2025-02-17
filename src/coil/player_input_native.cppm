module;

#include <optional>
#include <string>
#include <unordered_map>
#include <variant>

export module coil.core.player_input.native;

import coil.core.base.events.map;
import coil.core.base.events;
import coil.core.base.signals;
import coil.core.base;
import coil.core.input;
import coil.core.json;
import coil.core.math;
import coil.core.player_input;

export namespace Coil
{
  struct NativePlayerInputMapping
  {
    struct ActionSet
    {
      std::unordered_map<InputKey, std::string> keyboard;
      struct Mouse
      {
        std::unordered_map<InputMouseButton, std::string> buttons;
        std::optional<std::string> move;
        std::optional<std::string> cursor;
      };
      Mouse mouse;
    };

    std::unordered_map<std::string, ActionSet> actionSets;
  };

  class NativePlayerInputManager final : public PlayerInputManager
  {
  public:
    NativePlayerInputManager(InputManager& inputManager)
    : inputManager_(inputManager)
    {
      // initialize ids
      pControllersIds_ = pControllers_->Map([](ControllerId controllerId, Controller const&) -> std::tuple<>
      {
        return {};
      });
      // add controller 0 for keyboard+mouse
      pControllers_->Set(nextControllerId_++, {{}});
    }

    void SetMapping(NativePlayerInputMapping const& mapping)
    {
      for(auto const& [actionSetName, mappingActionSet] : mapping.actionSets)
      {
        auto& actionSet = actionSets_[GetActionSetId(actionSetName.c_str())];

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

    // PlayerInputManager's methods

    ActionSetId GetActionSetId(std::string_view name) override
    {
      auto actionSetId = actionSetsIds_.insert({ std::string{name}, actionSetsIds_.size() }).first->second;
      if(actionSetId >= actionSets_.size())
      {
        actionSets_.resize(actionSetId + 1);
      }
      return actionSetId;
    }

    ButtonActionId GetButtonActionId(std::string_view name) override
    {
      return buttonActionsIds_.insert({ std::string{name}, buttonActionsIds_.size() }).first->second;
    }

    AnalogActionId GetAnalogActionId(std::string_view name) override
    {
      return analogActionsIds_.insert({ std::string{name}, analogActionsIds_.size() }).first->second;
    }

    void Update() override
    {
      // process events
      auto& inputFrame = inputManager_.GetCurrentFrame();
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
                pControllers_->With((ControllerId)SpecialControllerId::keyboardMouse, [&](Controller& controller)
                {
                  if(controller.actionSetId.has_value())
                  {
                    ActionSet const& actionSet = actionSets_[controller.actionSetId.value()];

                    auto i = actionSet.inputKeyToButtonActionId.find(event.key);
                    if(i != actionSet.inputKeyToButtonActionId.end())
                    {
                      auto const& buttonActionState = controller.GetButtonAction(i->second);
                      buttonActionState.sigIsPressed.SetIfDiffers(event.isPressed);
                    }
                  }
                });
              }
            }, event);
          }
          if constexpr(std::same_as<E1, InputMouseEvent>)
          {
            std::visit([&]<typename E2>(E2 const& event)
            {
              if constexpr(std::same_as<E2, InputMouseButtonEvent>)
              {
                pControllers_->With((ControllerId)SpecialControllerId::keyboardMouse, [&](Controller& controller)
                {
                  if(controller.actionSetId.has_value())
                  {
                    ActionSet const& actionSet = actionSets_[controller.actionSetId.value()];

                    auto i = actionSet.mouseButtonToButtonActionId.find(event.button);
                    if(i != actionSet.mouseButtonToButtonActionId.end())
                    {
                      auto const& buttonActionState = controller.GetButtonAction(i->second);
                      buttonActionState.sigIsPressed.SetIfDiffers(event.isPressed);
                    }
                  }
                });
              }
              if constexpr(std::same_as<E2, InputMouseRawMoveEvent>)
              {
                pControllers_->With((ControllerId)SpecialControllerId::keyboardMouse, [&](Controller& controller)
                {
                  if(controller.actionSetId.has_value())
                  {
                    ActionSet const& actionSet = actionSets_[controller.actionSetId.value()];

                    if(actionSet.mouseMoveAnalogActionId.has_value())
                    {
                      auto const& analogAction = controller.GetAnalogAction(actionSet.mouseMoveAnalogActionId.value(), false);
                      std::visit([&](auto const& relativeOrAbsoluteValue)
                      {
                        if constexpr(std::same_as<decltype(relativeOrAbsoluteValue), EventPtr<vec2> const&>)
                        {
                          relativeOrAbsoluteValue.Notify(vec2(event.rawMove.x(), event.rawMove.y()));
                        }
                      }, analogAction.relativeOrAbsoluteValue);
                    }
                  }
                });
              }
              if constexpr(std::same_as<E2, InputMouseCursorMoveEvent>)
              {
                pControllers_->With((ControllerId)SpecialControllerId::keyboardMouse, [&](Controller& controller)
                {
                  if(controller.actionSetId.has_value())
                  {
                    ActionSet const& actionSet = actionSets_[controller.actionSetId.value()];

                    if(actionSet.mouseCursorAnalogActionId.has_value())
                    {
                      auto const& analogAction = controller.GetAnalogAction(actionSet.mouseCursorAnalogActionId.value(), true);
                      std::visit([&](auto const& relativeOrAbsoluteValue)
                      {
                        if constexpr(std::same_as<decltype(relativeOrAbsoluteValue), SignalVarPtr<vec2> const&>)
                        {
                          relativeOrAbsoluteValue.SetIfDiffers(vec2(event.cursor.x(), event.cursor.y()));
                        }
                      }, analogAction.relativeOrAbsoluteValue);
                    }
                  }
                });
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
                  ControllerId const controllerId = nextControllerId_++;
                  pControllers_->Set(controllerId,
                  {{
                    .inputControllerId = inputControllerId,
                  }});
                  inputControllerIdToControllerId_[inputControllerId] = controllerId;
                }
                else
                {
                  auto i = inputControllerIdToControllerId_.find(inputControllerId);
                  pControllers_->Set(i->second, {});
                  inputControllerIdToControllerId_.erase(i);
                }
              }
              // TODO: buttons, triggers, sticks
            }, event.event);
          }
        }, *event);
      }
    }

    void ActivateActionSet(ControllerId controllerId, ActionSetId actionSetId) override
    {
      pControllers_->With(controllerId, [&](Controller& controller)
      {
        controller.actionSetId = actionSetId;
      });
    }

    PlayerInputButtonAction GetButtonAction(ControllerId controllerId, ActionSetId actionSetId, ButtonActionId actionId) const override
    {
      Controller* pController = pControllers_->Get(controllerId);
      if(!pController) return {};
      auto const& action = pController->GetButtonAction(actionId);
      return
      {
        .sigIsPressed = action.sigIsPressed,
      };
    }

    PlayerInputAnalogAction GetAnalogAction(ControllerId controllerId, ActionSetId actionSetId, AnalogActionId actionId) const override
    {
      Controller* pController = pControllers_->Get(controllerId);
      if(!pController) return {};

      // determine if it's absolute
      // currently the only relative action is raw mouse move
      auto const& optMouseMoveAnalogActionId = actionSets_[actionSetId].mouseMoveAnalogActionId;
      bool absolute = !(optMouseMoveAnalogActionId.has_value() && optMouseMoveAnalogActionId.value() == actionId);

      auto const& action = pController->GetAnalogAction(actionId, absolute);
      return std::visit([](auto const& relativeOrAbsoluteValue) -> PlayerInputAnalogAction
      {
        return
        {
          .relativeOrAbsoluteValue = relativeOrAbsoluteValue,
        };
      }, action.relativeOrAbsoluteValue);
    }

  private:
    InputManager& inputManager_;

    std::unordered_map<std::string, ActionSetId> actionSetsIds_;
    std::unordered_map<std::string, ButtonActionId> buttonActionsIds_;
    std::unordered_map<std::string, AnalogActionId> analogActionsIds_;

    struct ActionSet
    {
      std::unordered_map<InputKey, ButtonActionId> inputKeyToButtonActionId;
      std::unordered_map<InputMouseButton, ButtonActionId> mouseButtonToButtonActionId;
      std::optional<AnalogActionId> mouseMoveAnalogActionId;
      std::optional<AnalogActionId> mouseCursorAnalogActionId;
    };
    std::vector<ActionSet> actionSets_;

    struct Controller
    {
      struct ButtonAction
      {
        SignalVarPtr<bool> sigIsPressed;
      };

      struct AnalogAction
      {
        std::variant<EventPtr<vec2>, SignalVarPtr<vec2>> relativeOrAbsoluteValue;
      };

      ButtonAction const& GetButtonAction(ButtonActionId actionId)
      {
        auto i = buttonActions.find(actionId);
        if(i != buttonActions.end()) return i->second;
        return buttonActions.insert({actionId,
        {
          .sigIsPressed = MakeVariableSignal<bool>(false),
        }}).first->second;
      }

      AnalogAction const& GetAnalogAction(AnalogActionId actionId, bool absolute)
      {
        auto i = analogActions.find(actionId);
        if(i != analogActions.end()) return i->second;
        return analogActions.insert(
        {
          actionId,
          absolute
            ? AnalogAction
            {
              .relativeOrAbsoluteValue = MakeVariableSignal<vec2>({}),
            }
            : AnalogAction
            {
              .relativeOrAbsoluteValue = MakeEvent<vec2>(),
            }
        }).first->second;
      }

      // input controller id, none for keyboard+mouse
      std::optional<InputControllerId> inputControllerId;
      std::optional<ActionSetId> actionSetId;
      std::unordered_map<ButtonActionId, ButtonAction> buttonActions;
      std::unordered_map<AnalogActionId, AnalogAction> analogActions;
    };
    SyncMapPtr<ControllerId, Controller> pControllers_ = MakeSyncMap<ControllerId, Controller>();
    std::unordered_map<InputControllerId, ControllerId> inputControllerIdToControllerId_;
    ControllerId nextControllerId_ = 0;

    // special controller ids
    enum class SpecialControllerId : ControllerId
    {
      // keyboard+mouse virtual controller is always 0
      keyboardMouse = 0
    };
  };

  template <>
  struct JsonDecoder<NativePlayerInputMapping> : public JsonDecoderBase<NativePlayerInputMapping>
  {
    static NativePlayerInputMapping Decode(Json const& j)
    {
      return
      {
        .actionSets = JsonDecodeField<std::unordered_map<std::string, NativePlayerInputMapping::ActionSet>>(j, "actionSets", {}),
      };
    }
  };

  template <>
  struct JsonDecoder<NativePlayerInputMapping::ActionSet> : public JsonDecoderBase<NativePlayerInputMapping::ActionSet>
  {
    static NativePlayerInputMapping::ActionSet Decode(Json const& j)
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
    static NativePlayerInputMapping::ActionSet::Mouse Decode(Json const& j)
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
    static std::unordered_map<InputKey, std::string> Decode(Json const& j)
    {
      std::unordered_map<InputKey, std::string> r;
      for(auto const& item : j.items())
      {
        r.insert(
        {
          FromString<InputKey>(JsonDecode<std::string>(item.key())),
          JsonDecode<std::string>(item.value()),
        });
      }
      return r;
    }
  };

  template <>
  struct JsonDecoder<std::unordered_map<InputMouseButton, std::string>> : public JsonDecoderBase<std::unordered_map<InputMouseButton, std::string>>
  {
    static std::unordered_map<InputMouseButton, std::string> Decode(Json const& j)
    {
      std::unordered_map<InputMouseButton, std::string> r;
      for(auto const& item : j.items())
      {
        r.insert(
        {
          FromString<InputMouseButton>(JsonDecode<std::string>(item.key())),
          JsonDecode<std::string>(item.value()),
        });
      }
      return r;
    }
  };
}
