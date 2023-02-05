find_package(PulseAudio CONFIG)

if(NOT TARGET PulseAudio::PulseAudio AND PKG_CONFIG_FOUND)
  pkg_search_module(PulseAudio IMPORTED_TARGET libpulse)
  if(TARGET PkgConfig::PulseAudio)
    add_library(PulseAudio::PulseAudio ALIAS PkgConfig::PulseAudio)
  endif()
endif()
