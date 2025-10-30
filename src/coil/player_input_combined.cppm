module;

#include <concepts>
#include <map>
#include <ranges>
#include <tuple>
#include <unordered_map>
#include <variant>
#include <vector>

export module coil.core.player_input.combined;

import coil.core.base.events.map;
import coil.core.base.events;
import coil.core.base.signals;
import coil.core.math;
import coil.core.player_input;

export namespace Coil
{
  // player input manager combining multiple managers
  template <std::derived_from<PlayerInputManager>... Managers>
  class CombinedPlayerInputManager final : public PlayerInputManager
  {
  public:
    CombinedPlayerInputManager(Managers&... managers)
    : managers_{managers...}
    {
      // initialze cells watching controller ids
      size_t managerIndex = 0;
      ((cells_[managerIndex].Init(this, &managers, managerIndex, managers.GetControllers()->GetEvent()), ++managerIndex), ...);
      // initialize ids
      pControllersIds_ = MakeSyncSet<ControllerId>();
    }

    // PlayerInputManager's methods

    ActionSetId GetActionSetId(std::string_view name) override
    {
      return std::apply([&](Managers&... managers) -> ActionSetId
      {
        ActionSetId actionSetId = actionSets_.size();
        actionSets_.push_back({ managers.GetActionSetId(name)... });
        return actionSetId;
      }, managers_);
    }

    ButtonActionId GetButtonActionId(std::string_view name) override
    {
      return std::apply([&](Managers&... managers) -> ButtonActionId
      {
        ButtonActionId actionId = buttonActions_.size();
        buttonActions_.push_back({ managers.GetButtonActionId(name)... });
        return actionId;
      }, managers_);
    }

    AnalogActionId GetAnalogActionId(std::string_view name) override
    {
      return std::apply([&](Managers&... managers) -> AnalogActionId
      {
        AnalogActionId actionId = analogActions_.size();
        analogActions_.push_back({ managers.GetAnalogActionId(name)... });
        return actionId;
      }, managers_);
    }

    void Update() override
    {
      std::apply([&](Managers&... managers)
      {
        // update managers
        (managers.Update(), ...);
      }, managers_);
    }

    void ActivateActionSet(ControllerId controllerId, ActionSetId actionSetId) override
    {
      auto [pManager, managerIndex, managerControllerId] = GetManagerControllerId(controllerId);
      pManager->ActivateActionSet(managerControllerId, actionSets_[actionSetId][managerIndex]);
    }

    PlayerInputButtonAction GetButtonAction(ControllerId controllerId, ActionSetId actionSetId, ButtonActionId actionId) const override
    {
      auto [pManager, managerIndex, managerControllerId] = GetManagerControllerId(controllerId);
      return pManager->GetButtonAction(managerControllerId, actionSets_[actionId][managerIndex], buttonActions_[actionId][managerIndex]);
    }

    PlayerInputAnalogAction GetAnalogAction(ControllerId controllerId, ActionSetId actionSetId, AnalogActionId actionId) const override
    {
      auto [pManager, managerIndex, managerControllerId] = GetManagerControllerId(controllerId);
      return pManager->GetAnalogAction(managerControllerId, actionSets_[actionId][managerIndex], analogActions_[actionId][managerIndex]);
    }

  private:
    void OnAddController(PlayerInputManager* pManager, size_t managerIndex, ControllerId managerControllerId)
    {
      controllersIdToManager_.insert({nextControllerId_, {pManager, managerIndex, managerControllerId}});
      controllersManagerToId_.insert({{managerIndex, managerControllerId}, nextControllerId_});
      pControllersIds_->Set(nextControllerId_, {{}});
      ++nextControllerId_;
    }
    void OnDeleteController(size_t managerIndex, ControllerId managerControllerId)
    {
      auto i = controllersManagerToId_.find({managerIndex, managerControllerId});
      if(i != controllersManagerToId_.end())
      {
        ControllerId controllerId = i->second;
        controllersIdToManager_.erase(controllerId);
        controllersManagerToId_.erase(i);
        pControllersIds_->Set(controllerId, {});
      }
    }

    class Cell : public Event<ControllerId, std::tuple<> const*>::Cell
    {
    public:
      void Init(CombinedPlayerInputManager* pSelf, PlayerInputManager* pManager, size_t index, EventPtr<ControllerId, std::tuple<> const*> const& pEvent)
      {
        pSelf_ = pSelf;
        pManager_ = pManager;
        index_ = index;
        SubscribeTo(pEvent);
      }

      void Notify(ControllerId key, std::tuple<> const* pValue) override
      {
        if(pValue)
        {
          pSelf_->OnAddController(pManager_, index_, key);
        }
        else
        {
          pSelf_->OnDeleteController(index_, key);
        }
      }

    private:
      CombinedPlayerInputManager* pSelf_ = nullptr;
      PlayerInputManager* pManager_ = nullptr;
      size_t index_ = -1;
    };

    std::tuple<PlayerInputManager*, size_t, ControllerId> GetManagerControllerId(ControllerId controllerId) const
    {
      auto i = controllersIdToManager_.find(controllerId);
      if(i == controllersIdToManager_.end()) return {};
      return i->second;
    }

    std::tuple<Managers&...> managers_;
    std::array<Cell, sizeof...(Managers)> cells_;
    std::unordered_map<ControllerId, std::tuple<PlayerInputManager*, size_t, ControllerId>> controllersIdToManager_;
    std::map<std::tuple<size_t, ControllerId>, ControllerId> controllersManagerToId_;
    size_t nextControllerId_ = 0;
    // internal controller ids used by managers
    std::vector<ControllerId> managersControllersIds_;
    // number of controllers per manager
    std::array<size_t, sizeof...(Managers)> managersControllersCounts_;
    // internal ids of action sets and actions, kept for each manager
    std::vector<std::array<ActionSetId, sizeof...(Managers)>> actionSets_;
    std::vector<std::array<ButtonActionId, sizeof...(Managers)>> buttonActions_;
    std::vector<std::array<AnalogActionId, sizeof...(Managers)>> analogActions_;
  };

  // player input manager combining all controllers into a single one
  template <std::derived_from<PlayerInputManager> Manager>
  class SingleControllerCombinedPlayerInputManager final : public PlayerInputManager
  {
  public:
    SingleControllerCombinedPlayerInputManager(Manager& manager)
    : manager_{manager}
    {
      // always a single controller
      pControllersIds_ = MakeSyncSet<ControllerId>();
      pControllersIds_->Set(0, {{}});
    }

    // PlayerInputManager's methods

    ActionSetId GetActionSetId(std::string_view name) override
    {
      return manager_.GetActionSetId(name);
    }

    ButtonActionId GetButtonActionId(std::string_view name) override
    {
      return manager_.GetButtonActionId(name);
    }

    AnalogActionId GetAnalogActionId(std::string_view name) override
    {
      return manager_.GetAnalogActionId(name);
    }

    void Update() override
    {
      manager_.Update();
    }

    void ActivateActionSet(ControllerId, ActionSetId actionSetId) override
    {
      for(auto const& [controllerId, _] : *manager_.GetControllers())
      {
        manager_.ActivateActionSet(controllerId, actionSetId);
      }
    }

    PlayerInputButtonAction GetButtonAction(ControllerId, ActionSetId actionSetId, ButtonActionId actionId) const override
    {
      auto sigButtonsActions = manager_.GetControllers()->Map([this, actionSetId, actionId](ControllerId controllerId, auto)
      {
        return manager_.GetButtonAction(controllerId, actionSetId, actionId);
      })->CreateSignal();
      auto sigSigIsPressed = MakeSignalDependentOnSignals([](std::unordered_map<ControllerId, PlayerInputButtonAction> const& buttonsActions)
      {
        return MakeSignalDependentOnRange([](auto rangeIsPressed)
        {
          for(bool isPressed : rangeIsPressed)
          {
            if(isPressed) return true;
          }
          return false;
        }, buttonsActions | std::views::values | std::views::transform([](PlayerInputButtonAction const& buttonAction)
        {
          return buttonAction.sigIsPressed;
        }));
      }, std::move(sigButtonsActions));

      return
      {
        .sigIsPressed = MakeSignalUnpackingSignal(std::move(sigSigIsPressed)),
      };
    }

    PlayerInputAnalogAction GetAnalogAction(ControllerId controllerId, ActionSetId actionSetId, AnalogActionId actionId) const override
    {
      auto sigButtonsActions = manager_.GetControllers()->Map([this, actionSetId, actionId](ControllerId controllerId, auto)
      {
        return manager_.GetAnalogAction(controllerId, actionSetId, actionId);
      })->CreateSignal();
      auto sigAbsoluteValue = MakeSignalDependentOnSignals([](std::unordered_map<ControllerId, PlayerInputAnalogAction> const& analogsActions)
      {
        return MakeSignalDependentOnRange([](auto rangeAbsoluteValue)
        {
          vec2 totalAbsoluteValue;
          for(vec2 absoluteValue : rangeAbsoluteValue)
          {
            totalAbsoluteValue += absoluteValue;
          }
          return totalAbsoluteValue;
        }, analogsActions | std::views::values | std::views::transform([](PlayerInputAnalogAction const& analogAction)
        {
          return std::visit([](auto const& relativeOrAbsoluteValue) -> std::optional<SignalPtr<vec2>>
          {
            // if it's relative event
            if constexpr(std::same_as<std::decay_t<decltype(relativeOrAbsoluteValue)>, EventPtr<vec2>>)
            {
              // TODO
              return {};
            }
            else
            {
              return {relativeOrAbsoluteValue};
            }
          }, analogAction.relativeOrAbsoluteValue);
        }) | std::views::filter([](auto const& opt)
        {
          return opt.has_value();
        }) | std::views::transform([](auto const& opt)
        {
          return opt.value();
        }));
      }, std::move(sigButtonsActions));

      return
      {
        .relativeOrAbsoluteValue = MakeSignalUnpackingSignal(std::move(sigAbsoluteValue)),
      };
    }

  private:
    Manager& manager_;

    std::unordered_map<ButtonActionId, PlayerInputButtonAction> buttonActions_;
    std::unordered_map<AnalogActionId, PlayerInputAnalogAction> analogActions_;
  };
}
