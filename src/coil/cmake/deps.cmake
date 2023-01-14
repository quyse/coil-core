list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})

find_package(PkgConfig)

find_package(nlohmann_json)
find_package(Vulkan)
find_package(SPIRV-Headers)
find_package(zstd)
find_package(SDL2)
find_package(PNG)
find_package(SQLite3)
find_package(Freetype)
find_package(harfbuzz)
find_package(Ogg)
find_package(Opus)

pkg_search_module(WaylandClient IMPORTED_TARGET wayland-client)
pkg_search_module(WaylandProtocols IMPORTED_TARGET wayland-protocols)
pkg_search_module(XkbCommon IMPORTED_TARGET xkbcommon)
