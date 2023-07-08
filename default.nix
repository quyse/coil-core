{ pkgsFun ? import <nixpkgs>
, pkgs ? pkgsFun {}
, lib ? pkgs.lib
, coil
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
        clang_16
      ];
    });

    libsquish = stdenv.mkDerivation rec {
      pname = "libsquish";
      version = "1.15";
      src = pkgs.fetchurl {
        url = "https://downloads.sourceforge.net/libsquish/libsquish-${version}.tgz";
        hash = "sha256-YoeW7rpgiGYYOmHQgNRpZ8ndpnI7wKPsUjJMhdIUcmk=";
      };
      sourceRoot = ".";
      nativeBuildInputs = [
        cmake
        ninja
      ];
      buildInputs = [
        openmp
      ];
      cmakeFlags = [
        "-DBUILD_SHARED_LIBS=ON"
      ];
    };

    inherit (llvmPackages_16) openmp;

    steam-sdk = if coil.toolchain-steam != null then coil.toolchain-steam.sdk else null;
  });
  coil-core-nixos = nixos-pkgs.coil-core;

  # Ubuntu build
  ubuntu-pkgs = rec {
    clangVersion = "16";
    diskImage = coil.toolchain-linux.diskImagesFuns.ubuntu_2204_amd64 [
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
      "libsquish-dev"
      "libfreetype-dev"
      "libharfbuzz-dev"
      "libwayland-dev"
      "wayland-protocols"
      "libxkbcommon-dev"
      "libpulse-dev"
      "libogg-dev"
      "libopus-dev"
    ];
    coil-core = pkgs.vmTools.runInLinuxImage ((pkgs.callPackage ./coil-core.nix {
      libsquish = null;
      steam-sdk = if coil.toolchain-steam != null then coil.toolchain-steam.sdk else null;
    }).overrideAttrs (attrs: {
      propagatedBuildInputs = [];
      cmakeFlags = (attrs.cmakeFlags or []) ++ [
        # force clang
        "-DCMAKE_CXX_COMPILER=clang++-${clangVersion}"
        "-DCMAKE_C_COMPILER=clang-${clangVersion}"
        # some libs do not work yet
        "-DCOIL_CORE_DONT_REQUIRE_LIBS=compress_zstd"
        # steam
        "-DSteam_INCLUDE_DIRS=${coil.toolchain-steam.sdk}/include"
        "-DSteam_LIBRARIES=${coil.toolchain-steam.sdk}/lib/libsteam_api.so"
      ];
      inherit diskImage;
      diskImageFormat = "qcow2";
      memSize = 2048;
      enableParallelBuilding = true;
    }));
  };
  coil-core-ubuntu = ubuntu-pkgs.coil-core;

  # Windows build
  windows-pkgs = import ./pkgs/windows-pkgs.nix {
    pkgs = nixos-pkgs;
    inherit lib coil dontRequireLibsList;
  };
  coil-core-windows = windows-pkgs.coil-core;

  # list of libs to not require if dependencies not available
  dontRequireLibsList = lib.concatStringsSep ";" (lib.concatLists [
    (lib.optional (coil.toolchain-steam == null) "steam")
  ]);

  touch = {
    inherit coil-core-nixos coil-core-ubuntu coil-core-windows;
  };
}
