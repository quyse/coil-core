module;

#include <steam/steam_api.h>
#include <unordered_map>

export module coil.core.steam;

import coil.core.player_input;

export namespace Coil
{
  class Steam
  {
  private:
    struct Global
    {
      Global()
      {
        _initialized = SteamAPI_Init();
      }

      ~Global()
      {
        if(_initialized)
        {
          SteamAPI_Shutdown();
        }
      }

      bool _initialized = false;
    };

  public:
    Steam()
    {
      static std::shared_ptr<Global> global = std::make_shared<Global>();
      _global = global;
      _initialized = global->_initialized;
    }

    // return if initialized
    operator bool() const
    {
      return _initialized;
    }

    // run every frame
    void Update()
    {
      SteamAPI_RunCallbacks();
    }

  private:
    std::shared_ptr<Global> _global;
    bool _initialized = false;
  };

  class SteamInput
  {
  private:
    struct Global
    {
      Global()
      {
        _steamInput->Init(false);
      }

      ~Global()
      {
        if(_steamInput)
        {
          _steamInput->Shutdown();
        }
      }

      Steam _steam;
      ISteamInput* const _steamInput = ::SteamInput();
    };

  public:
    SteamInput()
    {
      static std::shared_ptr<Global> global = std::make_shared<Global>();
      _global = global;
    }

    ISteamInput& operator()() const
    {
      return *_global->_steamInput;
    }

  private:
    std::shared_ptr<Global> _global;
  };

  class SteamPlayerInputManager final : public PlayerInputManager
  {
  public:
    // PlayerInputManager's methods

    ActionSetId GetActionSetId(char const* name) override
    {
      ActionSetId actionSetId = _actionSets.size();
      _actionSets.push_back({
        .name = name,
      });
      return actionSetId;
    }

    ButtonActionId GetButtonActionId(char const* name) override
    {
      ButtonActionId actionId = _buttonActions.size();
      _buttonActions.push_back({
        .name = name,
      });
      return actionId;
    }

    AnalogActionId GetAnalogActionId(char const* name) override
    {
      AnalogActionId actionId = _analogActions.size();
      _analogActions.push_back({
        .name = name,
      });
      return actionId;
    }

    void Update() override
    {
      // get active controllers
      _tempInputHandles.resize(STEAM_INPUT_MAX_COUNT);
      _tempInputHandles.resize(_steamInput().GetConnectedControllers(_tempInputHandles.data()));

      // remove non-active controllers
      for(auto& [inputHandle, controllerState] : _controllers)
        controllerState.active = false;
      for(size_t i = 0; i < _tempInputHandles.size(); ++i)
        _controllers[_tempInputHandles[i]].active = true;
      for(auto i = _controllers.begin(); i != _controllers.end(); )
      {
        if(i->second.active) ++i;
        else
        {
          auto j = i;
          ++i;
          _controllers.erase(j);
        }
      }

      // update states
      _controllersIds.clear();
      for(auto& [inputHandle, controllerState] : _controllers)
      {
        _controllersIds.push_back(inputHandle);
        controllerState.buttonStates.resize(_buttonActions.size());
        for(size_t i = 0; i < _buttonActions.size(); ++i)
        {
          auto& buttonAction = _buttonActions[i];
          if(!buttonAction.handle)
          {
            buttonAction.handle = _steamInput().GetDigitalActionHandle(buttonAction.name);
          }

          auto& buttonState = controllerState.buttonStates[i];
          bool isPressed = _steamInput().GetDigitalActionData(inputHandle, _buttonActions[i].handle).bState;
          buttonState.isJustChanged = buttonState.isPressed != isPressed;
          buttonState.isPressed = isPressed;
        }

        controllerState.analogStates.resize(_analogActions.size());
        for(size_t i = 0; i < _analogActions.size(); ++i)
        {
          auto& analogAction = _analogActions[i];
          if(!analogAction.handle)
          {
            analogAction.handle = _steamInput().GetAnalogActionHandle(analogAction.name);
          }

          auto& analogState = controllerState.analogStates[i];
          auto data = _steamInput().GetAnalogActionData(inputHandle, _analogActions[i].handle);
          analogState.x = data.x;
          analogState.y = data.y;
        }
      }
    }

    void ActivateActionSet(ControllerId controllerId, ActionSetId actionSetId) override
    {
      _steamInput().ActivateActionSet(controllerId, GetActionSetHandle(actionSetId));
    }

    PlayerInputButtonActionState GetButtonActionState(ControllerId controllerId, ButtonActionId actionId) const override
    {
      auto i = _controllers.find(controllerId);
      if(i == _controllers.end()) return {};
      auto const& controller = i->second;
      if(actionId >= controller.buttonStates.size()) return {};
      return controller.buttonStates[actionId];
    }

    PlayerInputAnalogActionState GetAnalogActionState(ControllerId controllerId, AnalogActionId actionId) const override
    {
      auto i = _controllers.find(controllerId);
      if(i == _controllers.end()) return {};
      auto const& controller = i->second;
      if(actionId >= controller.analogStates.size()) return {};
      return controller.analogStates[actionId];
    }

  private:
    SteamInput _steamInput;

    InputActionSetHandle_t GetActionSetHandle(ActionSetId actionSetId)
    {
      auto& actionSet = _actionSets[actionSetId];
      if(!actionSet.handle)
      {
        actionSet.handle = _steamInput().GetActionSetHandle(actionSet.name);
      }
      return actionSet.handle;
    }

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
