{ pkgsFun ? import <nixpkgs>
, pkgs ? pkgsFun {}
, lib ? pkgs.lib
, coil
, features ? null
}:

lib.makeExtensible (self: with self; {
  # NixOS build
  nixos-pkgs = pkgs.extend (self: super: with self; {
    coil-core = (callPackage ./coil-core.nix {
      inherit features;
    }).overrideAttrs (attrs: {
      cmakeFlags = (attrs.cmakeFlags or []) ++ [
        # force clang
        "-DCMAKE_CXX_COMPILER=clang++"
        "-DCMAKE_C_COMPILER=clang"
        # do not require some libs
        "-DCOIL_CORE_DONT_REQUIRE_LIBS=${dontRequireLibsList}"
      ];
      nativeBuildInputs = attrs.nativeBuildInputs ++ [
        clang
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
      meta.license = lib.licenses.mit;
    };

    libwebm = stdenv.mkDerivation rec {
      pname = "libwebm";
      version = "1.0.0.31";
      src = pkgs.fetchgit {
        url = "https://chromium.googlesource.com/webm/libwebm";
        rev = "libwebm-${version}";
        hash = "sha256-+ayX33rcX/jkewsW8WrGalTe9X44qFBHOrIYTteOQzc=";
      };
      nativeBuildInputs = [
        cmake
        ninja
      ];
      cmakeFlags = [
        "-DENABLE_WEBM_PARSER=ON"
      ];
      meta.license = lib.licenses.bsd3;
    };

    libgav1 = stdenv.mkDerivation rec {
      pname = "libgav1";
      version = "0.19.0";
      src = pkgs.fetchgit {
        url = "https://chromium.googlesource.com/codecs/libgav1";
        rev = "v${version}";
        hash = "sha256-kuDXv8H24UwyrOK0cAsTSvMPFCkPI8qUcj5u9WwgfkU=";
      };
      nativeBuildInputs = [
        cmake
        ninja
      ];
      cmakeFlags = [
        # https://github.com/NixOS/nixpkgs/issues/144170
        "-DCMAKE_INSTALL_INCLUDEDIR=include"
        "-DCMAKE_INSTALL_LIBDIR=lib"
        # eliminate abseil dependency
        "-DLIBGAV1_THREADPOOL_USE_STD_MUTEX=1"
        "-DLIBGAV1_ENABLE_EXAMPLES=0"
        "-DLIBGAV1_ENABLE_TESTS=0"
      ];
      meta.license = lib.licenses.asl20;
    };

    mbedtls = super.mbedtls.overrideAttrs (attrs: {
      postConfigure = (attrs.postConfigure or "") + ''
        perl scripts/config.pl set MBEDTLS_SSL_DTLS_SRTP
      '';
    });

    inherit (llvmPackages) openmp;

    libdatachannel = callPackage ./pkgs/libdatachannel.nix {};
    plog = callPackage ./pkgs/plog.nix {};
    libjuice = callPackage ./pkgs/libjuice.nix {};
    libsrtp = callPackage ./pkgs/libsrtp.nix {};

    steam-sdk = if coil.toolchain-steam != null then coil.toolchain-steam.sdk else null;
  });
  coil-core-nixos = nixos-pkgs.coil-core;

  # Ubuntu build
  ubuntu-pkgs = rec {
    clangVersion = "18";
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
      "libmbedtls-dev"
      "libpng-dev"
      "libsquish-dev"
      "libfreetype-dev"
      "libharfbuzz-dev"
      "libwayland-dev"
      "wayland-protocols"
      "libxkbcommon-dev"
      "libpulse-dev"
      "libogg-dev"
      "libwebm-dev"
      "libopus-dev"
      "libgav1-dev"
      "libcurl4-openssl-dev"
    ];
    coil-core = pkgs.vmTools.runInLinuxImage ((pkgs.callPackage ./coil-core.nix {
      inherit features;
      libsquish = null;
      libwebm = null;
      libgav1 = null;
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
})
