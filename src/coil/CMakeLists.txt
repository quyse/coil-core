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

include(deps.cmake)

add_library(coil_core_base STATIC
  base.cpp
)
set_property(TARGET coil_core_base PROPERTY PUBLIC_HEADER
  base.hpp
  base_debug.hpp
)
list(APPEND coil_core_libraries coil_core_base)

add_library(coil_core_unicode INTERFACE)
set_property(TARGET coil_core_unicode PROPERTY PUBLIC_HEADER
  unicode.hpp
)
list(APPEND coil_core_libraries coil_core_unicode)

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
list(APPEND coil_core_libraries coil_core_windows)
endif()

add_library(coil_core_util INTERFACE)
set_property(TARGET coil_core_util PROPERTY PUBLIC_HEADER
  util.hpp
)
list(APPEND coil_core_libraries coil_core_util)

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
list(APPEND coil_core_libraries coil_core_entrypoint)

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
list(APPEND coil_core_libraries coil_core_entrypoint_console)

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
list(APPEND coil_core_libraries coil_core_entrypoint_graphical)

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
list(APPEND coil_core_libraries coil_core_data)

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
list(APPEND coil_core_libraries coil_core_fs)

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
list(APPEND coil_core_libraries coil_core_math)

add_library(coil_core_json INTERFACE)
set_property(TARGET coil_core_json PROPERTY PUBLIC_HEADER
  json.hpp
)
target_link_libraries(coil_core_json
  INTERFACE
    coil_core_math
    nlohmann_json::nlohmann_json
)
list(APPEND coil_core_libraries coil_core_json)

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
list(APPEND coil_core_libraries coil_core_time)

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
list(APPEND coil_core_libraries coil_core_input)

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
list(APPEND coil_core_libraries coil_core_platform)

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
list(APPEND coil_core_libraries coil_core_graphics)

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
list(APPEND coil_core_libraries coil_core_image)

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
list(APPEND coil_core_libraries coil_core_render)

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
list(APPEND coil_core_libraries coil_core_spirv)

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
list(APPEND coil_core_libraries coil_core_vulkan)

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
list(APPEND coil_core_libraries coil_core_sdl)

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
list(APPEND coil_core_libraries coil_core_sdl_vulkan)

add_library(coil_core_render_math INTERFACE)
set_property(TARGET coil_core_render_math PROPERTY PUBLIC_HEADER
  render_math.hpp
)
target_link_libraries(coil_core_render_math
  INTERFACE
    coil_core_graphics
    coil_core_math
)
list(APPEND coil_core_libraries coil_core_render_math)

add_library(coil_core_logic INTERFACE)
set_property(TARGET coil_core_logic PROPERTY PUBLIC_HEADER
  logic_math.hpp
)
target_link_libraries(coil_core_logic
  INTERFACE
    coil_core_math
)
list(APPEND coil_core_libraries coil_core_logic)

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
list(APPEND coil_core_libraries coil_core_image_png)

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
list(APPEND coil_core_libraries coil_core_compress_zstd)

add_library(coil_core_asset_mesh INTERFACE)
set_property(TARGET coil_core_asset_mesh PROPERTY PUBLIC_HEADER
  asset_mesh.hpp
)
target_link_libraries(coil_core_asset_mesh
  INTERFACE
    coil_core_math
)
list(APPEND coil_core_libraries coil_core_asset_mesh)

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
list(APPEND coil_core_libraries coil_core_asset_gltf)

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
list(APPEND coil_core_libraries coil_core_sqlite)

add_library(coil_core_fonts INTERFACE)
set_property(TARGET coil_core_fonts PROPERTY PUBLIC_HEADER
  fonts.hpp
)
target_link_libraries(coil_core_fonts
  INTERFACE
    coil_core_graphics
    coil_core_image
)
list(APPEND coil_core_libraries coil_core_fonts)

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
list(APPEND coil_core_libraries coil_core_fthb)

if(UNIX)
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
list(APPEND coil_core_libraries coil_core_wayland)
endif()

if(UNIX)
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
list(APPEND coil_core_libraries coil_core_wayland_vulkan)
endif()

add_library(coil_core_audio STATIC
  audio.cpp
)
set_property(TARGET coil_core_audio PROPERTY PUBLIC_HEADER
  audio.hpp
)
# target_link_libraries(coil_core_audio
#   PUBLIC
# )
list(APPEND coil_core_libraries coil_core_audio)

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
list(APPEND coil_core_libraries coil_core_wincoreaudio)
endif()

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
list(APPEND coil_core_libraries coil_core_ogg)

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
list(APPEND coil_core_libraries coil_core_opus)


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
  deps.cmake
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/coil_core"
)

# tests
if(BUILD_TESTING)

add_executable(test_json
  test_json.cpp
)
target_link_libraries(test_json
  coil_core_entrypoint_console
  coil_core_json
  coil_core_unicode
)
add_test(NAME test_json COMMAND test_json)

add_executable(test_compress_zstd
  test_compress_zstd.cpp
)
target_link_libraries(test_compress_zstd
  coil_core_compress_zstd
  coil_core_entrypoint_console
  coil_core_fs
)
add_test(NAME test_compress_zstd COMMAND test_compress_zstd)

add_executable(test_sqlite
  test_sqlite.cpp
)
target_link_libraries(test_sqlite
  coil_core_entrypoint_console
  coil_core_sqlite
)
add_test(NAME test_sqlite COMMAND test_sqlite)

endif() # BUILD_TESTING
