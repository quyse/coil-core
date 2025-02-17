module;

#include <steam/steam_api.h>
#include <memory>
#include <string_view>
#include <unordered_map>

export module coil.core.steam;

import coil.core.base.events.map;
import coil.core.base.signals;
import coil.core.math;
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
        initialized_ = SteamAPI_Init();
      }

      ~Global()
      {
        if(initialized_)
        {
          SteamAPI_Shutdown();
        }
      }

      bool initialized_ = false;
    };

  public:
    Steam()
    {
      static std::shared_ptr<Global> global = std::make_shared<Global>();
      global_ = global;
      initialized_ = global->initialized_;
    }

    // return if initialized
    operator bool() const
    {
      return initialized_;
    }

    // run every frame
    void Update()
    {
      SteamAPI_RunCallbacks();
    }

  private:
    std::shared_ptr<Global> global_;
    bool initialized_ = false;
  };

  class SteamInput
  {
  private:
    struct Global
    {
      Global()
      {
        steamInput_->Init(false);
      }

      ~Global()
      {
        if(steamInput_)
        {
          steamInput_->Shutdown();
        }
      }

      Steam steam_;
      ISteamInput* const steamInput_ = ::SteamInput();
    };

  public:
    SteamInput()
    {
      static std::shared_ptr<Global> global = std::make_shared<Global>();
      global_ = global;
    }

    ISteamInput& operator()() const
    {
      return *global_->steamInput_;
    }

  private:
    std::shared_ptr<Global> global_;
  };

  class SteamPlayerInputManager final : public PlayerInputManager
  {
  public:
    SteamPlayerInputManager()
    {
      // initialize ids
      pControllersIds_ = pControllers_->Map([](ControllerId controllerId, Controller const&) -> std::tuple<>
      {
        return {};
      });
    }

    // PlayerInputManager's methods

    ActionSetId GetActionSetId(std::string_view name) override
    {
      ActionSetId actionSetId = actionSets_.size();
      actionSets_.push_back({
        .name = std::string{name},
      });
      return actionSetId;
    }

    ButtonActionId GetButtonActionId(std::string_view name) override
    {
      ButtonActionId actionId = buttonActions_.size();
      buttonActions_.push_back({
        .name = std::string{name},
      });
      return actionId;
    }

    AnalogActionId GetAnalogActionId(std::string_view name) override
    {
      AnalogActionId actionId = analogActions_.size();
      analogActions_.push_back({
        .name = std::string{name},
      });
      return actionId;
    }

    void Update() override
    {
      // get active controllers
      tempInputHandles_.resize(STEAM_INPUT_MAX_COUNT);
      tempInputHandles_.resize(steamInput_().GetConnectedControllers(tempInputHandles_.data()));

      // remove non-active controllers
      // mark all controllers non-active first
      for(auto& [inputHandle, controllerState] : *pControllers_)
        controllerState.active = false;
      // mark active controllers
      for(size_t i = 0; i < tempInputHandles_.size(); ++i)
      {
        auto inputHandle = tempInputHandles_[i];
        Controller* pController = pControllers_->Get(inputHandle);
        if(!pController)
        {
          pController = pControllers_->Set(inputHandle, {{}});
        }
        pController->active = true;
      }
      // remove remaining non-active controllers
      tempInputHandles_.clear();
      for(auto const& [inputHandle, controller] : *pControllers_)
      {
        if(!controller.active)
        {
          tempInputHandles_.push_back(inputHandle);
        }
      }
      for(size_t i = 0; i < tempInputHandles_.size(); ++i)
      {
        pControllers_->Set(tempInputHandles_[i], {});
      }

      // update states
      for(auto const& [inputHandle, controller] : *pControllers_)
      {
        for(auto const& [buttonActionId, controllerButtonAction] : controller.buttonActions)
        {
          auto& buttonAction = buttonActions_[buttonActionId];
          if(!buttonAction.handle)
          {
            buttonAction.handle = steamInput_().GetDigitalActionHandle(buttonAction.name.c_str());
          }

          bool isPressed = steamInput_().GetDigitalActionData(inputHandle, buttonAction.handle).bState;
          controllerButtonAction.sigIsPressed.SetIfDiffers(isPressed);
        }

        for(auto const& [analogActionId, controllerAnalogAction] : controller.analogActions)
        {
          auto& analogAction = analogActions_[analogActionId];
          if(!analogAction.handle)
          {
            analogAction.handle = steamInput_().GetAnalogActionHandle(analogAction.name.c_str());
          }

          auto data = steamInput_().GetAnalogActionData(inputHandle, analogAction.handle);
          controllerAnalogAction.sigValue.SetIfDiffers(vec2(data.x, data.y));
        }
      }
    }

    void ActivateActionSet(ControllerId controllerId, ActionSetId actionSetId) override
    {
      steamInput_().ActivateActionSet(controllerId, GetActionSetHandle(actionSetId));
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
      auto const& action = pController->GetAnalogAction(actionId);
      return
      {
        .relativeOrAbsoluteValue = action.sigValue,
      };
    }

  private:
    SteamInput steamInput_;

    InputActionSetHandle_t GetActionSetHandle(ActionSetId actionSetId)
    {
      auto& actionSet = actionSets_[actionSetId];
      if(!actionSet.handle)
      {
        actionSet.handle = steamInput_().GetActionSetHandle(actionSet.name.c_str());
      }
      return actionSet.handle;
    }

    struct ActionSet
    {
      InputActionSetHandle_t handle = 0;
      std::string name;
    };
    std::vector<ActionSet> actionSets_;

    struct ButtonAction
    {
      InputDigitalActionHandle_t handle = 0;
      std::string name;
    };
    std::vector<ButtonAction> buttonActions_;

    struct AnalogAction
    {
      InputAnalogActionHandle_t handle = 0;
      std::string name;
    };
    std::vector<AnalogAction> analogActions_;

    struct Controller
    {
      struct ButtonAction
      {
        SignalVarPtr<bool> sigIsPressed;
      };

      struct AnalogAction
      {
        SignalVarPtr<vec2> sigValue;
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

      AnalogAction const& GetAnalogAction(AnalogActionId actionId)
      {
        auto i = analogActions.find(actionId);
        if(i != analogActions.end()) return i->second;
        return analogActions.insert({actionId,
        {
          .sigValue = MakeVariableSignal<vec2>({}),
        }}).first->second;
      }

      std::unordered_map<ButtonActionId, ButtonAction> buttonActions;
      std::unordered_map<AnalogActionId, AnalogAction> analogActions;
      bool active = false;
    };
    SyncMapPtr<ControllerId, Controller> pControllers_ = MakeSyncMap<ControllerId, Controller>();

    std::vector<InputHandle_t> tempInputHandles_;
  };
}
