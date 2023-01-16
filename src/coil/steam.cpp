#include "steam.hpp"
#include <steam/steam_api.h>

namespace Coil
{
  Steam& Steam::Get()
  {
    static Steam steam;
    return steam;
  }

  Steam::Steam()
  {
    SteamAPI_Init();
  }

  Steam::~Steam()
  {
    SteamAPI_Shutdown();
  }
}
