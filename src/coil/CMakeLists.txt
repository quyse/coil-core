cmake_minimum_required(VERSION 3.19)

project(coil-core VERSION 3.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZAITON ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)

pkg_search_module(HARFBUZZ REQUIRED harfbuzz)

FILE(GLOB Sources *.cpp)

add_library(coil-core STATIC
  ${Sources}
)
target_include_directories(coil-core PRIVATE
  ${HARFBUZZ_INCLUDE_DIRS}
)
