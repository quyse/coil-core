cmake_minimum_required(VERSION 3.19)

project(coil_core VERSION 3.0)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZAITON ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(cmake/deps.cmake)


if(TRUE)
  add_library(coil_core_base STATIC
    base.cpp
  )
  set_property(TARGET coil_core_base PROPERTY PUBLIC_HEADER
    base.hpp
    base_debug.hpp
  )
  list(APPEND coil_core_libraries base)
endif()
list(APPEND coil_core_all_libraries base)

if(TRUE)
  add_library(coil_core_unicode INTERFACE)
  set_property(TARGET coil_core_unicode PROPERTY PUBLIC_HEADER
    unicode.hpp
  )
  list(APPEND coil_core_libraries unicode)
endif()
list(APPEND coil_core_all_libraries unicode)

if(WIN32)
  add_library(coil_core_windows STATIC
    windows.cpp
  )
  set_property(TARGET coil_core_windows PROPERTY PUBLIC_HEADER
    windows.hpp
  )
  target_link_libraries(coil_core_windows
    PUBLIC
      coil_core_base
  )
  list(APPEND coil_core_libraries windows)
endif()
if(WIN32)
  list(APPEND coil_core_all_libraries windows)
endif()

if(TRUE)
  add_library(coil_core_util INTERFACE)
  set_property(TARGET coil_core_util PROPERTY PUBLIC_HEADER
    util.hpp
  )
  list(APPEND coil_core_libraries util)
endif()
list(APPEND coil_core_all_libraries util)

if(TRUE)
  add_library(coil_core_entrypoint INTERFACE)
  set_property(TARGET coil_core_entrypoint PROPERTY PUBLIC_HEADER
    entrypoint.hpp
  )
  target_link_libraries(coil_core_entrypoint
    INTERFACE
      coil_core_base
  )
  target_link_options(coil_core_entrypoint
    INTERFACE
      $<$<BOOL:${MINGW}>:-municode>
  )
  list(APPEND coil_core_libraries entrypoint)
endif()
list(APPEND coil_core_all_libraries entrypoint)

if(TRUE)
  add_library(coil_core_entrypoint_console STATIC
    entrypoint_console.cpp
  )
  target_link_libraries(coil_core_entrypoint_console
    PRIVATE
      coil_core_entrypoint
      $<$<BOOL:${WIN32}>:coil_core_windows>
  )
  target_link_options(coil_core_entrypoint_console
    INTERFACE
      $<$<BOOL:${WIN32}>:$<IF:$<BOOL:${MSVC}>,/subsystem:console,-mconsole>>
  )
  list(APPEND coil_core_libraries entrypoint_console)
endif()
list(APPEND coil_core_all_libraries entrypoint_console)

if(TRUE)
  add_library(coil_core_entrypoint_graphical STATIC
    entrypoint_graphical.cpp
  )
  target_link_libraries(coil_core_entrypoint_graphical
    PRIVATE
      coil_core_entrypoint
      $<$<BOOL:${WIN32}>:coil_core_windows>
  )
  target_link_options(coil_core_entrypoint_graphical
    INTERFACE
      $<$<BOOL:${WIN32}>:$<IF:$<BOOL:${MSVC}>,/subsystem:windows,-mwindows>>
  )
  list(APPEND coil_core_libraries entrypoint_graphical)
endif()
list(APPEND coil_core_all_libraries entrypoint_graphical)

if(TRUE)
  add_library(coil_core_data STATIC
    data.cpp
  )
  set_property(TARGET coil_core_data PROPERTY PUBLIC_HEADER
    data.hpp
  )
  target_link_libraries(coil_core_data
    PUBLIC
      coil_core_base
  )
  list(APPEND coil_core_libraries data)
endif()
list(APPEND coil_core_all_libraries data)

if(TRUE)
  add_library(coil_core_fs STATIC
    fs.cpp
  )
  set_property(TARGET coil_core_fs PROPERTY PUBLIC_HEADER
    fs.hpp
  )
  target_link_libraries(coil_core_fs
    PUBLIC
      coil_core_base
    PRIVATE
      coil_core_unicode
      $<$<BOOL:${WIN32}>:coil_core_windows>
  )
  list(APPEND coil_core_libraries fs)
endif()
list(APPEND coil_core_all_libraries fs)

if(TRUE)
  add_library(coil_core_math INTERFACE)
  set_property(TARGET coil_core_math PROPERTY PUBLIC_HEADER
    math.hpp
    math_debug.hpp
    math_geometry.hpp
  )
  target_link_libraries(coil_core_math
    INTERFACE
      coil_core_base
  )
  list(APPEND coil_core_libraries math)
endif()
list(APPEND coil_core_all_libraries math)

if(TARGET nlohmann_json::nlohmann_json)
  add_library(coil_core_json INTERFACE)
  set_property(TARGET coil_core_json PROPERTY PUBLIC_HEADER
    json.hpp
  )
  target_link_libraries(coil_core_json
    INTERFACE
      coil_core_math
      nlohmann_json::nlohmann_json
  )
  list(APPEND coil_core_libraries json)
endif()
list(APPEND coil_core_all_libraries json)

if(TRUE)
  add_library(coil_core_time STATIC
    time.cpp
  )
  set_property(TARGET coil_core_time PROPERTY PUBLIC_HEADER
    time.hpp
  )
  target_link_libraries(coil_core_time
    PUBLIC
      coil_core_base
    PRIVATE
      $<$<BOOL:${WIN32}>:coil_core_windows>
  )
  list(APPEND coil_core_libraries time)
endif()
list(APPEND coil_core_all_libraries time)

if(TRUE)
  add_library(coil_core_input STATIC
    input.cpp
    input_debug.cpp
  )
  set_property(TARGET coil_core_input PROPERTY PUBLIC_HEADER
    input.hpp
    input_debug.hpp
    input_keys.hpp
  )
  target_link_libraries(coil_core_input
    PUBLIC
      coil_core_math
    PRIVATE
      coil_core_unicode
  )
  list(APPEND coil_core_libraries input)
endif()
list(APPEND coil_core_all_libraries input)

if(TRUE)
  add_library(coil_core_platform STATIC
    platform.cpp
  )
  set_property(TARGET coil_core_platform PROPERTY PUBLIC_HEADER
    platform.hpp
  )
  target_link_libraries(coil_core_platform
    PUBLIC
      coil_core_input
      coil_core_math
  )
  list(APPEND coil_core_libraries platform)
endif()
list(APPEND coil_core_all_libraries platform)

if(TRUE)
  add_library(coil_core_graphics STATIC
    graphics.cpp
    graphics_format.cpp
    graphics_shaders.cpp
  )
  set_property(TARGET coil_core_graphics PROPERTY PUBLIC_HEADER
    graphics.hpp
    graphics_format.hpp
    graphics_shaders.hpp
  )
  target_link_libraries(coil_core_graphics
    PUBLIC
      coil_core_platform
      coil_core_util
  )
  list(APPEND coil_core_libraries graphics)
endif()
list(APPEND coil_core_all_libraries graphics)

if(TRUE)
  add_library(coil_core_image STATIC
    image.cpp
  )
  set_property(TARGET coil_core_image PROPERTY PUBLIC_HEADER
    image.hpp
  )
  target_link_libraries(coil_core_image
    PUBLIC
      coil_core_math
  )
  list(APPEND coil_core_libraries image)
endif()
list(APPEND coil_core_all_libraries image)

if(TRUE)
  add_library(coil_core_render STATIC
    render.cpp
  )
  set_property(TARGET coil_core_render PROPERTY PUBLIC_HEADER
    render.hpp
  )
  target_link_libraries(coil_core_render
    PUBLIC
      coil_core_graphics
  )
  list(APPEND coil_core_libraries render)
endif()
list(APPEND coil_core_all_libraries render)

if(TARGET SPIRV-Headers::SPIRV-Headers)
  add_library(coil_core_spirv STATIC
    spirv.cpp
  )
  set_property(TARGET coil_core_spirv PROPERTY PUBLIC_HEADER
    spirv.hpp
  )
  target_link_libraries(coil_core_spirv
    PUBLIC
      coil_core_graphics
      SPIRV-Headers::SPIRV-Headers
  )
  list(APPEND coil_core_libraries spirv)
endif()
list(APPEND coil_core_all_libraries spirv)

if(TARGET coil_core_spirv AND TARGET Vulkan::Vulkan)
  add_library(coil_core_vulkan STATIC
    vulkan.cpp
    vulkan_format.cpp
  )
  set_property(TARGET coil_core_vulkan PROPERTY PUBLIC_HEADER
    vulkan.hpp
  )
  target_link_libraries(coil_core_vulkan
    PUBLIC
      coil_core_graphics
      coil_core_spirv
      Vulkan::Vulkan
  )
  list(APPEND coil_core_libraries vulkan)
endif()
list(APPEND coil_core_all_libraries vulkan)

if(TARGET SDL2::SDL2)
  add_library(coil_core_sdl STATIC
    sdl.cpp
  )
  set_property(TARGET coil_core_sdl PROPERTY PUBLIC_HEADER
    sdl.hpp
  )
  target_link_libraries(coil_core_sdl
    PUBLIC
      coil_core_input
      coil_core_platform
      SDL2::SDL2
    PRIVATE
      coil_core_graphics
      coil_core_unicode
  )
  list(APPEND coil_core_libraries sdl)
endif()
list(APPEND coil_core_all_libraries sdl)

if(TARGET coil_core_vulkan AND TARGET coil_core_sdl)
  add_library(coil_core_sdl_vulkan STATIC
    sdl_vulkan.cpp
  )
  set_property(TARGET coil_core_sdl_vulkan PROPERTY PUBLIC_HEADER
    sdl_vulkan.hpp
  )
  target_link_libraries(coil_core_sdl_vulkan
    PRIVATE
      coil_core_vulkan
      coil_core_sdl
  )
  list(APPEND coil_core_libraries sdl_vulkan)
endif()
list(APPEND coil_core_all_libraries sdl_vulkan)

if(TRUE)
  add_library(coil_core_render_math INTERFACE)
  set_property(TARGET coil_core_render_math PROPERTY PUBLIC_HEADER
    render_math.hpp
  )
  target_link_libraries(coil_core_render_math
    INTERFACE
      coil_core_graphics
      coil_core_math
  )
  list(APPEND coil_core_libraries render_math)
endif()
list(APPEND coil_core_all_libraries render_math)

if(TRUE)
  add_library(coil_core_logic INTERFACE)
  set_property(TARGET coil_core_logic PROPERTY PUBLIC_HEADER
    logic_math.hpp
  )
  target_link_libraries(coil_core_logic
    INTERFACE
      coil_core_math
  )
  list(APPEND coil_core_libraries logic)
endif()
list(APPEND coil_core_all_libraries logic)

if(TARGET PNG::PNG)
  add_library(coil_core_image_png STATIC
    image_png.cpp
  )
  set_property(TARGET coil_core_image_png PROPERTY PUBLIC_HEADER
    image_png.hpp
  )
  target_link_libraries(coil_core_image_png
    PUBLIC
      coil_core_graphics
    PRIVATE
      PNG::PNG
  )
  list(APPEND coil_core_libraries image_png)
endif()
list(APPEND coil_core_all_libraries image_png)

if(TARGET zstd::libzstd_shared)
  add_library(coil_core_compress_zstd STATIC
    compress_zstd.cpp
  )
  set_property(TARGET coil_core_compress_zstd PROPERTY PUBLIC_HEADER
    compress_zstd.hpp
  )
  target_link_libraries(coil_core_compress_zstd
    PUBLIC
      coil_core_base
      zstd::libzstd_shared
  )
  list(APPEND coil_core_libraries compress_zstd)
endif()
list(APPEND coil_core_all_libraries compress_zstd)

if(TRUE)
  add_library(coil_core_asset_mesh INTERFACE)
  set_property(TARGET coil_core_asset_mesh PROPERTY PUBLIC_HEADER
    asset_mesh.hpp
  )
  target_link_libraries(coil_core_asset_mesh
    INTERFACE
      coil_core_math
  )
  list(APPEND coil_core_libraries asset_mesh)
endif()
list(APPEND coil_core_all_libraries asset_mesh)

if(TARGET coil_core_json)
  add_library(coil_core_asset_gltf STATIC
    asset_gltf.cpp
  )
  set_property(TARGET coil_core_asset_gltf PROPERTY PUBLIC_HEADER
    asset_gltf.hpp
    asset_gltf_cache.hpp
  )
  target_link_libraries(coil_core_asset_gltf
    PUBLIC
      coil_core_asset_mesh
      coil_core_math
    PRIVATE
      coil_core_data
      coil_core_json
  )
  list(APPEND coil_core_libraries asset_gltf)
endif()
list(APPEND coil_core_all_libraries asset_gltf)

if(TARGET SQLite::SQLite3)
  add_library(coil_core_sqlite STATIC
    sqlite.cpp
  )
  set_property(TARGET coil_core_sqlite PROPERTY PUBLIC_HEADER
    sqlite.hpp
  )
  target_link_libraries(coil_core_sqlite
    PUBLIC
      coil_core_base
      SQLite::SQLite3
  )
  list(APPEND coil_core_libraries sqlite)
endif()
list(APPEND coil_core_all_libraries sqlite)

if(TRUE)
  add_library(coil_core_fonts INTERFACE)
  set_property(TARGET coil_core_fonts PROPERTY PUBLIC_HEADER
    fonts.hpp
  )
  target_link_libraries(coil_core_fonts
    INTERFACE
      coil_core_graphics
      coil_core_image
  )
  list(APPEND coil_core_libraries fonts)
endif()
list(APPEND coil_core_all_libraries fonts)

if(TARGET Freetype::Freetype AND TARGET harfbuzz::harfbuzz)
  add_library(coil_core_fthb STATIC
    fthb.cpp
  )
  set_property(TARGET coil_core_fthb PROPERTY PUBLIC_HEADER
    fthb.hpp
  )
  target_link_libraries(coil_core_fthb
    PUBLIC
      coil_core_fonts
      Freetype::Freetype
      harfbuzz::harfbuzz
  )
  list(APPEND coil_core_libraries fthb)
endif()
list(APPEND coil_core_all_libraries fthb)

if(TARGET PkgConfig::WaylandClient AND TARGET PkgConfig::WaylandProtocols AND TARGET PkgConfig::XkbCommon)
  add_library(coil_core_wayland STATIC
    wayland.cpp
    wayland_keys.cpp
  )
  set_property(TARGET coil_core_wayland PROPERTY PUBLIC_HEADER
    wayland.hpp
  )
  target_link_libraries(coil_core_wayland
    PUBLIC
      coil_core_platform
    PRIVATE
      coil_core_graphics
      PkgConfig::WaylandClient
      PkgConfig::WaylandProtocols
      PkgConfig::XkbCommon
  )
  find_program(WaylandScanner wayland-scanner REQUIRED)
  pkg_get_variable(WaylandProtocols_DataDir wayland-protocols pkgdatadir)
  file(GLOB_RECURSE WaylandProtocols_Xmls "${WaylandProtocols_DataDir}/*.xml")
  foreach(protocol_xml_path ${WaylandProtocols_Xmls})
    cmake_path(GET protocol_xml_path FILENAME protocol_xml)
    cmake_path(REMOVE_EXTENSION protocol_xml OUTPUT_VARIABLE protocol)
    set(protocol_c "${CMAKE_BINARY_DIR}/wayland-${protocol}-protocol.c")
    set(protocol_h "${CMAKE_BINARY_DIR}/wayland-${protocol}-client-protocol.h")
    add_custom_command(OUTPUT ${protocol_c}
      COMMAND ${WaylandScanner} -c private-code ${protocol_xml_path} ${protocol_c}
      DEPENDS ${protocol_xml_path}
    )
    add_custom_command(OUTPUT ${protocol_h}
      COMMAND ${WaylandScanner} -c client-header ${protocol_xml_path} ${protocol_h}
      DEPENDS ${protocol_xml_path}
    )
    target_sources(coil_core_wayland PRIVATE ${protocol_c})
    list(APPEND WaylandProtocols_Outputs ${protocol_c} ${protocol_h})
  endforeach()
  add_custom_target(WaylandProtocolsGenerated DEPENDS ${WaylandProtocols_Outputs})
  add_dependencies(coil_core_wayland WaylandProtocolsGenerated)
  target_include_directories(coil_core_wayland PRIVATE ${CMAKE_BINARY_DIR})
  list(APPEND coil_core_libraries wayland)
endif()
if(UNIX)
  list(APPEND coil_core_all_libraries wayland)
endif()

if(TARGET coil_core_vulkan AND TARGET coil_core_wayland)
  add_library(coil_core_wayland_vulkan STATIC
    wayland_vulkan.cpp
  )
  set_property(TARGET coil_core_wayland_vulkan PROPERTY PUBLIC_HEADER
    wayland_vulkan.hpp
  )
  target_link_libraries(coil_core_wayland_vulkan
    PRIVATE
      coil_core_vulkan
      coil_core_wayland
  )
  list(APPEND coil_core_libraries wayland_vulkan)
endif()
if(UNIX)
  list(APPEND coil_core_all_libraries wayland_vulkan)
endif()

if(TRUE)
  add_library(coil_core_audio STATIC
    audio.cpp
  )
  set_property(TARGET coil_core_audio PROPERTY PUBLIC_HEADER
    audio.hpp
  )
  list(APPEND coil_core_libraries audio)
endif()
list(APPEND coil_core_all_libraries audio)

if(WIN32)
  add_library(coil_core_wincoreaudio STATIC
    wincoreaudio.cpp
  )
  set_property(TARGET coil_core_wincoreaudio PROPERTY PUBLIC_HEADER
    wincoreaudio.hpp
  )
  target_link_libraries(coil_core_wincoreaudio
    PUBLIC
      coil_core_audio
      coil_core_windows
  )
  list(APPEND coil_core_libraries wincoreaudio)
endif()
if(WIN32)
  list(APPEND coil_core_all_libraries wincoreaudio)
endif()

if(TARGET Ogg::ogg)
  add_library(coil_core_ogg STATIC
    ogg.cpp
  )
  set_property(TARGET coil_core_ogg PROPERTY PUBLIC_HEADER
    ogg.hpp
  )
  target_link_libraries(coil_core_ogg
    PUBLIC
      coil_core_base
      Ogg::ogg
  )
  list(APPEND coil_core_libraries ogg)
endif()
list(APPEND coil_core_all_libraries ogg)

if(TARGET coil_core_ogg AND TARGET Opus::opus)
  add_library(coil_core_opus STATIC
    opus.cpp
  )
  set_property(TARGET coil_core_opus PROPERTY PUBLIC_HEADER
    opus.hpp
  )
  target_link_libraries(coil_core_opus
    PUBLIC
      coil_core_audio
      coil_core_ogg
    PRIVATE
      Opus::opus
  )
  list(APPEND coil_core_libraries opus)
endif()
list(APPEND coil_core_all_libraries opus)

if(TARGET Steam::Steam)
  add_library(coil_core_steam STATIC
    steam.cpp
  )
  set_property(TARGET coil_core_steam PROPERTY PUBLIC_HEADER
    steam.hpp
  )
  target_link_libraries(coil_core_steam
    PUBLIC
      coil_core_base
    PRIVATE
      Steam::Steam
  )
  list(APPEND coil_core_libraries steam)
endif()
list(APPEND coil_core_all_libraries steam)


list(TRANSFORM coil_core_libraries PREPEND coil_core_)

# interface
foreach(library ${coil_core_libraries})
  target_include_directories(${library}
    INTERFACE
      $<INSTALL_INTERFACE:include>
  )
endforeach()

# installation
install(TARGETS ${coil_core_libraries}
  EXPORT coil_coreTargets
  ARCHIVE DESTINATION lib
  PUBLIC_HEADER DESTINATION include/coil
)
install(EXPORT coil_coreTargets
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/coil_core"
)

# package config
configure_package_config_file("${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/coil_coreConfig.cmake"
  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/coil_core"
)
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/coil_coreConfig.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/coil_core"
)
install(DIRECTORY
  cmake
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/coil_core"
)

# tests
if(BUILD_TESTING)

  if(TARGET coil_core_json)
    add_executable(test_json
      test_json.cpp
    )
    target_link_libraries(test_json
      coil_core_entrypoint_console
      coil_core_json
      coil_core_unicode
    )
    add_test(NAME test_json COMMAND test_json)
  endif()

  if(TARGET coil_core_compress_zstd)
    add_executable(test_compress_zstd
      test_compress_zstd.cpp
    )
    target_link_libraries(test_compress_zstd
      coil_core_compress_zstd
      coil_core_entrypoint_console
      coil_core_fs
    )
    add_test(NAME test_compress_zstd COMMAND test_compress_zstd)
  endif()

  if(TARGET coil_core_sqlite)
    add_executable(test_sqlite
      test_sqlite.cpp
    )
    target_link_libraries(test_sqlite
      coil_core_entrypoint_console
      coil_core_sqlite
    )
    add_test(NAME test_sqlite COMMAND test_sqlite)
  endif()

  if(TARGET coil_core_steam)
    add_executable(test_steam
      test_steam.cpp
    )
    target_link_libraries(test_steam
      coil_core_entrypoint_console
      coil_core_steam
    )
    add_test(NAME test_steam COMMAND test_steam)
  endif()

endif() # BUILD_TESTING

set(COIL_CORE_REQUIRE_LIBS ${coil_core_all_libraries} CACHE STRING "list of libs required to build")

foreach(library ${COIL_CORE_REQUIRE_LIBS})
  if(NOT TARGET coil_core_${library})
    message(SEND_ERROR "required library coil_core_${library} is not set up")
  endif()
endforeach()
