#pragma once

#include "base.hpp"
#include <memory>

/*
Player Input is a higher-level input library, operating with player actions
rather than low-level device input events.
*/

namespace Coil
{
  class PlayerInputManager;

  enum class PlayerInputActionType
  {
    Button,
    Analog,
  };

  struct PlayerInputButtonActionState
  {
    bool isPressed = false;
    bool isJustChanged = false;
  };

  struct PlayerInputAnalogActionState
  {
    float x = 0;
    float y = 0;
  };

  // base class for player input manager
  class PlayerInputManager
  {
  public:
    using ControllerId = uint64_t;
    using ActionSetId = uint64_t;
    using ButtonActionId = uint64_t;
    using AnalogActionId = uint64_t;

    virtual ActionSetId GetActionSetId(char const* name) = 0;
    virtual ButtonActionId GetButtonActionId(char const* name) = 0;
    virtual AnalogActionId GetAnalogActionId(char const* name) = 0;

    virtual void Update() = 0;
    virtual void ActivateActionSet(ControllerId controllerId, ActionSetId actionSetId) = 0;

    // get state of button action for given controller
    virtual PlayerInputButtonActionState GetButtonActionState(ControllerId controllerId, ButtonActionId actionId) const = 0;
    // get state of analog action for given controller
    virtual PlayerInputAnalogActionState GetAnalogActionState(ControllerId controllerId, AnalogActionId actionId) const = 0;

    // get connected controllers
    std::vector<ControllerId> const& GetControllersIds() const;

  protected:
    std::vector<ControllerId> _controllersIds;
  };


  // helper classes for declaring action sets with adaptable structs

  // registration adapter
  struct PlayerInputActionSetRegistrationAdapter
  {
    template <template <typename> typename ActionSet>
    class Base; // defined later

    template <PlayerInputActionType actionType>
    using Member = std::tuple<>;
  };

  // state adapter
  template <PlayerInputActionType actionType>
  struct PlayerInputActionSetStateAdapterMember;
  template <>
  struct PlayerInputActionSetStateAdapterMember<PlayerInputActionType::Button>
  {
    using Type = PlayerInputButtonActionState;
  };
  template <>
  struct PlayerInputActionSetStateAdapterMember<PlayerInputActionType::Analog>
  {
    using Type = PlayerInputAnalogActionState;
  };
  struct PlayerInputActionSetStateAdapter
  {
    template <PlayerInputActionType actionType>
    using Member = typename PlayerInputActionSetStateAdapterMember<actionType>::Type;

    template <template <typename> typename ActionSet>
    class Base
    {
    public:
      // update action states
      void Update(PlayerInputManager& manager, ActionSet<PlayerInputActionSetRegistrationAdapter> const& registration, PlayerInputManager::ControllerId controllerId)
      {
        for(size_t i = 0; i < registration._buttons.size(); ++i)
        {
          auto const& button = registration._buttons[i];
          static_cast<ActionSet<PlayerInputActionSetStateAdapter>*>(this)->*std::get<1>(button) = manager.GetButtonActionState(controllerId, std::get<2>(button));
        }
        for(size_t i = 0; i < registration._analogs.size(); ++i)
        {
          auto const& analog = registration._analogs[i];
          static_cast<ActionSet<PlayerInputActionSetStateAdapter>*>(this)->*std::get<1>(analog) = manager.GetAnalogActionState(controllerId, std::get<2>(analog));
        }
      }

    protected:
      template <PlayerInputActionType actionType, auto f>
      constexpr Member<actionType> RegisterMember(char const* name)
      {
        return {};
      }
    };
  };

  // base class nested in registration adapter
  template <template <typename> typename ActionSet>
  class PlayerInputActionSetRegistrationAdapter::Base
  {
  public:
    void Register(PlayerInputManager& manager)
    {
      for(size_t i = 0; i < _buttons.size(); ++i)
      {
        std::get<2>(_buttons[i]) = manager.GetButtonActionId(std::get<0>(_buttons[i]));
      }
      for(size_t i = 0; i < _analogs.size(); ++i)
      {
        std::get<2>(_analogs[i]) = manager.GetAnalogActionId(std::get<0>(_analogs[i]));
      }
    }

  protected:
    template <PlayerInputActionType actionType, auto f>
    std::tuple<> RegisterMember(char const* name)
    {
      GetMembers<actionType>().push_back(
      {
        name,
        f.template operator()<ActionSet<PlayerInputActionSetStateAdapter>>(),
        {},
      });
      return {};
    }

  private:
    // get vector of members
    template <PlayerInputActionType actionType>
    auto& GetMembers();
    template <>
    auto& GetMembers<PlayerInputActionType::Button>()
    {
      return _buttons;
    }
    template <>
    auto& GetMembers<PlayerInputActionType::Analog>()
    {
      return _analogs;
    }

    // button actions
    std::vector<
      std::tuple<
        char const*,
        typename PlayerInputActionSetStateAdapter::Member<PlayerInputActionType::Button> ActionSet<PlayerInputActionSetStateAdapter>::*,
        PlayerInputManager::ButtonActionId
      >
    > _buttons;
    // analog actions
    std::vector<
      std::tuple<
        char const*,
        typename PlayerInputActionSetStateAdapter::Member<PlayerInputActionType::Analog> ActionSet<PlayerInputActionSetStateAdapter>::*,
        PlayerInputManager::AnalogActionId
      >
    > _analogs;

    // allow state adapter access the vectors
    friend PlayerInputActionSetStateAdapter::Base<ActionSet>;
  };
}
