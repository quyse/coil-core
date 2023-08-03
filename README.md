# Coil Core game engine

Work-in-progress C++ 20 game engine. Not generally usable yet.

## Features

* Vulkan graphics
* SPIR-V shaders in C++
* Math library
  * Also deterministic floating-point math library (WIP)
* Windowing
  * SDL2
  * Wayland (WIP)
* Audio
  * PulseAudio
  * Windows CoreAudio
  * Ogg/Opus
* Localization utilities
* Text rendering with Freetype/Harfbuzz
* Various cross-platform utilities
* glTF model support (WIP)
* Rendering (WIP)

## Libraries

Some WIP documentation.

* [Basic](docs/basic.md)
* [Data](docs/data.md)
* [Math](docs/math.md)
* [Platform](docs/platform.md)
* [Graphics](docs/graphics.md)
* [Audio](docs/audio.md)
* [Text](docs/text.md)

## Platform support

* Linux
* Windows
* macOS (planned)

## Building

[NixOS](https://nixos.org/) is the main development platform, and the Nix configuration is the easiest to use. It contains the following CI-tested attributes:

* `coil-core-nixos` - built using [nixpkgs](https://github.com/NixOS/nixpkgs) dependencies, for use on NixOS
* `coil-core-ubuntu` - built using Ubuntu LTS
* `coil-core-windows` - Windows build, using MSVC/clang toolchain

### Building without Nix

Even non-NixOS builds are automated using Nix, so building without Nix is not officially supported. Build configuration is done with CMake. See `ubuntu-pkgs` attribute in [default.nix](default.nix) for the list of third-party dependencies. Recent Clang or MSVC-clang is probably required (other compilers are not tested), e.g. `cmake -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang ...`.

`COIL_CORE_REQUIRE_LIBS` CMake variable can be used to require only selected subset of engine libraries, e.g. for the case when not all dependencies are available. By default all libraries are required to build, and configuration will fail if some third-party dependency is not available. Use empty value (`-DCOIL_CORE_REQUIRE_LIBS=`) to configure as many libraries as possible without failing if some of them cannot be configured.
