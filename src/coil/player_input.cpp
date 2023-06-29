#include "player_input.hpp"

namespace Coil
{
  std::vector<PlayerInputManager::ControllerId> const& PlayerInputManager::GetControllersIds() const
  {
    return _controllersIds;
  }
}
