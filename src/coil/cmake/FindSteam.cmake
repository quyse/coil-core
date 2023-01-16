if(NOT TARGET Steam::Steam)
  find_path(Steam_INCLUDE_DIRS
    NAMES steam/steam_api.h
  )
  find_library(Steam_LIBRARIES
    NAMES steam_api steam_api64
  )
  add_library(Steam::Steam IMPORTED UNKNOWN)
  set_target_properties(Steam::Steam
    PROPERTIES
    IMPORTED_LOCATION "${Steam_LIBRARIES}"
    INTERFACE_INCLUDE_DIRECTORIES "${Steam_INCLUDE_DIRS}"
  )
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Steam
  REQUIRED_VARS Steam_LIBRARIES Steam_INCLUDE_DIRS
)
mark_as_advanced(Steam_LIBRARIES Steam_INCLUDE_DIRS)
