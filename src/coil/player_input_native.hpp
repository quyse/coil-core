#pragma once

#include "input.hpp"
#include "player_input.hpp"
#include "json.hpp"
#include <optional>
#include <unordered_map>

namespace Coil
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

  template <>
  struct JsonDecoder<NativePlayerInputMapping> : public JsonDecoderBase<NativePlayerInputMapping>
  {
    static NativePlayerInputMapping Decode(Json const& j);
  };

  class NativePlayerInputManager final : public PlayerInputManager
  {
  public:
    NativePlayerInputManager(InputManager& inputManager);

    void SetMapping(NativePlayerInputMapping const& mapping);

    // PlayerInputManager's methods
    ActionSetId GetActionSetId(char const* name) override;
    ButtonActionId GetButtonActionId(char const* name) override;
    AnalogActionId GetAnalogActionId(char const* name) override;
    void Update() override;
    void ActivateActionSet(ControllerId controllerId, ActionSetId actionSetId) override;
    PlayerInputButtonActionState GetButtonActionState(ControllerId controllerId, ButtonActionId actionId) const override;
    PlayerInputAnalogActionState GetAnalogActionState(ControllerId controllerId, AnalogActionId actionId) const override;

  private:
    InputManager& _inputManager;

    std::unordered_map<std::string, ActionSetId> _actionSetsIds;
    std::unordered_map<std::string, ButtonActionId> _buttonActionsIds;
    std::unordered_map<std::string, AnalogActionId> _analogActionsIds;

    struct ActionSet
    {
      std::unordered_map<InputKey, ButtonActionId> inputKeyToButtonActionId;
      std::unordered_map<InputMouseButton, ButtonActionId> mouseButtonToButtonActionId;
      std::optional<AnalogActionId> mouseMoveAnalogActionId;
      std::optional<AnalogActionId> mouseCursorAnalogActionId;
    };
    std::vector<ActionSet> _actionSets;

    struct Controller
    {
      PlayerInputButtonActionState& GetButtonActionState(ButtonActionId actionId);
      PlayerInputAnalogActionState& GetAnalogActionState(AnalogActionId actionId);

      // input controller id, none for keyboard+mouse
      std::optional<InputControllerId> inputControllerId;
      std::optional<ActionSetId> actionSetId;
      std::vector<PlayerInputButtonActionState> buttonActionsStates;
      std::vector<PlayerInputAnalogActionState> analogActionsStates;
    };
    std::unordered_map<ControllerId, Controller> _controllers;
    std::unordered_map<InputControllerId, ControllerId> _inputControllerIdToControllerId;
    ControllerId _nextControllerId = 0;

    // special controller ids
    enum class SpecialControllerId : ControllerId
    {
      // keyboard+mouse virtual controller is always 0
      keyboardMouse = 0
    };
  };
}
