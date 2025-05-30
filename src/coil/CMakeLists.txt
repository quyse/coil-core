cmake_minimum_required(VERSION 3.28.2)

project(coil_core VERSION 3.0)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CTest)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(cmake/deps.cmake)


if(TRUE)
  add_library(coil_core_base STATIC)
  target_sources(coil_core_base PUBLIC FILE_SET CXX_MODULES FILES
    base.cppm
    base_debug.cppm
    base_events.cppm
    base_events_map.cppm
    base_generator.cppm
    base_signals.cppm
    base_util.cppm
  )
  target_sources(coil_core_base PUBLIC FILE_SET HEADERS FILES
    base.hpp
    base_meta.hpp
  )
  target_compile_features(coil_core_base PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries base)
endif()
list(APPEND coil_core_all_libraries base)

if(TRUE)
  add_library(coil_core_unicode STATIC)
  target_sources(coil_core_unicode PUBLIC FILE_SET CXX_MODULES FILES
    unicode.cppm
  )
  target_compile_features(coil_core_unicode PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries unicode)
endif()
list(APPEND coil_core_all_libraries unicode)

if(WIN32)
  add_library(coil_core_windows STATIC)
  target_sources(coil_core_windows PUBLIC FILE_SET CXX_MODULES FILES
    windows.cppm
  )
  target_sources(coil_core_windows PUBLIC FILE_SET HEADERS FILES
    windows.hpp
  )
  target_link_libraries(coil_core_windows
    PUBLIC
      coil_core_base
  )
  target_compile_features(coil_core_windows PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries windows)
endif()
if(WIN32)
  list(APPEND coil_core_all_libraries windows)
endif()

if(TRUE)
  add_library(coil_core_entrypoint INTERFACE)
  target_sources(coil_core_entrypoint PUBLIC FILE_SET HEADERS FILES
    entrypoint.hpp
  )
  target_link_options(coil_core_entrypoint
    INTERFACE
      $<$<BOOL:${MINGW}>:-municode>
  )
  list(APPEND coil_core_libraries entrypoint)
endif()
list(APPEND coil_core_all_libraries entrypoint)

if(TRUE)
  add_library(coil_core_entrypoint_console STATIC)
  target_sources(coil_core_entrypoint_console PUBLIC FILE_SET CXX_MODULES FILES
    entrypoint_console.cppm
  )
  target_link_libraries(coil_core_entrypoint_console
    PRIVATE
      coil_core_base
      coil_core_entrypoint
      $<$<BOOL:${WIN32}>:coil_core_windows>
  )
  target_link_options(coil_core_entrypoint_console
    INTERFACE
      $<$<BOOL:${WIN32}>:$<IF:$<BOOL:${MSVC}>,/subsystem:console,-mconsole>>
  )
  target_compile_features(coil_core_entrypoint_console PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries entrypoint_console)
endif()
list(APPEND coil_core_all_libraries entrypoint_console)

if(TRUE)
  add_library(coil_core_entrypoint_graphical STATIC)
  target_sources(coil_core_entrypoint_graphical PUBLIC FILE_SET CXX_MODULES FILES
    entrypoint_graphical.cppm
  )
  target_link_libraries(coil_core_entrypoint_graphical
    PRIVATE
      coil_core_base
      coil_core_entrypoint
      $<$<BOOL:${WIN32}>:coil_core_windows>
  )
  target_link_options(coil_core_entrypoint_graphical
    INTERFACE
      $<$<BOOL:${WIN32}>:$<IF:$<BOOL:${MSVC}>,/subsystem:windows,-mwindows>>
  )
  target_compile_features(coil_core_entrypoint_graphical PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries entrypoint_graphical)
endif()
list(APPEND coil_core_all_libraries entrypoint_graphical)

if(TRUE)
  add_library(coil_core_appidentity STATIC)
  target_sources(coil_core_appidentity PUBLIC FILE_SET CXX_MODULES FILES
    appidentity.cppm
  )
  target_compile_features(coil_core_appidentity PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries appidentity)
endif()
list(APPEND coil_core_all_libraries appidentity)

if(TRUE)
  add_library(coil_core_data STATIC)
  target_sources(coil_core_data PUBLIC FILE_SET CXX_MODULES FILES
    data.cppm
  )
  target_link_libraries(coil_core_data
    PUBLIC
      coil_core_base
  )
  target_compile_features(coil_core_data PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries data)
endif()
list(APPEND coil_core_all_libraries data)

if(TRUE)
  add_library(coil_core_assets STATIC)
  target_sources(coil_core_assets PUBLIC FILE_SET CXX_MODULES FILES
    assets.cppm
    assets_structs.cppm
  )
  target_link_libraries(coil_core_assets
    PUBLIC
      coil_core_json
      coil_core_tasks
  )
  target_compile_features(coil_core_assets PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries assets)
endif()
list(APPEND coil_core_all_libraries assets)

if(TRUE)
  add_library(coil_core_fs STATIC)
  target_sources(coil_core_fs PUBLIC FILE_SET CXX_MODULES FILES
    fs.cppm
  )
  target_link_libraries(coil_core_fs
    PUBLIC
      coil_core_data
      coil_core_tasks
      coil_core_unicode
      $<$<BOOL:${WIN32}>:coil_core_windows>
  )
  target_compile_features(coil_core_fs PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries fs)
endif()
list(APPEND coil_core_all_libraries fs)

if(TARGET coil_core_appidentity)
  add_library(coil_core_process STATIC)
  target_sources(coil_core_process PUBLIC FILE_SET CXX_MODULES FILES
    process.cppm
  )
  target_link_libraries(coil_core_process
    PUBLIC
      coil_core_appidentity
      coil_core_fs
      $<$<BOOL:${WIN32}>:coil_core_unicode>
      $<$<BOOL:${WIN32}>:coil_core_windows>
  )
  target_compile_features(coil_core_process PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries process)
endif()
list(APPEND coil_core_all_libraries process)

if(TRUE)
  add_library(coil_core_math STATIC)
  target_sources(coil_core_math PUBLIC FILE_SET CXX_MODULES FILES
    math.cppm
    math_debug.cppm
    math_geometry.cppm
  )
  target_compile_features(coil_core_math PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries math)
endif()
list(APPEND coil_core_all_libraries math)

if(TRUE)
  add_library(coil_core_math_determ STATIC)
  target_sources(coil_core_math_determ PUBLIC FILE_SET CXX_MODULES FILES
    math_determ.cppm
    math_determ_debug.cppm
  )
  target_compile_options(coil_core_math_determ
    PRIVATE
      "$<IF:$<BOOL:${MSVC}>,/fp:strict,-ffp-model=strict>"
  )
  target_link_libraries(coil_core_math_determ
    PUBLIC
      coil_core_math
  )
  target_compile_features(coil_core_math_determ PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries math_determ)
endif()
list(APPEND coil_core_all_libraries math_determ)

if(TRUE)
  add_library(coil_core_tasks STATIC)
  target_sources(coil_core_tasks PUBLIC FILE_SET CXX_MODULES FILES
    tasks.cppm
    tasks_storage.cppm
    tasks_streams.cppm
    tasks_sync.cppm
  )
  target_link_libraries(coil_core_tasks
    PUBLIC
      coil_core_base
      coil_core_data
  )
  target_compile_features(coil_core_tasks PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries tasks)
endif()
list(APPEND coil_core_all_libraries tasks)

if(TARGET nlohmann_json::nlohmann_json)
  add_library(coil_core_json STATIC)
  target_sources(coil_core_json PUBLIC FILE_SET CXX_MODULES FILES
    json.cppm
  )
  target_sources(coil_core_json PUBLIC FILE_SET HEADERS FILES
    json.hpp
  )
  target_link_libraries(coil_core_json
    PUBLIC
      coil_core_base
      coil_core_math
      nlohmann_json::nlohmann_json
  )
  target_compile_features(coil_core_json PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries json)
endif()
list(APPEND coil_core_all_libraries json)

if(TRUE)
  add_library(coil_core_time STATIC)
  target_sources(coil_core_time PUBLIC FILE_SET CXX_MODULES FILES
    time.cppm
  )
  target_link_libraries(coil_core_time
    PUBLIC
      coil_core_base
      $<$<BOOL:${WIN32}>:coil_core_windows>
  )
  target_compile_features(coil_core_time PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries time)
endif()
list(APPEND coil_core_all_libraries time)

if(TRUE)
  add_library(coil_core_text STATIC)
  target_sources(coil_core_text PUBLIC FILE_SET CXX_MODULES FILES
    text.cppm
  )
  target_link_libraries(coil_core_text
    PUBLIC
      coil_core_base
  )
  target_compile_features(coil_core_text PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries text)
endif()
list(APPEND coil_core_all_libraries text)

if(TRUE)
  add_library(coil_core_localization STATIC)
  target_sources(coil_core_localization PUBLIC FILE_SET CXX_MODULES FILES
    localization.cppm
  )
  target_link_libraries(coil_core_localization
    PUBLIC
      coil_core_text
  )
  target_compile_features(coil_core_localization PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries localization)
endif()
list(APPEND coil_core_all_libraries localization)

if(TRUE)
  add_library(coil_core_input STATIC)
  target_sources(coil_core_input PUBLIC FILE_SET CXX_MODULES FILES
    input.cppm
    input_debug.cppm
  )
  target_sources(coil_core_input PUBLIC FILE_SET HEADERS FILES
    input_keys.hpp
  )
  target_link_libraries(coil_core_input
    PUBLIC
      coil_core_base
      coil_core_math
      coil_core_unicode
  )
  target_compile_features(coil_core_input PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries input)
endif()
list(APPEND coil_core_all_libraries input)

if(TRUE)
  add_library(coil_core_player_input STATIC)
  target_sources(coil_core_player_input PUBLIC FILE_SET CXX_MODULES FILES
    player_input.cppm
    player_input_combined.cppm
  )
  target_link_libraries(coil_core_player_input
    PUBLIC
      coil_core_base
      coil_core_math
  )
  target_compile_features(coil_core_player_input PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries player_input)
endif()
list(APPEND coil_core_all_libraries player_input)

if(TRUE)
  add_library(coil_core_player_input_native STATIC)
  target_sources(coil_core_player_input_native PUBLIC FILE_SET CXX_MODULES FILES
    player_input_native.cppm
  )
  target_link_libraries(coil_core_player_input_native
    PUBLIC
      coil_core_input
      coil_core_json
      coil_core_player_input
  )
  target_compile_features(coil_core_player_input_native PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries player_input_native)
endif()
list(APPEND coil_core_all_libraries player_input_native)

if(TRUE)
  add_library(coil_core_platform STATIC)
  target_sources(coil_core_platform PUBLIC FILE_SET CXX_MODULES FILES
    platform.cppm
  )
  target_link_libraries(coil_core_platform
    PUBLIC
      coil_core_base
      coil_core_graphics
      coil_core_input
      coil_core_math
  )
  target_compile_features(coil_core_platform PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries platform)
endif()
list(APPEND coil_core_all_libraries platform)

if(TRUE)
  add_library(coil_core_graphics STATIC)
  target_sources(coil_core_graphics PUBLIC FILE_SET CXX_MODULES FILES
    graphics.cppm
    graphics_format.cppm
    graphics_shaders.cppm
  )
  target_link_libraries(coil_core_graphics
    PUBLIC
      coil_core_base
      coil_core_image
      coil_core_math
      coil_core_tasks
  )
  target_compile_features(coil_core_graphics PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries graphics)
endif()
list(APPEND coil_core_all_libraries graphics)

if(TRUE)
  add_library(coil_core_image STATIC)
  target_sources(coil_core_image PUBLIC FILE_SET CXX_MODULES FILES
    image.cppm
    image_format.cppm
  )
  target_link_libraries(coil_core_image
    PUBLIC
      coil_core_base
      coil_core_math
  )
  target_compile_features(coil_core_image PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries image)
endif()
list(APPEND coil_core_all_libraries image)

if(TRUE)
  add_library(coil_core_image_transform STATIC)
  target_sources(coil_core_image_transform PUBLIC FILE_SET CXX_MODULES FILES
    image_transform.cppm
  )
  target_link_libraries(coil_core_image_transform
    PUBLIC
      coil_core_image
      coil_core_tasks
  )
  target_compile_features(coil_core_image_transform PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries image_transform)
endif()
list(APPEND coil_core_all_libraries image_transform)

if(TRUE)
  add_library(coil_core_scene STATIC)
  target_sources(coil_core_scene PUBLIC FILE_SET CXX_MODULES FILES
    scene.cppm
  )
  target_link_libraries(coil_core_scene
    PUBLIC
      coil_core_base
  )
  target_compile_features(coil_core_scene PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries scene)
endif()
list(APPEND coil_core_all_libraries scene)

if(TRUE)
  add_library(coil_core_render STATIC)
  target_sources(coil_core_render PUBLIC FILE_SET CXX_MODULES FILES
    render.cppm
    render_canvas.cppm
    render_math.cppm
    render_polygons.cppm
  )
  target_link_libraries(coil_core_render
    PUBLIC
      coil_core_graphics
  )
  target_compile_features(coil_core_render PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries render)
endif()
list(APPEND coil_core_all_libraries render)

if(TRUE)
  add_library(coil_core_render_realistic STATIC
    render_realistic_opaque.cpp
    render_realistic_screenspace.cpp
  )
  target_sources(coil_core_render_realistic PUBLIC FILE_SET CXX_MODULES FILES
    render_realistic.cppm
    render_realistic_formats.cppm
  )
  target_link_libraries(coil_core_render_realistic
    PUBLIC
      coil_core_graphics
      coil_core_render
  )
  target_compile_features(coil_core_render_realistic PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries render_realistic)
endif()
list(APPEND coil_core_all_libraries render_realistic)

if(TARGET SPIRV-Headers::SPIRV-Headers)
  add_library(coil_core_spirv STATIC)
  target_sources(coil_core_spirv PUBLIC FILE_SET CXX_MODULES FILES
    spirv.cppm
  )
  target_link_libraries(coil_core_spirv
    PUBLIC
      coil_core_graphics
      SPIRV-Headers::SPIRV-Headers
  )
  target_compile_features(coil_core_spirv PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries spirv)
endif()
list(APPEND coil_core_all_libraries spirv)

if(TARGET coil_core_spirv AND TARGET Vulkan::Vulkan)
  add_library(coil_core_vulkan STATIC
    vulkan_computer.cpp
    vulkan_context.cpp
    vulkan_device.cpp
    vulkan_format.cpp
    vulkan_frame.cpp
    vulkan_pool.cpp
    vulkan_presenter.cpp
    vulkan_system.cpp
  )
  target_sources(coil_core_vulkan PUBLIC FILE_SET CXX_MODULES FILES
    vulkan.cppm
    vulkan_utils.cppm
  )
  target_link_libraries(coil_core_vulkan
    PUBLIC
      coil_core_appidentity
      coil_core_base
      coil_core_graphics
      coil_core_spirv
      Vulkan::Vulkan
  )
  target_compile_features(coil_core_vulkan PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries vulkan)
endif()
list(APPEND coil_core_all_libraries vulkan)

if(TARGET SDL3::SDL3)
  add_library(coil_core_sdl STATIC)
  target_sources(coil_core_sdl PUBLIC FILE_SET CXX_MODULES FILES
    sdl.cppm
  )
  target_link_libraries(coil_core_sdl
    PUBLIC
      coil_core_input
      coil_core_platform
      coil_core_unicode
      SDL3::SDL3
  )
  target_compile_features(coil_core_sdl PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries sdl)
endif()
list(APPEND coil_core_all_libraries sdl)

if(TARGET coil_core_vulkan AND TARGET coil_core_sdl)
  add_library(coil_core_sdl_vulkan STATIC)
  target_sources(coil_core_sdl_vulkan PUBLIC FILE_SET CXX_MODULES FILES
    sdl_vulkan.cppm
  )
  target_link_libraries(coil_core_sdl_vulkan
    PUBLIC
      coil_core_vulkan
      coil_core_sdl
  )
  target_compile_features(coil_core_sdl_vulkan PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries sdl_vulkan)
endif()
list(APPEND coil_core_all_libraries sdl_vulkan)

if(TRUE)
  add_library(coil_core_logic STATIC)
  target_sources(coil_core_logic PUBLIC FILE_SET CXX_MODULES FILES
    logic_math.cppm
  )
  target_link_libraries(coil_core_logic
    PUBLIC
      coil_core_math
  )
  target_compile_features(coil_core_logic PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries logic)
endif()
list(APPEND coil_core_all_libraries logic)

if(TARGET PNG::PNG)
  add_library(coil_core_image_png STATIC)
  target_sources(coil_core_image_png PUBLIC FILE_SET CXX_MODULES FILES
    image_png.cppm
  )
  target_link_libraries(coil_core_image_png
    PUBLIC
      coil_core_data
      coil_core_image
      coil_core_tasks
      PNG::PNG
  )
  target_compile_features(coil_core_image_png PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries image_png)
endif()
list(APPEND coil_core_all_libraries image_png)

if(TARGET Squish::Squish)
  add_library(coil_core_image_compress STATIC)
  target_sources(coil_core_image_compress PUBLIC FILE_SET CXX_MODULES FILES
    image_compress.cppm
  )
  target_link_libraries(coil_core_image_compress
    PUBLIC
      coil_core_image
      coil_core_tasks
    PRIVATE
      Squish::Squish
  )
  target_compile_features(coil_core_image_compress PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries image_compress)
endif()
list(APPEND coil_core_all_libraries image_compress)

if(TARGET zstd::libzstd_shared)
  add_library(coil_core_compress_zstd STATIC)
  target_sources(coil_core_compress_zstd PUBLIC FILE_SET CXX_MODULES FILES
    compress_zstd.cppm
  )
  target_link_libraries(coil_core_compress_zstd
    PUBLIC
      coil_core_tasks
      zstd::libzstd_shared
  )
  target_compile_features(coil_core_compress_zstd PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries compress_zstd)
endif()
list(APPEND coil_core_all_libraries compress_zstd)

if(TRUE)
  add_library(coil_core_mesh STATIC)
  target_sources(coil_core_mesh PUBLIC FILE_SET CXX_MODULES FILES
    mesh.cppm
  )
  target_link_libraries(coil_core_mesh
    PUBLIC
      coil_core_math
  )
  target_compile_features(coil_core_mesh PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries mesh)
endif()
list(APPEND coil_core_all_libraries mesh)

if(TARGET coil_core_json)
  add_library(coil_core_gltf STATIC)
  target_sources(coil_core_gltf PUBLIC FILE_SET CXX_MODULES FILES
    gltf.cppm
    gltf_cache.cppm
  )
  target_link_libraries(coil_core_gltf
    PUBLIC
      coil_core_math
      coil_core_mesh
      coil_core_data
      coil_core_json
  )
  target_compile_features(coil_core_gltf PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries gltf)
endif()
list(APPEND coil_core_all_libraries gltf)

if(TARGET SQLite::SQLite3)
  add_library(coil_core_sqlite STATIC)
  target_sources(coil_core_sqlite PUBLIC FILE_SET CXX_MODULES FILES
    sqlite.cppm
  )
  target_link_libraries(coil_core_sqlite
    PUBLIC
      coil_core_base
      SQLite::SQLite3
  )
  target_compile_features(coil_core_sqlite PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries sqlite)
endif()
list(APPEND coil_core_all_libraries sqlite)

if(TARGET MbedTLS::mbedcrypto)
  add_library(coil_core_crypto STATIC)
  target_sources(coil_core_crypto PUBLIC FILE_SET CXX_MODULES FILES
    crypto.cppm
    crypto_base.cppm
    crypto_mbedtls.cppm
  )
  target_link_libraries(coil_core_crypto
    PUBLIC
      coil_core_base
      MbedTLS::mbedcrypto
  )
  target_compile_features(coil_core_crypto PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries crypto)
endif()
list(APPEND coil_core_all_libraries crypto)

if(TRUE)
  add_library(coil_core_fonts STATIC)
  target_sources(coil_core_fonts PUBLIC FILE_SET CXX_MODULES FILES
    fonts.cppm
    fonts_cache.cppm
  )
  target_link_libraries(coil_core_fonts
    PUBLIC
      coil_core_image
      coil_core_localization
  )
  target_compile_features(coil_core_fonts PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries fonts)
endif()
list(APPEND coil_core_all_libraries fonts)

if(TARGET coil_core_fonts)
  add_library(coil_core_render_fonts STATIC)
  target_sources(coil_core_render_fonts PUBLIC FILE_SET CXX_MODULES FILES
    render_fonts.cppm
  )
  target_link_libraries(coil_core_render_fonts
    PUBLIC
      coil_core_fonts
      coil_core_render
  )
  target_compile_features(coil_core_render_fonts PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries render_fonts)
endif()
list(APPEND coil_core_all_libraries render_fonts)

if(TARGET Freetype::Freetype AND TARGET harfbuzz::harfbuzz)
  add_library(coil_core_fthb STATIC)
  target_sources(coil_core_fthb PUBLIC FILE_SET CXX_MODULES FILES
    fthb.cppm
  )
  target_link_libraries(coil_core_fthb
    PUBLIC
      coil_core_fonts
      coil_core_tasks
      Freetype::Freetype
      harfbuzz::harfbuzz
  )
  target_compile_features(coil_core_fthb PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries fthb)
endif()
list(APPEND coil_core_all_libraries fthb)

if(TARGET PkgConfig::WaylandClient AND TARGET PkgConfig::WaylandProtocols AND TARGET PkgConfig::XkbCommon)
  add_library(coil_core_wayland_protocols STATIC)
  find_program(WaylandScanner wayland-scanner REQUIRED)
  pkg_get_variable(WaylandProtocols_DataDir wayland-protocols pkgdatadir)
  file(GLOB_RECURSE WaylandProtocols_Xmls "${WaylandProtocols_DataDir}/*.xml")
  if(TARGET PkgConfig::WlrProtocols)
    pkg_get_variable(WlrProtocols_DataDir wlr-protocols pkgdatadir)
    file(GLOB_RECURSE WlrProtocols_Xmls "${WlrProtocols_DataDir}/*.xml")
  endif()
  foreach(protocol_xml_path ${WaylandProtocols_Xmls} ${WlrProtocols_Xmls})
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
    target_sources(coil_core_wayland_protocols PRIVATE ${protocol_c})
    set_property(TARGET coil_core_wayland_protocols APPEND PROPERTY PUBLIC_HEADER ${protocol_h})
    list(APPEND WaylandProtocols_Outputs ${protocol_c} ${protocol_h})
  endforeach()
  add_custom_target(WaylandProtocolsGenerated DEPENDS ${WaylandProtocols_Outputs})
  add_dependencies(coil_core_wayland_protocols WaylandProtocolsGenerated)
  # cannot use build dir in interface
  # target_include_directories(coil_core_wayland_protocols PUBLIC ${CMAKE_BINARY_DIR})
  list(APPEND coil_core_libraries wayland_protocols)
endif()
if(UNIX)
  list(APPEND coil_core_all_libraries wayland_protocols)
endif()

if(TARGET coil_core_wayland_protocols)
  add_library(coil_core_wayland STATIC
    wayland_keys.cpp
  )
  target_sources(coil_core_wayland PUBLIC FILE_SET CXX_MODULES FILES
    wayland.cppm
  )
  target_link_libraries(coil_core_wayland
    PUBLIC
      coil_core_graphics
      coil_core_platform
      coil_core_wayland_protocols
      PkgConfig::WaylandClient
      PkgConfig::XkbCommon
  )
  target_include_directories(coil_core_wayland PRIVATE ${CMAKE_BINARY_DIR})
  target_compile_features(coil_core_wayland PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries wayland)
endif()
if(UNIX)
  list(APPEND coil_core_all_libraries wayland)
endif()

if(TARGET coil_core_vulkan AND TARGET coil_core_wayland)
  add_library(coil_core_wayland_vulkan STATIC)
  target_sources(coil_core_wayland_vulkan PUBLIC FILE_SET CXX_MODULES FILES
    wayland_vulkan.cppm
  )
  target_link_libraries(coil_core_wayland_vulkan
    PRIVATE
      coil_core_vulkan
      coil_core_wayland
  )
  target_compile_features(coil_core_wayland_vulkan PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries wayland_vulkan)
endif()
if(UNIX)
  list(APPEND coil_core_all_libraries wayland_vulkan)
endif()

if(TRUE)
  add_library(coil_core_audio STATIC)
  target_sources(coil_core_audio PUBLIC FILE_SET CXX_MODULES FILES
    audio.cppm
    audio_soft.cppm
  )
  target_link_libraries(coil_core_audio
    PUBLIC
      coil_core_base
  )
  target_compile_features(coil_core_audio PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries audio)
endif()
list(APPEND coil_core_all_libraries audio)

if(TRUE)
  add_library(coil_core_video STATIC)
  target_sources(coil_core_video PUBLIC FILE_SET CXX_MODULES FILES
    video.cppm
  )
  target_link_libraries(coil_core_video
    PUBLIC
      coil_core_image
  )
  target_compile_features(coil_core_video PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries video)
endif()
list(APPEND coil_core_all_libraries video)

if(WIN32)
  add_library(coil_core_wincoreaudio STATIC)
  target_sources(coil_core_wincoreaudio PUBLIC FILE_SET CXX_MODULES FILES
    wincoreaudio.cppm
  )
  target_link_libraries(coil_core_wincoreaudio
    PUBLIC
      coil_core_audio
      coil_core_windows
  )
  target_compile_features(coil_core_wincoreaudio PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries wincoreaudio)
endif()
if(WIN32)
  list(APPEND coil_core_all_libraries wincoreaudio)
endif()

if(TARGET PulseAudio::PulseAudio)
  add_library(coil_core_pulseaudio STATIC)
  target_sources(coil_core_pulseaudio PUBLIC FILE_SET CXX_MODULES FILES
    pulseaudio.cppm
  )
  target_link_libraries(coil_core_pulseaudio
    PUBLIC
      coil_core_appidentity
      coil_core_audio
      PulseAudio::PulseAudio
  )
  target_compile_features(coil_core_pulseaudio PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries pulseaudio)
endif()
if(UNIX)
  list(APPEND coil_core_all_libraries pulseaudio)
endif()

if(TARGET Ogg::ogg)
  add_library(coil_core_ogg STATIC)
  target_sources(coil_core_ogg PUBLIC FILE_SET CXX_MODULES FILES
    ogg.cppm
  )
  target_link_libraries(coil_core_ogg
    PUBLIC
      coil_core_tasks
      Ogg::ogg
  )
  target_compile_features(coil_core_ogg PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries ogg)
endif()
list(APPEND coil_core_all_libraries ogg)

if(TARGET Webm::Webm)
  add_library(coil_core_webm STATIC)
  target_sources(coil_core_webm PUBLIC FILE_SET CXX_MODULES FILES
    webm.cppm
  )
  target_link_libraries(coil_core_webm
    PUBLIC
      coil_core_tasks
      Webm::Webm
  )
  target_compile_features(coil_core_webm PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries webm)
endif()
list(APPEND coil_core_all_libraries webm)

if(TARGET Opus::opus)
  add_library(coil_core_opus STATIC)
  target_sources(coil_core_opus PUBLIC FILE_SET CXX_MODULES FILES
    opus.cppm
  )
  target_link_libraries(coil_core_opus
    PUBLIC
      coil_core_audio
      coil_core_tasks
    PRIVATE
      Opus::opus
  )
  target_compile_features(coil_core_opus PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries opus)
endif()
list(APPEND coil_core_all_libraries opus)

if(TARGET libgav1::libgav1)
  add_library(coil_core_av1 STATIC)
  target_sources(coil_core_av1 PUBLIC FILE_SET CXX_MODULES FILES
    av1.cppm
  )
  target_link_libraries(coil_core_av1
    PUBLIC
      coil_core_tasks
      coil_core_video
      libgav1::libgav1
  )
  target_compile_features(coil_core_av1 PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries av1)
endif()
list(APPEND coil_core_all_libraries av1)

if(TARGET CURL::libcurl)
  add_library(coil_core_curl STATIC)
  target_sources(coil_core_curl PUBLIC FILE_SET CXX_MODULES FILES
    curl.cppm
  )
  target_sources(coil_core_curl PUBLIC FILE_SET HEADERS FILES
    curl.hpp
  )
  target_link_libraries(coil_core_curl
    PUBLIC
      coil_core_tasks
      CURL::libcurl
      $<$<BOOL:${WIN32}>:MbedTLS::mbedtls;crypt32.lib>
  )
  target_compile_features(coil_core_curl PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries curl)
endif()
list(APPEND coil_core_all_libraries curl)

if(TARGET Steam::Steam)
  add_library(coil_core_steam STATIC)
  target_sources(coil_core_steam PUBLIC FILE_SET CXX_MODULES FILES
    steam.cppm
  )
  target_link_libraries(coil_core_steam
    PUBLIC
      coil_core_player_input
      Steam::Steam
  )
  target_compile_features(coil_core_steam PUBLIC cxx_std_26)
  list(APPEND coil_core_libraries steam)
endif()
list(APPEND coil_core_all_libraries steam)

list(TRANSFORM coil_core_libraries PREPEND coil_core_)


# executables (tools)

if(TARGET coil_core_localization)
  add_executable(coil_core_localization_tool)
  target_sources(coil_core_localization_tool PRIVATE
    localization_tool.cpp
  )
  target_link_libraries(coil_core_localization_tool
    coil_core_entrypoint_console
    coil_core_fs
    coil_core_json
    coil_core_text
    coil_core_unicode
  )
  list(APPEND coil_core_tools localization_tool)
endif()
list(APPEND coil_core_all_tools localization_tool)

list(TRANSFORM coil_core_tools PREPEND coil_core_)


# executables (examples)

if(TARGET coil_core_sdl_vulkan)
  add_executable(example_render_gradient)
  target_sources(example_render_gradient PRIVATE
    examples/example_render_gradient.cpp
  )
  target_link_libraries(example_render_gradient
    coil_core_appidentity
    coil_core_entrypoint_console
    coil_core_render
    coil_core_sdl
    coil_core_sdl_vulkan
    coil_core_vulkan
  )
  list(APPEND coil_core_examples example_render_gradient)
endif()
list(APPEND coil_core_all_examples example_render_gradient)

if(TARGET coil_core_vulkan)
  add_executable(example_vulkan_compute)
  target_sources(example_vulkan_compute PRIVATE
    examples/example_vulkan_compute.cpp
  )
  target_link_libraries(example_vulkan_compute
    coil_core_entrypoint_console
    coil_core_vulkan
  )
  list(APPEND coil_core_examples example_vulkan_compute)
endif()
list(APPEND coil_core_all_examples example_vulkan_compute)

if(TARGET coil_core_steam)
  add_executable(example_steam)
  target_sources(example_steam PRIVATE
    examples/example_steam.cpp
  )
  target_link_libraries(example_steam
    coil_core_appidentity
    coil_core_assets
    coil_core_entrypoint_console
    coil_core_fs
    coil_core_image_compress
    coil_core_image_png
    coil_core_player_input_native
    coil_core_render
    coil_core_sdl
    coil_core_sdl_vulkan
    coil_core_steam
    coil_core_vulkan
  )
  list(APPEND coil_core_examples example_steam)
endif()
list(APPEND coil_core_all_examples example_steam)


# interface
foreach(library ${coil_core_libraries})
  target_include_directories(${library}
    INTERFACE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )
endforeach()


list(APPEND coil_core_executables ${coil_core_tools})
set(COIL_CORE_INSTALL_EXAMPLES OFF CACHE BOOL "whether to install example executables")
if(COIL_CORE_INSTALL_EXAMPLES)
  list(APPEND coil_core_executables ${coil_core_examples})
endif()

# installation
install(TARGETS ${coil_core_libraries} ${coil_core_executables}
  EXPORT coil_coreTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/coil"
  FILE_SET HEADERS DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/coil"
  FILE_SET CXX_MODULES DESTINATION "${CMAKE_INSTALL_LIBDIR}/src"
  CXX_MODULES_BMI DESTINATION "${CMAKE_INSTALL_LIBDIR}/cxx"
)
install(EXPORT coil_coreTargets
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/coil_core"
  CXX_MODULES_DIRECTORY .
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

  if(TARGET coil_core_data)
    add_executable(test_data)
    target_sources(test_data PRIVATE
      test_data.cpp
    )
    target_link_libraries(test_data
      coil_core_data
      coil_core_entrypoint_console
    )
    add_test(NAME test_data COMMAND test_data)
  endif()

  if(TARGET coil_core_process)
    add_executable(test_process)
    target_sources(test_process PRIVATE
      test_process.cpp
    )
    target_link_libraries(test_process
      coil_core_appidentity
      coil_core_entrypoint_console
      coil_core_process
    )
    add_test(NAME test_process COMMAND test_process)
  endif()

  if(TARGET coil_core_math_determ)
    add_executable(test_math_determ)
    target_sources(test_math_determ PRIVATE
      test_math_determ.cpp
    )
    target_link_libraries(test_math_determ
      coil_core_entrypoint_console
      coil_core_math_determ
    )
    add_test(NAME test_math_determ COMMAND test_math_determ)
  endif()

  if(TARGET coil_core_tasks)
    add_executable(test_tasks)
    target_sources(test_tasks PRIVATE
      test_tasks.cpp
    )
    target_link_libraries(test_tasks
      coil_core_entrypoint_console
      coil_core_math
      coil_core_tasks
    )
    add_test(NAME test_tasks COMMAND test_tasks)
  endif()

  if(TARGET coil_core_json)
    add_executable(test_json)
    target_sources(test_json PRIVATE
      test_json.cpp
    )
    target_link_libraries(test_json
      coil_core_entrypoint_console
      coil_core_json
      coil_core_unicode
    )
    add_test(NAME test_json COMMAND test_json)
  endif()

  if(TARGET coil_core_localization)
    set(test_localization_localized "${CMAKE_BINARY_DIR}/test_localization_localized")
    add_custom_command(
      OUTPUT
        ${test_localization_localized}/localized.cpp
        ${test_localization_localized}/en.cpp
        ${test_localization_localized}/ru.cpp
      COMMAND coil_core_localization_tool -w "${CMAKE_CURRENT_LIST_DIR}" "${CMAKE_CURRENT_LIST_DIR}/test_localization_config.json" "${test_localization_localized}"
      DEPENDS
        coil_core_localization_tool
        "${CMAKE_CURRENT_LIST_DIR}/test_localization_config.json"
        "${CMAKE_CURRENT_LIST_DIR}/test_localization_en.json"
        "${CMAKE_CURRENT_LIST_DIR}/test_localization_ru.json"
    )
    add_executable(test_localization)
    target_sources(test_localization PRIVATE
      test_localization.cpp
    )
    target_sources(test_localization PRIVATE FILE_SET CXX_MODULES BASE_DIRS "${CMAKE_CURRENT_LIST_DIR};${test_localization_localized}" FILES
      ${test_localization_localized}/localized.cpp
      ${test_localization_localized}/en.cpp
      ${test_localization_localized}/ru.cpp
    )
    target_link_libraries(test_localization
      coil_core_entrypoint_console
      coil_core_localization
    )
    target_include_directories(test_localization PRIVATE ${test_localization_localized})
    add_test(NAME test_localization COMMAND test_localization)
  endif()

  if(TARGET coil_core_compress_zstd)
    add_executable(test_compress_zstd)
    target_sources(test_compress_zstd PRIVATE
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
    add_executable(test_sqlite)
    target_sources(test_sqlite PRIVATE
      test_sqlite.cpp
    )
    target_link_libraries(test_sqlite
      coil_core_entrypoint_console
      coil_core_sqlite
    )
    add_test(NAME test_sqlite COMMAND test_sqlite)
  endif()

  if(TARGET coil_core_crypto)
    add_executable(test_crypto)
    target_sources(test_crypto PRIVATE
      test_crypto.cpp
    )
    target_link_libraries(test_crypto
      coil_core_crypto
      coil_core_entrypoint_console
    )
    add_test(NAME test_crypto COMMAND test_crypto)
  endif()


endif() # BUILD_TESTING


# check that everything required to be built is actually built

# require all libraries to build by default
set(COIL_CORE_REQUIRE_LIBS ${coil_core_all_libraries} CACHE STRING "list of libs required to build")
# allow to exclude some libraries
set(COIL_CORE_DONT_REQUIRE_LIBS "" CACHE STRING "list of libs not required to build (overrides COIL_CORE_REQUIRE_LIBS)")

foreach(library ${COIL_CORE_REQUIRE_LIBS})
  if((NOT TARGET coil_core_${library}) AND (NOT (library IN_LIST COIL_CORE_DONT_REQUIRE_LIBS)))
    message(SEND_ERROR "required library coil_core_${library} is not set up")
  endif()
endforeach()
