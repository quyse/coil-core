#include "steam.hpp"

namespace Coil
{
  class Steam::Global
  {
  public:
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

  private:
    bool _initialized = false;

    friend Steam;
  };

  Steam::Steam()
  {
    static std::shared_ptr<Global> global = std::make_shared<Global>();
    _global = global;
    _initialized = global->_initialized;
  }

  Steam::operator bool() const
  {
    return _initialized;
  }

  void Steam::Update()
  {
    SteamAPI_RunCallbacks();
  }

  class SteamInput::Global
  {
  public:
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

  SteamInput::SteamInput()
  {
    static std::shared_ptr<Global> global = std::make_shared<Global>();
    _global = global;
  }

  ISteamInput& SteamInput::operator()() const
  {
    return *_global->_steamInput;
  }

  PlayerInputManager::ActionSetId SteamPlayerInputManager::GetActionSetId(char const* name)
  {
    ActionSetId actionSetId = _actionSets.size();
    _actionSets.push_back({
      .name = name,
    });
    return actionSetId;
  }

  PlayerInputManager::ButtonActionId SteamPlayerInputManager::GetButtonActionId(char const* name)
  {
    ButtonActionId actionId = _buttonActions.size();
    _buttonActions.push_back({
      .name = name,
    });
    return actionId;
  }

  PlayerInputManager::AnalogActionId SteamPlayerInputManager::GetAnalogActionId(char const* name)
  {
    AnalogActionId actionId = _analogActions.size();
    _analogActions.push_back({
      .name = name,
    });
    return actionId;
  }

  void SteamPlayerInputManager::Update()
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

  void SteamPlayerInputManager::ActivateActionSet(ControllerId controllerId, ActionSetId actionSetId)
  {
    _steamInput().ActivateActionSet(controllerId, GetActionSetHandle(actionSetId));
  }

  PlayerInputButtonActionState SteamPlayerInputManager::GetButtonActionState(ControllerId controllerId, ButtonActionId actionId) const
  {
    auto i = _controllers.find(controllerId);
    if(i == _controllers.end()) return {};
    auto const& controller = i->second;
    if(actionId >= controller.buttonStates.size()) return {};
    return controller.buttonStates[actionId];
  }

  PlayerInputAnalogActionState SteamPlayerInputManager::GetAnalogActionState(ControllerId controllerId, AnalogActionId actionId) const
  {
    auto i = _controllers.find(controllerId);
    if(i == _controllers.end()) return {};
    auto const& controller = i->second;
    if(actionId >= controller.analogStates.size()) return {};
    return controller.analogStates[actionId];
  }

  InputActionSetHandle_t SteamPlayerInputManager::GetActionSetHandle(ActionSetId actionSetId)
  {
    auto& actionSet = _actionSets[actionSetId];
    if(!actionSet.handle)
    {
      actionSet.handle = _steamInput().GetActionSetHandle(actionSet.name);
    }
    return actionSet.handle;
  }
}
