cmake_minimum_required(VERSION 3.19)

project(coil_core VERSION 3.0)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

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

add_library(coil_core_unicode INTERFACE)
set_property(TARGET coil_core_unicode PROPERTY PUBLIC_HEADER
  unicode.hpp
)

add_library(coil_core_windows INTERFACE)
set_property(TARGET coil_core_windows PROPERTY PUBLIC_HEADER
  windows.hpp
)

add_library(coil_core_util INTERFACE)
set_property(TARGET coil_core_util PROPERTY PUBLIC_HEADER
  util.hpp
)

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
    $<$<BOOL:${WIN32}>:-municode>
)

add_library(coil_core_entrypoint_console STATIC
  entrypoint_console.cpp
)
target_link_libraries(coil_core_entrypoint_console
  PRIVATE
    coil_core_entrypoint
    coil_core_windows
)
target_link_options(coil_core_entrypoint_console
  INTERFACE
    $<$<BOOL:${WIN32}>:-mconsole>
)

add_library(coil_core_entrypoint_graphical STATIC
  entrypoint_graphical.cpp
)
target_link_libraries(coil_core_entrypoint_graphical
  PRIVATE
    coil_core_entrypoint
    coil_core_windows
)
target_link_options(coil_core_entrypoint_graphical
  INTERFACE
    $<$<BOOL:${WIN32}>:-mwindows>
)

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
    coil_core_windows
)

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

add_library(coil_core_json INTERFACE)
set_property(TARGET coil_core_json PROPERTY PUBLIC_HEADER
  json.hpp
)
target_link_libraries(coil_core_json
  INTERFACE
    coil_core_math
    PkgConfig::nlohmann_json
)

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
    coil_core_windows
)

add_library(coil_core_input STATIC
  input.cpp
  input_debug.cpp
)
set_property(TARGET coil_core_input PROPERTY PUBLIC_HEADER
  input.hpp
  input_debug.hpp
)
target_link_libraries(coil_core_input
  PUBLIC
    coil_core_base
  PRIVATE
    coil_core_unicode
)

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

add_library(coil_core_graphics_image STATIC
  graphics_image.cpp
)
set_property(TARGET coil_core_graphics_image PROPERTY PUBLIC_HEADER
  graphics_image.hpp
)
target_link_libraries(coil_core_graphics_image
  PUBLIC
    coil_core_math
)

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

add_library(coil_core_spirv STATIC
  spirv.cpp
)
set_property(TARGET coil_core_spirv PROPERTY PUBLIC_HEADER
  spirv.hpp
)
target_link_libraries(coil_core_spirv
  PUBLIC
    coil_core_graphics
    PkgConfig::SPIRV-Headers
)

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
    PkgConfig::Vulkan
)

add_library(coil_core_sdl STATIC
  sdl.cpp
)
set_property(TARGET coil_core_sdl PROPERTY PUBLIC_HEADER
  sdl.hpp
)
target_link_libraries(coil_core_sdl
  PUBLIC
    coil_core_graphics
    coil_core_input
    coil_core_platform
    SDL2::SDL2
  PRIVATE
    coil_core_unicode
)

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
    SDL2::SDL2
)

add_library(coil_core_render_math INTERFACE)
set_property(TARGET coil_core_render_math PROPERTY PUBLIC_HEADER
  render_math.hpp
)
target_link_libraries(coil_core_render_math
  INTERFACE
    coil_core_graphics
    coil_core_math
)

add_library(coil_core_logic INTERFACE)
set_property(TARGET coil_core_logic PROPERTY PUBLIC_HEADER
  logic_math.hpp
)
target_link_libraries(coil_core_logic
  INTERFACE
    coil_core_math
)

add_library(coil_core_graphics_image_png STATIC
  graphics_image_png.cpp
)
set_property(TARGET coil_core_graphics_image_png PROPERTY PUBLIC_HEADER
  graphics_image_png.hpp
)
target_link_libraries(coil_core_graphics_image_png
  PUBLIC
    coil_core_graphics
    PNG::PNG
)

add_library(coil_core_compress_zstd STATIC
  compress_zstd.cpp
)
set_property(TARGET coil_core_compress_zstd PROPERTY PUBLIC_HEADER
  compress_zstd.hpp
)
target_link_libraries(coil_core_compress_zstd
  PUBLIC
    coil_core_base
    PkgConfig::Zstd
)

add_library(coil_core_asset_mesh INTERFACE)
set_property(TARGET coil_core_asset_mesh PROPERTY PUBLIC_HEADER
  asset_mesh.hpp
)
target_link_libraries(coil_core_asset_mesh
  INTERFACE
    coil_core_math
)

add_library(coil_core_asset_gltf STATIC
  asset_gltf.cpp
)
set_property(TARGET coil_core_asset_gltf PROPERTY PUBLIC_HEADER
  asset_gltf.hpp
  asset_gltf_cache.hpp
)
target_link_libraries(coil_core_asset_gltf
  PUBLIC
    coil_core_data
    coil_core_json
    coil_core_math
)

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

add_library(coil_core_fonts INTERFACE)
set_property(TARGET coil_core_fonts PROPERTY PUBLIC_HEADER
  fonts.hpp
)
target_link_libraries(coil_core_fonts
  INTERFACE
    coil_core_graphics
    coil_core_graphics_image
)

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
    PkgConfig::Harfbuzz
)

set(coil_core_libraries
  coil_core_asset_gltf
  coil_core_asset_mesh
  coil_core_base
  coil_core_compress_zstd
  coil_core_data
  coil_core_entrypoint
  coil_core_entrypoint_console
  coil_core_entrypoint_graphical
  coil_core_fonts
  coil_core_fs
  coil_core_fthb
  coil_core_graphics
  coil_core_graphics_image
  coil_core_graphics_image_png
  coil_core_input
  coil_core_json
  coil_core_logic
  coil_core_math
  coil_core_platform
  coil_core_render
  coil_core_render_math
  coil_core_sdl
  coil_core_sdl_vulkan
  coil_core_spirv
  coil_core_sqlite
  coil_core_time
  coil_core_unicode
  coil_core_util
  coil_core_vulkan
  coil_core_windows
)

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

enable_testing()

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
