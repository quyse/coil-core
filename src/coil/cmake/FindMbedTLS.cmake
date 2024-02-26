find_package(MbedTLS CONFIG)

if(NOT TARGET MbedTLS::mbedcrypto)
  find_path(MbedTLS_INCLUDE_DIRS
    NAMES mbedtls/sha256.h
  )
  find_library(MbedTLS_LIBRARIES
    NAMES mbedcrypto
  )
  if(MbedTLS_INCLUDE_DIRS AND MbedTLS_LIBRARIES)
    add_library(MbedTLS::mbedcrypto IMPORTED UNKNOWN)
    set_target_properties(MbedTLS::mbedcrypto
      PROPERTIES
      IMPORTED_LOCATION "${MbedTLS_LIBRARIES}"
      INTERFACE_INCLUDE_DIRECTORIES "${MbedTLS_INCLUDE_DIRS}"
    )
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MbedTLS
  REQUIRED_VARS MbedTLS_LIBRARIES MbedTLS_INCLUDE_DIRS
)
mark_as_advanced(MbedTLS_LIBRARIES MbedTLS_INCLUDE_DIRS)
