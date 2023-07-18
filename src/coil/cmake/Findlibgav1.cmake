find_package(libgav1 CONFIG)

if(NOT TARGET libgav1::libgav1)
  find_path(LIBGAV1_INCLUDE_DIRS
    NAMES gav1/decoder.h
  )
  find_library(LIBGAV1_LIBRARIES
    NAMES gav1 libgav1_shared
  )
  if(LIBGAV1_INCLUDE_DIRS AND LIBGAV1_LIBRARIES)
    add_library(libgav1::libgav1 IMPORTED UNKNOWN)
    set_target_properties(libgav1::libgav1
      PROPERTIES
      IMPORTED_LOCATION "${LIBGAV1_LIBRARIES}"
      INTERFACE_INCLUDE_DIRECTORIES "${LIBGAV1_INCLUDE_DIRS}"
    )
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libgav1
  REQUIRED_VARS LIBGAV1_LIBRARIES LIBGAV1_INCLUDE_DIRS
)
mark_as_advanced(LIBGAV1_LIBRARIES LIBGAV1_INCLUDE_DIRS)
