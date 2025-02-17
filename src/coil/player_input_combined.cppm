module;

#include <concepts>
#include <map>
#include <tuple>
#include <unordered_map>
#include <vector>

export module coil.core.player_input.combined;

import coil.core.base.events.map;
import coil.core.base.events;
import coil.core.player_input;

export namespace Coil
{
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
}
