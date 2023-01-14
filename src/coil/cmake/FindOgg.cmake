find_package(Ogg CONFIG)

if(NOT TARGET Ogg::ogg AND PKG_CONFIG_FOUND)
  pkg_search_module(Ogg IMPORTED_TARGET ogg)
  if(TARGET PkgConfig::Ogg)
    add_library(Ogg::ogg ALIAS PkgConfig::Ogg)
  endif()
endif()
