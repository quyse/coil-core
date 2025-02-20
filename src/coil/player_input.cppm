module;

#include <memory>
#include <variant>
#include <vector>

export module coil.core.player_input;

import coil.core.base.events.map;
import coil.core.base.events;
import coil.core.base.signals;
import coil.core.base;
import coil.core.math;

/*
Player Input is a higher-level input library, operating with player actions
rather than low-level device input events.

Modelled mostly after Steam Input API https://partner.steamgames.com/doc/api/isteaminput
and shares its limitations.

Notably:
All actions must have unique names. Same action in different action sets must be named differently.

*/

export namespace Coil
{
  class PlayerInputManager;

  enum class PlayerInputActionType
  {
    Button,
    Analog,
  };

  struct PlayerInputButtonAction
  {
    SignalPtr<bool> sigIsPressed;
  };

  struct PlayerInputAnalogAction
  {
    // relative or absolute value
    std::variant<EventPtr<vec2>, SignalPtr<vec2>> relativeOrAbsoluteValue;
  };

  // base class for player input manager
  class PlayerInputManager
  {
  public:
    using ControllerId = uint64_t;
    using ActionSetId = uint64_t;
    using ButtonActionId = uint64_t;
    using AnalogActionId = uint64_t;

    virtual ActionSetId GetActionSetId(std::string_view name) = 0;
    virtual ButtonActionId GetButtonActionId(std::string_view name) = 0;
    virtual AnalogActionId GetAnalogActionId(std::string_view name) = 0;

    virtual void Update() = 0;
    virtual void ActivateActionSet(ControllerId controllerId, ActionSetId actionSetId) = 0;

    // get state of button action for given controller
    virtual PlayerInputButtonAction GetButtonAction(ControllerId controllerId, ActionSetId actionSetId, ButtonActionId actionId) const = 0;
    // get state of analog action for given controller
    virtual PlayerInputAnalogAction GetAnalogAction(ControllerId controllerId, ActionSetId actionSetId, AnalogActionId actionId) const = 0;

    // get connected controllers
    SyncSetPtr<ControllerId> const& GetControllers() const
    {
      return pControllersIds_;
    }

  protected:
    // must be initialized by implementation
    SyncSetPtr<ControllerId> pControllersIds_;
  };


  // helper classes for declaring action sets with adaptable structs

  // registration adapter
  struct PlayerInputActionSetRegistrationAdapter
  {
    template <template <typename> typename ActionSet>
    class Base; // defined later

    template <PlayerInputActionType actionType>
    using Field = std::tuple<>;
  };

  // state adapter
  struct PlayerInputActionSetAdapter
  {
    template <PlayerInputActionType actionType>
    struct Field;

    template <>
    struct Field<PlayerInputActionType::Button>
    {
      SignalPtr<bool> sigIsPressed;
    };

    template <>
    struct Field<PlayerInputActionType::Analog>
    {
      EventPtr<vec2> evRelativeValue;
      SignalPtr<vec2> sigAbsoluteValue;
    };

    template <template <typename> typename ActionSet>
    class Base
    {
    public:
      void Register(PlayerInputManager& manager, ActionSet<PlayerInputActionSetRegistrationAdapter> const& registration, PlayerInputManager::ControllerId controllerId)
      {
        actionSetId_ = registration.actionSetId_;
        controllerId_ = controllerId;

        for(size_t i = 0; i < registration.buttons_.size(); ++i)
        {
          auto const& button = registration.buttons_[i];
          auto& field = static_cast<ActionSet<PlayerInputActionSetAdapter>*>(this)->*(button.pMember);
          field.sigIsPressed = manager.GetButtonAction(controllerId, registration.actionSetId_, button.actionId).sigIsPressed;
        }

        for(size_t i = 0; i < registration.analogs_.size(); ++i)
        {
          auto const& analog = registration.analogs_[i];
          auto& field = static_cast<ActionSet<PlayerInputActionSetAdapter>*>(this)->*(analog.pMember);
          std::visit([&](auto const& relativeOrAbsoluteValue)
          {
            // if it's relative value event
            if constexpr(std::same_as<std::decay_t<decltype(relativeOrAbsoluteValue)>, EventPtr<vec2>>)
            {
              auto sigAbsoluteValue = MakeVariableSignal<vec2>({});
              field.evRelativeValue = MakeEventDependentOnEvent([sigAbsoluteValue](vec2 const& relativeValue) -> vec2
              {
                sigAbsoluteValue.SetIfDiffers(sigAbsoluteValue.Get() + relativeValue);
                return relativeValue;
              }, relativeOrAbsoluteValue);
              field.sigAbsoluteValue = sigAbsoluteValue;
            }
            // else it's absolute value signal
            else
            {
              field.evRelativeValue = MakeEventDependentOnEvent([relativeOrAbsoluteValue, lastAbsoluteValue = vec2()]() mutable -> vec2
              {
                vec2 absoluteValue = relativeOrAbsoluteValue.Get();
                vec2 relativeValue = absoluteValue - lastAbsoluteValue;
                lastAbsoluteValue = absoluteValue;
                return relativeValue;
              }, static_cast<EventPtr<>>(relativeOrAbsoluteValue));
              field.sigAbsoluteValue = relativeOrAbsoluteValue;
            }
          }, manager.GetAnalogAction(controllerId, registration.actionSetId_, analog.actionId).relativeOrAbsoluteValue);
        }
      }

      void Activate(PlayerInputManager& manager)
      {
        manager.ActivateActionSet(controllerId_, actionSetId_);
      }

    protected:
      template <PlayerInputActionType actionType, auto f, Literal name>
      constexpr Field<actionType> RegisterField()
      {
        return {};
      }

    private:
      PlayerInputManager::ActionSetId actionSetId_ = {};
      PlayerInputManager::ControllerId controllerId_ = {};
    };
  };

  // base class nested in registration adapter
  template <template <typename> typename ActionSet>
  class PlayerInputActionSetRegistrationAdapter::Base
  {
  public:
    void Register(PlayerInputManager& manager, std::string_view actionSetName)
    {
      actionSetId_ = manager.GetActionSetId(actionSetName);
      for(size_t i = 0; i < buttons_.size(); ++i)
      {
        buttons_[i].actionId = manager.GetButtonActionId(buttons_[i].name);
      }
      for(size_t i = 0; i < analogs_.size(); ++i)
      {
        analogs_[i].actionId = manager.GetAnalogActionId(analogs_[i].name);
      }
    }

  protected:
    template <PlayerInputActionType actionType, auto f, Literal name>
    Field<actionType> RegisterField()
    {
      return ActionsHelper<actionType, f>::RegisterField(*this, name);
    }

  private:
    // actions helper
    template <PlayerInputActionType actionType, auto f>
    struct ActionsHelper;
    template <auto f>
    struct ActionsHelper<PlayerInputActionType::Button, f>
    {
      static Field<PlayerInputActionType::Button> RegisterField(Base& self, std::string_view name)
      {
        self.buttons_.push_back(ButtonAction
        {
          .name = name,
          .actionId = {},
          .pMember = f.template operator()<ActionSet<PlayerInputActionSetAdapter>>(),
        });
        return {};
      }
    };
    template <auto f>
    struct ActionsHelper<PlayerInputActionType::Analog, f>
    {
      static Field<PlayerInputActionType::Analog> RegisterField(Base& self, std::string_view name)
      {
        self.analogs_.push_back(AnalogAction
        {
          .name = name,
          .actionId = {},
          .pMember = f.template operator()<ActionSet<PlayerInputActionSetAdapter>>(),
        });
        return {};
      }
    };

    PlayerInputManager::ActionSetId actionSetId_ = {};

    // button actions
    struct ButtonAction
    {
      std::string_view name;
      PlayerInputManager::ButtonActionId actionId;
      PlayerInputActionSetAdapter::Field<PlayerInputActionType::Button> ActionSet<PlayerInputActionSetAdapter>::*pMember;
    };
    std::vector<ButtonAction> buttons_;

    // analog actions
    struct AnalogAction
    {
      std::string_view name;
      PlayerInputManager::AnalogActionId actionId;
      PlayerInputActionSetAdapter::Field<PlayerInputActionType::Analog> ActionSet<PlayerInputActionSetAdapter>::*pMember;
    };
    std::vector<AnalogAction> analogs_;

    // allow state adapter access the vectors
    friend PlayerInputActionSetAdapter::Base<ActionSet>;
  };
}
