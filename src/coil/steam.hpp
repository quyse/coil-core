#pragma once

#include "player_input.hpp"
#include <memory>
#include <unordered_map>
#include <vector>
#include <steam/steam_api.h>

namespace Coil
{
  class Steam
  {
  public:
    Steam();

    // return if initialized
    operator bool() const;
    // run every frame
    void Update();

  private:
    class Global;

    std::shared_ptr<Global> _global;
    bool _initialized = false;
  };

  class SteamInput
  {
  public:
    SteamInput();

    ISteamInput& operator()() const;

  private:
    class Global;

    std::shared_ptr<Global> _global;
  };

  class SteamPlayerInputManager final : public PlayerInputManager
  {
  public:
    // PlayerInputManager's methods
    ActionSetId GetActionSetId(char const* name) override;
    ButtonActionId GetButtonActionId(char const* name) override;
    AnalogActionId GetAnalogActionId(char const* name) override;
    void Update() override;
    void ActivateActionSet(ControllerId controllerId, ActionSetId actionSetId) override;
    PlayerInputButtonActionState GetButtonActionState(ControllerId controllerId, ButtonActionId actionId) const override;
    PlayerInputAnalogActionState GetAnalogActionState(ControllerId controllerId, AnalogActionId actionId) const override;

  private:
    SteamInput _steamInput;

    InputActionSetHandle_t GetActionSetHandle(ActionSetId actionSetId);

    struct ActionSet
    {
      InputActionSetHandle_t handle = 0;
      char const* name = nullptr;
    };
    std::vector<ActionSet> _actionSets;

    struct ButtonAction
    {
      InputDigitalActionHandle_t handle = 0;
      char const* name = nullptr;
    };
    std::vector<ButtonAction> _buttonActions;

    struct AnalogAction
    {
      InputAnalogActionHandle_t handle = 0;
      char const* name = nullptr;
    };
    std::vector<AnalogAction> _analogActions;

    struct ControllerState
    {
      std::vector<PlayerInputButtonActionState> buttonStates;
      std::vector<PlayerInputAnalogActionState> analogStates;
      bool active = false;
    };
    std::unordered_map<InputHandle_t, ControllerState> _controllers;

    std::vector<InputHandle_t> _tempInputHandles;
  };
}
