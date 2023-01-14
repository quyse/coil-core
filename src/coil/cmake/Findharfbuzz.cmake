find_package(harfbuzz CONFIG)

if(NOT TARGET harfbuzz::harfbuzz AND PKG_CONFIG_FOUND)
  pkg_search_module(Harfbuzz IMPORTED_TARGET harfbuzz)
  if(TARGET PkgConfig::Harfbuzz)
    add_library(harfbuzz::harfbuzz ALIAS PkgConfig::Harfbuzz)
  endif()
endif()
