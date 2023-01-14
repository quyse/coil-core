find_package(Opus CONFIG)

if(NOT TARGET Opus::opus AND PKG_CONFIG_FOUND)
  pkg_search_module(Opus IMPORTED_TARGET opus)
  if(TARGET PkgConfig::Opus)
    add_library(Opus::opus ALIAS PkgConfig::Opus)
  endif()
endif()
