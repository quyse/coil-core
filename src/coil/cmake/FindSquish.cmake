if(NOT TARGET Squish::Squish)
  find_path(Squish_INCLUDE_DIRS
    NAMES squish.h
  )
  find_library(Squish_LIBRARIES
    NAMES squish
  )
  if(Squish_INCLUDE_DIRS AND Squish_LIBRARIES)
    add_library(Squish::Squish IMPORTED UNKNOWN)
    set_target_properties(Squish::Squish
      PROPERTIES
      IMPORTED_LOCATION "${Squish_LIBRARIES}"
      INTERFACE_INCLUDE_DIRECTORIES "${Squish_INCLUDE_DIRS}"
    )
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Squish
  REQUIRED_VARS Squish_LIBRARIES Squish_INCLUDE_DIRS
)
mark_as_advanced(Squish_LIBRARIES Squish_INCLUDE_DIRS)
