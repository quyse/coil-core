{ pkgsFun ? import <nixpkgs>
, pkgs ? pkgsFun {}
, lib ? pkgs.lib
}:

rec {
  # NixOS build
  coil-core = (pkgs.callPackage ./coil-core.nix {
    # fix cmake files; harfbuzz has this fix
    harfbuzz = pkgs.harfbuzz.overrideAttrs (attrs: {
      patches = (attrs.patches or []) ++ [
        (pkgs.fetchpatch {
          url = "https://github.com/harfbuzz/harfbuzz/pull/3857.patch";
          hash = "sha256-a2RMkRVk+yDReZjn7IHbLLHpuAkyCdvtkKc8ubVN98w=";
        })
      ];
    });
    # build with cmake to get cmake exports
    libogg = pkgs.libogg.overrideAttrs (attrs: {
      nativeBuildInputs = (attrs.nativeBuildInputs or []) ++ [pkgs.cmake];
    });
    # use git checkout instead of tarball to get cmake exports
    libopus = pkgs.libopus.overrideAttrs (attrs: {
      src = pkgs.fetchgit {
        url = "https://github.com/xiph/opus";
        rev = "v1.3.1";
        hash = "sha256-DO9JAO6907VpOUgBiJ4WIZm9hTAYBM2Qabi+x1ibqN4=";
      };
      nativeBuildInputs = (attrs.nativeBuildInputs or []) ++ [pkgs.cmake];
    });
  }).overrideAttrs (attrs: {
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
