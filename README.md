# Coil Core game engine

Work-in-progress C++ 20 game engine. Not generally usable yet.

## Features

* Vulkan graphics
* SPIR-V shaders in C++
* Linux and Windows support
* Windowing
  * SDL2
  * Wayland
* Audio
  * PulseAudio
  * Windows CoreAudio
  * Ogg/Opus
* Localization utilities
* Text rendering with Freetype/Harfbuzz
* Various cross-platform utilities
* glTF model support (WIP)
* Rendering (WIP)

## Building

Nix configuration is the easiest to use. It contains the following CI-tested attributes:

* `coil-core` - built using [nixpkgs](https://github.com/NixOS/nixpkgs) dependencies, for use on NixOS
* `coil-core-ubuntu` - built on Ubuntu, for non-NixOS Linuxes
* `coil-core-windows` - Windows build, using MSVC/clang toolchain

If Nix is not available, the engine can be compiled manually using CMake on various platforms. See [default.nix](default.nix) for lists of dependencies.

* Forcing clang or MSVC-clang is usually required (GCC does not work), e.g. `-DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang`
* `COIL_CORE_REQUIRE_LIBS` can be used to require only necessary engine libraries (for example, if not all dependencies are available). By default all libraries are required to build, and configuration will fail if some dependency is not available (useful for CI). Use `-DCOIL_CORE_REQUIRE_LIBS=` to not require any library (it will still try to configure as many libraries as possible).
