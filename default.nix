{ pkgsFun ? import <nixpkgs>
, pkgs ? pkgsFun {}
, lib ? pkgs.lib
}:

rec {
  # NixOS build
  coil-core = (pkgs.callPackage ./coil-core.nix {}).overrideAttrs (attrs: {
    # force clang
    cmakeFlags = (attrs.cmakeFlags or []) ++ [
      "-DCMAKE_CXX_COMPILER=clang++"
      "-DCMAKE_C_COMPILER=clang"
    ];
    nativeBuildInputs = attrs.nativeBuildInputs ++ [
      pkgs.clang_14
    ];
  });

  # Ubuntu build
  coil-core-ubuntu = pkgs.vmTools.runInLinuxImage (coil-core.overrideAttrs (attrs: {
    diskImage = pkgs.vmTools.diskImageExtraFuns.ubuntu2204x86_64 [
      "clang"
      "cmake"
      "libfreetype-dev"
      "libharfbuzz-dev"
      "libogg-dev"
      "libopus-dev"
      "libpng-dev"
      "libsdl2-dev"
      "libsqlite3-dev"
      "libstdc++-10-dev"
      "libvulkan-dev"
      "libwayland-dev"
      "libxkbcommon-dev"
      "libzstd-dev"
      "ninja-build"
      "nlohmann-json3-dev"
      "pkg-config"
      "spirv-headers"
      "wayland-protocols"
    ];
    diskImageFormat = "qcow2";
    memSize = 2048;
  }));

  touch = {
    inherit coil-core coil-core-ubuntu;
  };
}
