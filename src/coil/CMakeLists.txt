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
    util_generator.hpp
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
  add_library(coil_core_appidentity STATIC
    appidentity.cpp
  )
  set_property(TARGET coil_core_appidentity PROPERTY PUBLIC_HEADER
    appidentity.hpp
  )
  list(APPEND coil_core_libraries appidentity)
endif()
list(APPEND coil_core_all_libraries appidentity)

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
  add_library(coil_core_assets INTERFACE)
  set_property(TARGET coil_core_assets PROPERTY PUBLIC_HEADER
    assets.hpp
    assets_struct.hpp
  )
  target_link_libraries(coil_core_assets
    INTERFACE
      coil_core_json
      coil_core_tasks
  )
  list(APPEND coil_core_libraries assets)
endif()
list(APPEND coil_core_all_libraries assets)

if(TRUE)
  add_library(coil_core_fs STATIC
    fs.cpp
  )
  set_property(TARGET coil_core_fs PROPERTY PUBLIC_HEADER
    fs.hpp
  )
  target_link_libraries(coil_core_fs
    PUBLIC
      coil_core_tasks
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

if(TRUE)
  add_library(coil_core_math_determ STATIC
    math_determ.cpp
  )
  set_property(TARGET coil_core_math_determ PROPERTY PUBLIC_HEADER
    math_determ.hpp
    math_determ_debug.hpp
  )
  target_compile_options(coil_core_math_determ
    PRIVATE
      "$<IF:$<BOOL:${MSVC}>,/fp:strict,-ffp-model=strict>"
  )
  target_link_libraries(coil_core_math_determ
    PUBLIC
      coil_core_math
  )
  list(APPEND coil_core_libraries math_determ)
endif()
list(APPEND coil_core_all_libraries math_determ)

if(TRUE)
  add_library(coil_core_tasks STATIC
    tasks.cpp
    tasks_streams.cpp
  )
  set_property(TARGET coil_core_tasks PROPERTY PUBLIC_HEADER
    tasks.hpp
    tasks_streams.hpp
    tasks_sync.hpp
  )
  target_link_libraries(coil_core_tasks
    PUBLIC
      coil_core_data
  )
  list(APPEND coil_core_libraries tasks)
endif()
list(APPEND coil_core_all_libraries tasks)

if(TARGET nlohmann_json::nlohmann_json)
  add_library(coil_core_json STATIC
    json.cpp
  )
  set_property(TARGET coil_core_json PROPERTY PUBLIC_HEADER
    json.hpp
  )
  target_link_libraries(coil_core_json
    PUBLIC
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
  add_library(coil_core_text INTERFACE)
  set_property(TARGET coil_core_text PROPERTY PUBLIC_HEADER
    text.hpp
  )
  target_link_libraries(coil_core_text
    INTERFACE
      coil_core_base
  )
  list(APPEND coil_core_libraries text)
endif()
list(APPEND coil_core_all_libraries text)

if(TRUE)
  add_library(coil_core_localization INTERFACE)
  set_property(TARGET coil_core_localization PROPERTY PUBLIC_HEADER
    localization.hpp
  )
  target_link_libraries(coil_core_localization
    INTERFACE
      coil_core_text
  )
  list(APPEND coil_core_libraries localization)
endif()
list(APPEND coil_core_all_libraries localization)

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
  add_library(coil_core_player_input STATIC
    player_input.cpp
  )
  set_property(TARGET coil_core_player_input PROPERTY PUBLIC_HEADER
    player_input.hpp
    player_input_combined.hpp
  )
  target_link_libraries(coil_core_player_input
    PUBLIC
      coil_core_base
  )
  list(APPEND coil_core_libraries player_input)
endif()
list(APPEND coil_core_all_libraries player_input)

if(TRUE)
  add_library(coil_core_player_input_native STATIC
    player_input_native.cpp
  )
  set_property(TARGET coil_core_player_input_native PROPERTY PUBLIC_HEADER
    player_input_native.hpp
  )
  target_link_libraries(coil_core_player_input_native
    PUBLIC
      coil_core_input
      coil_core_json
      coil_core_player_input
  )
  list(APPEND coil_core_libraries player_input_native)
endif()
list(APPEND coil_core_all_libraries player_input_native)

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
      coil_core_util
  )
  list(APPEND coil_core_libraries platform)
endif()
list(APPEND coil_core_all_libraries platform)

if(TRUE)
  add_library(coil_core_graphics STATIC
    graphics.cpp
    graphics_shaders.cpp
  )
  set_property(TARGET coil_core_graphics PROPERTY PUBLIC_HEADER
    graphics.hpp
    graphics_format.hpp
    graphics_shaders.hpp
  )
  target_link_libraries(coil_core_graphics
    PUBLIC
      coil_core_image
      coil_core_platform
      coil_core_util
  )
  list(APPEND coil_core_libraries graphics)
endif()
list(APPEND coil_core_all_libraries graphics)

if(TRUE)
  add_library(coil_core_image STATIC
    image.cpp
    image_format.cpp
  )
  set_property(TARGET coil_core_image PROPERTY PUBLIC_HEADER
    image.hpp
    image_format.hpp
    image_typed.hpp
  )
  target_link_libraries(coil_core_image
    PUBLIC
      coil_core_math
  )
  list(APPEND coil_core_libraries image)
endif()
list(APPEND coil_core_all_libraries image)

if(TRUE)
  add_library(coil_core_image_transform STATIC
    image_transform.cpp
  )
  set_property(TARGET coil_core_image_transform PROPERTY PUBLIC_HEADER
    image_transform.hpp
  )
  target_link_libraries(coil_core_image_transform
    PUBLIC
      coil_core_image
      coil_core_tasks
  )
  list(APPEND coil_core_libraries image_transform)
endif()
list(APPEND coil_core_all_libraries image_transform)

if(TRUE)
  add_library(coil_core_render STATIC
    render.cpp
    render_canvas.cpp
  )
  set_property(TARGET coil_core_render PROPERTY PUBLIC_HEADER
    render.hpp
    render_canvas.hpp
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
    PRIVATE
      coil_core_appidentity
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
      coil_core_image
      coil_core_tasks
    PRIVATE
      PNG::PNG
  )
  list(APPEND coil_core_libraries image_png)
endif()
list(APPEND coil_core_all_libraries image_png)

if(TARGET Squish::Squish)
  add_library(coil_core_image_compress STATIC
    image_compress.cpp
  )
  set_property(TARGET coil_core_image_compress PROPERTY PUBLIC_HEADER
    image_compress.hpp
  )
  target_link_libraries(coil_core_image_compress
    PUBLIC
      coil_core_image
      coil_core_tasks
    PRIVATE
      Squish::Squish
  )
  list(APPEND coil_core_libraries image_compress)
endif()
list(APPEND coil_core_all_libraries image_compress)

if(TARGET zstd::libzstd_shared)
  add_library(coil_core_compress_zstd STATIC
    compress_zstd.cpp
  )
  set_property(TARGET coil_core_compress_zstd PROPERTY PUBLIC_HEADER
    compress_zstd.hpp
  )
  target_link_libraries(coil_core_compress_zstd
    PUBLIC
      coil_core_tasks
      zstd::libzstd_shared
  )
  list(APPEND coil_core_libraries compress_zstd)
endif()
list(APPEND coil_core_all_libraries compress_zstd)

if(TRUE)
  add_library(coil_core_mesh INTERFACE)
  set_property(TARGET coil_core_mesh PROPERTY PUBLIC_HEADER
    mesh.hpp
  )
  target_link_libraries(coil_core_mesh
    INTERFACE
      coil_core_math
  )
  list(APPEND coil_core_libraries mesh)
endif()
list(APPEND coil_core_all_libraries mesh)

if(TARGET coil_core_json)
  add_library(coil_core_gltf STATIC
    gltf.cpp
  )
  set_property(TARGET coil_core_gltf PROPERTY PUBLIC_HEADER
    gltf.hpp
    gltf_cache.hpp
  )
  target_link_libraries(coil_core_gltf
    PUBLIC
      coil_core_math
      coil_core_mesh
    PRIVATE
      coil_core_data
      coil_core_json
  )
  list(APPEND coil_core_libraries gltf)
endif()
list(APPEND coil_core_all_libraries gltf)

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

if(TARGET MbedTLS::mbedcrypto)
  add_library(coil_core_crypto STATIC
    crypto_mbedtls.cpp
  )
  set_property(TARGET coil_core_crypto PROPERTY PUBLIC_HEADER
    crypto.hpp
    crypto_base.hpp
    crypto_mbedtls.hpp
  )
  target_link_libraries(coil_core_crypto
    PUBLIC
      coil_core_base
      MbedTLS::mbedcrypto
  )
  list(APPEND coil_core_libraries crypto)
endif()
list(APPEND coil_core_all_libraries crypto)

if(TRUE)
  add_library(coil_core_fonts STATIC
    fonts.cpp
    fonts_cache.cpp
  )
  set_property(TARGET coil_core_fonts PROPERTY PUBLIC_HEADER
    fonts.hpp
    fonts_cache.hpp
  )
  target_link_libraries(coil_core_fonts
    PUBLIC
      coil_core_image
      coil_core_localization
  )
  list(APPEND coil_core_libraries fonts)
endif()
list(APPEND coil_core_all_libraries fonts)

if(TARGET coil_core_fonts)
  add_library(coil_core_render_fonts STATIC
    render_fonts.cpp
  )
  set_property(TARGET coil_core_render_fonts PROPERTY PUBLIC_HEADER
    render_fonts.hpp
  )
  target_link_libraries(coil_core_render_fonts
    PUBLIC
      coil_core_fonts
      coil_core_render
  )
  list(APPEND coil_core_libraries render_fonts)
endif()
list(APPEND coil_core_all_libraries render_fonts)

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
      coil_core_tasks
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
    audio_soft.cpp
  )
  set_property(TARGET coil_core_audio PROPERTY PUBLIC_HEADER
    audio.hpp
    audio_soft.hpp
  )
  list(APPEND coil_core_libraries audio)
endif()
list(APPEND coil_core_all_libraries audio)

if(TRUE)
  add_library(coil_core_video STATIC
    video.cpp
  )
  set_property(TARGET coil_core_video PROPERTY PUBLIC_HEADER
    video.hpp
  )
  target_link_libraries(coil_core_video
    PUBLIC
      coil_core_image
  )
  list(APPEND coil_core_libraries video)
endif()
list(APPEND coil_core_all_libraries video)

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

if(TARGET PulseAudio::PulseAudio)
  add_library(coil_core_pulseaudio STATIC
    pulseaudio.cpp
  )
  set_property(TARGET coil_core_pulseaudio PROPERTY PUBLIC_HEADER
    pulseaudio.hpp
  )
  target_link_libraries(coil_core_pulseaudio
    PUBLIC
      coil_core_audio
    PRIVATE
      coil_core_appidentity
      PulseAudio::PulseAudio
  )
  list(APPEND coil_core_libraries pulseaudio)
endif()
if(UNIX)
  list(APPEND coil_core_all_libraries pulseaudio)
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
      coil_core_tasks
      Ogg::ogg
  )
  list(APPEND coil_core_libraries ogg)
endif()
list(APPEND coil_core_all_libraries ogg)

if(TARGET Webm::Webm)
  add_library(coil_core_webm STATIC
    webm.cpp
  )
  set_property(TARGET coil_core_webm PROPERTY PUBLIC_HEADER
    webm.hpp
  )
  target_link_libraries(coil_core_webm
    PUBLIC
      coil_core_tasks
      Webm::Webm
  )
  list(APPEND coil_core_libraries webm)
endif()
list(APPEND coil_core_all_libraries webm)

if(TARGET Opus::opus)
  add_library(coil_core_opus STATIC
    opus.cpp
  )
  set_property(TARGET coil_core_opus PROPERTY PUBLIC_HEADER
    opus.hpp
  )
  target_link_libraries(coil_core_opus
    PUBLIC
      coil_core_audio
      coil_core_tasks
    PRIVATE
      Opus::opus
  )
  list(APPEND coil_core_libraries opus)
endif()
list(APPEND coil_core_all_libraries opus)

if(TARGET libgav1::libgav1)
  add_library(coil_core_av1 STATIC
    av1.cpp
  )
  set_property(TARGET coil_core_av1 PROPERTY PUBLIC_HEADER
    av1.hpp
  )
  target_link_libraries(coil_core_av1
    PUBLIC
      coil_core_tasks
      coil_core_video
      libgav1::libgav1
  )
  list(APPEND coil_core_libraries av1)
endif()
list(APPEND coil_core_all_libraries av1)

if(TARGET CURL::libcurl)
  add_library(coil_core_curl STATIC
    curl.cpp
  )
  set_property(TARGET coil_core_curl PROPERTY PUBLIC_HEADER
    curl.hpp
  )
  target_link_libraries(coil_core_curl
    PUBLIC
      coil_core_tasks
      CURL::libcurl
  )
  list(APPEND coil_core_libraries curl)
endif()
list(APPEND coil_core_all_libraries curl)

if(TARGET Steam::Steam)
  add_library(coil_core_steam STATIC
    steam.cpp
  )
  set_property(TARGET coil_core_steam PROPERTY PUBLIC_HEADER
    steam.hpp
  )
  target_link_libraries(coil_core_steam
    PUBLIC
      coil_core_player_input
      Steam::Steam
  )
  list(APPEND coil_core_libraries steam)
endif()
list(APPEND coil_core_all_libraries steam)

list(TRANSFORM coil_core_libraries PREPEND coil_core_)


# executables (tools)

if(TARGET coil_core_localization)
  add_executable(coil_core_localization_tool
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
  add_executable(example_render_gradient
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

if(TARGET coil_core_steam)
  add_executable(example_steam
    examples/example_steam.cpp
  )
  target_link_libraries(example_steam
    coil_core_appidentity
    coil_core_entrypoint_console
    coil_core_fs
    coil_core_image_compress
    coil_core_image_png
    coil_core_player_input_native
    coil_core_render
    coil_core_sdl
    coil_core_sdl_vulkan
    coil_core_steam
    coil_core_util
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
      $<INSTALL_INTERFACE:include>
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

  if(TARGET coil_core_data)
    add_executable(test_data
      test_data.cpp
    )
    target_link_libraries(test_data
      coil_core_data
      coil_core_entrypoint_console
    )
    add_test(NAME test_data COMMAND test_data)
  endif()

  if(TARGET coil_core_math_determ)
    add_executable(test_math_determ
      test_math_determ.cpp
    )
    target_link_libraries(test_math_determ
      coil_core_entrypoint_console
      coil_core_math_determ
    )
    add_test(NAME test_math_determ COMMAND test_math_determ)
  endif()

  if(TARGET coil_core_tasks)
    add_executable(test_tasks
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

  if(TARGET coil_core_localization)
    set(test_localization_localized "${CMAKE_BINARY_DIR}/test_localization_localized")
    add_custom_command(
      OUTPUT
        ${test_localization_localized}/localized.cpp
        ${test_localization_localized}/localized.hpp
        ${test_localization_localized}/en.cpp
        ${test_localization_localized}/ru.cpp
      COMMAND coil_core_localization_tool -w "${CMAKE_CURRENT_LIST_DIR}" "${CMAKE_CURRENT_LIST_DIR}/test_localization_config.json" "${test_localization_localized}"
      DEPENDS
        coil_core_localization_tool
        "${CMAKE_CURRENT_LIST_DIR}/test_localization_config.json"
        "${CMAKE_CURRENT_LIST_DIR}/test_localization_en.json"
        "${CMAKE_CURRENT_LIST_DIR}/test_localization_ru.json"
    )
    add_executable(test_localization
      test_localization.cpp
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

  if(TARGET coil_core_crypto)
    add_executable(test_crypto
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
