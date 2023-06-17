{ pkgsFun ? import <nixpkgs>
, pkgs ? pkgsFun {}
, lib ? pkgs.lib
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
  coil-core-ubuntu = pkgs.vmTools.runInLinuxImage (coil-core-nixos.overrideAttrs (attrs: {
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
      "libpulse-dev"
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
