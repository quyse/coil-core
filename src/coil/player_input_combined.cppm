module;

#include <concepts>
#include <tuple>
#include <vector>

export module coil.core.player_input.combined;

import coil.core.player_input;

export namespace Coil
{
  template <std::derived_from<PlayerInputManager>... Managers>
  class CombinedPlayerInputManager final : public PlayerInputManager
  {
  public:
    CombinedPlayerInputManager(Managers&... managers)
    : _managers(managers...) {}

    // PlayerInputManager's methods

    ActionSetId GetActionSetId(char const* name) override
    {
      return std::apply([&](Managers&... managers) -> ActionSetId
      {
        ActionSetId actionSetId = _actionSets.size();
        _actionSets.push_back({ managers.GetActionSetId(name)... });
        return actionSetId;
      }, _managers);
    }

    ButtonActionId GetButtonActionId(std::string_view name) override
    {
      return std::apply([&](Managers&... managers) -> ButtonActionId
      {
        ButtonActionId actionId = _buttonActions.size();
        _buttonActions.push_back({ managers.GetButtonActionId(name)... });
        return actionId;
      }, _managers);
    }

    AnalogActionId GetAnalogActionId(std::string_view name) override
    {
      return std::apply([&](Managers&... managers) -> AnalogActionId
      {
        AnalogActionId actionId = _analogActions.size();
        _analogActions.push_back({ managers.GetAnalogActionId(name)... });
        return actionId;
      }, _managers);
    }

    void Update() override
    {
      std::apply([&](Managers&... managers)
      {
        // update managers
        (managers.Update(), ...);

        // update controller ids
        _managersControllersIds.clear();
        _controllersIds.clear();
        size_t managerIndex = 0;
        ControllerId nextControllerId = 0;
        ([&](Managers& managers)
        {
          auto const& controllersIds = managers.GetControllersIds();
          for(size_t i = 0; i < controllersIds.size(); ++i)
          {
            _managersControllersIds.push_back(controllersIds[i]);
            _controllersIds.push_back(nextControllerId++);
          }
          _managersControllersCounts[managerIndex++] = controllersIds.size();
        }(managers), ... );
      }, _managers);
    }

    void ActivateActionSet(ControllerId controllerId, ActionSetId actionSetId) override
    {
      auto [pManager, managerIndex, managerControllerId] = GetManagerControllerId(controllerId);
      pManager->ActivateActionSet(managerControllerId, _actionSets[actionSetId][managerIndex]);
    }

    PlayerInputButtonActionState GetButtonActionState(ControllerId controllerId, ButtonActionId actionId) const override
    {
      auto [pManager, managerIndex, managerControllerId] = GetManagerControllerId(controllerId);
      return pManager->GetButtonActionState(managerControllerId, _buttonActions[actionId][managerIndex]);
    }

    PlayerInputAnalogActionState GetAnalogActionState(ControllerId controllerId, AnalogActionId actionId) const override
    {
      auto [pManager, managerIndex, managerControllerId] = GetManagerControllerId(controllerId);
      return pManager->GetAnalogActionState(managerControllerId, _analogActions[actionId][managerIndex]);
    }

  private:
    std::tuple<PlayerInputManager*, size_t, ControllerId> GetManagerControllerId(ControllerId controllerId) const
    {
      return std::apply([&](Managers&... managers) -> auto
      {
        std::tuple<PlayerInputManager*, size_t, ControllerId> r = {};
        size_t managerIndex = 0;
        ControllerId localControllerId = controllerId;
        (void)((
          (localControllerId < _managersControllersCounts[managerIndex])
          ? (r = { &managers, managerIndex, _managersControllersIds[controllerId] }, true)
          : (localControllerId -= _managersControllersCounts[managerIndex++], false)
        ) || ...);
        return r;
      }, _managers);
    }

    std::tuple<Managers&...> _managers;
    // internal controller ids used by managers
    std::vector<ControllerId> _managersControllersIds;
    // number of controllers per manager
    std::array<size_t, sizeof...(Managers)> _managersControllersCounts;
    // internal ids of action sets and actions, kept for each manager
    std::vector<std::array<ActionSetId, sizeof...(Managers)>> _actionSets;
    std::vector<std::array<ButtonActionId, sizeof...(Managers)>> _buttonActions;
    std::vector<std::array<AnalogActionId, sizeof...(Managers)>> _analogActions;
  };
}
