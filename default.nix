{ pkgsFun ? import <nixpkgs>
, pkgs ? pkgsFun {}
, lib ? pkgs.lib
, toolchain-linux
, toolchain-windows
, toolchain-steam
}:

rec {
  # NixOS build
  nixos-pkgs = pkgs.extend (self: super: with self; {
    coil-core = (callPackage ./coil-core.nix {
    }).overrideAttrs (attrs: {
      cmakeFlags = (attrs.cmakeFlags or []) ++ [
        # force clang
        "-DCMAKE_CXX_COMPILER=clang++"
        "-DCMAKE_C_COMPILER=clang"
        # do not require some libs
        "-DCOIL_CORE_DONT_REQUIRE_LIBS=${dontRequireLibsList}"
      ];
      nativeBuildInputs = attrs.nativeBuildInputs ++ [
        pkgs.clang_16
      ];
    });
    steam-sdk = if toolchain-steam != null then toolchain-steam.sdk else null;
  });
  coil-core-nixos = nixos-pkgs.coil-core;

  # Ubuntu build
  coil-core-ubuntu = let
    clangVersion = "15";
  in pkgs.vmTools.runInLinuxImage ((pkgs.callPackage ./coil-core.nix {
    steam-sdk = if toolchain-steam != null then toolchain-steam.sdk else null;
  }).overrideAttrs (attrs: {
    nativeBuildInputs = [
      pkgs.cmake
      pkgs.ninja
    ];
    propagatedBuildInputs = [
    ];
    cmakeFlags = (attrs.cmakeFlags or []) ++ [
      # force clang
      "-DCMAKE_CXX_COMPILER=clang++-${clangVersion}"
      "-DCMAKE_C_COMPILER=clang-${clangVersion}"
      # some libs do not work yet
      "-DCOIL_CORE_DONT_REQUIRE_LIBS=compress_zstd;steam"
    ];
    diskImage = toolchain-linux.diskImagesFuns.ubuntu2204x86_64 [
      "clang-${clangVersion}"
      "cmake"
      "ninja-build"
      "pkg-config"
      "libsdl2-dev"
      "libvulkan-dev"
      "spirv-headers"
      "nlohmann-json3-dev"
      "libzstd-dev"
      "libsqlite3-dev"
      "libpng-dev"
      "libfreetype-dev"
      "libharfbuzz-dev"
      "libwayland-dev"
      "wayland-protocols"
      "libxkbcommon-dev"
      "libpulse-dev"
      "libogg-dev"
      "libopus-dev"
    ];
    diskImageFormat = "qcow2";
    memSize = 2048;
  }));

  # Windows build
  windows-pkgs = import ./pkgs/windows-pkgs.nix {
    pkgs = nixos-pkgs;
    inherit lib toolchain-windows toolchain-steam dontRequireLibsList;
  };
  coil-core-windows = windows-pkgs.coil-core;

  # list of libs to not require if dependencies not available
  dontRequireLibsList = lib.concatStringsSep ";" (lib.concatLists [
    (lib.optional (toolchain-steam == null) "steam")
  ]);

  touch = {
    inherit coil-core-nixos coil-core-ubuntu coil-core-windows;
  };
}
