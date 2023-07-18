find_package(Webm CONFIG)

if(NOT TARGET Webm::Webm)
  find_path(Webm_INCLUDE_DIRS
    NAMES webm/webm_parser.h
  )
  find_library(Webm_LIBRARIES
    NAMES webm
  )
  if(Webm_INCLUDE_DIRS AND Webm_LIBRARIES)
    add_library(Webm::Webm IMPORTED UNKNOWN)
    set_target_properties(Webm::Webm
      PROPERTIES
      IMPORTED_LOCATION "${Webm_LIBRARIES}"
      INTERFACE_INCLUDE_DIRECTORIES "${Webm_INCLUDE_DIRS}"
    )
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Webm
  REQUIRED_VARS Webm_LIBRARIES Webm_INCLUDE_DIRS
)
mark_as_advanced(Webm_LIBRARIES Webm_INCLUDE_DIRS)
