{ pkgsFun ? import <nixpkgs>
, pkgs ? pkgsFun {}
, lib ? pkgs.lib
, toolchain-windows
, toolchain-steam
}:

rec {
  # NixOS build
  coil-core = (pkgs.callPackage ./coil-core.nix {
    steam = toolchain-steam.sdk;
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

  # Windows build
  windows-pkgs = let
    coil-core-nix = coil-core;
  in lib.makeExtensible (self: with self; {
    msvc = toolchain-windows.msvc {};
    inherit (msvc) mkCmakePkg finalizePkg;

    nlohmann_json = mkCmakePkg {
      inherit (pkgs.nlohmann_json) pname version src;
      cmakeFlags = "-DJSON_BuildTests=OFF";
    };
    vulkan-headers = mkCmakePkg {
      inherit (pkgs.vulkan-headers) pname version src;
    };
    vulkan-loader = mkCmakePkg {
      inherit (pkgs.vulkan-loader) pname version src;
      buildInputs = [
        vulkan-headers
      ];
      cmakeFlags = "-DENABLE_WERROR=OFF";
    };
    spirv-headers = mkCmakePkg {
      inherit (pkgs.spirv-headers) pname version src;
    };
    zstd = mkCmakePkg {
      inherit (pkgs.zstd) pname version src;
      cmakeFlags = "-DZSTD_MULTITHREAD_SUPPORT=OFF -DZSTD_BUILD_PROGRAMS=OFF -DZSTD_BUILD_TESTS=OFF";
      sourceDir = "build/cmake";
    };
    SDL2 = mkCmakePkg {
      inherit (pkgs.SDL2) pname version src;
      cmakeFlags = "-DBUILD_SHARED_LIBS=ON";
    };
    zlib = mkCmakePkg {
      inherit (pkgs.zlib) pname version src;
    };
    libpng = mkCmakePkg {
      inherit (pkgs.libpng) pname version src;
      buildInputs = [
        zlib
      ];
      cmakeFlags = "-DPNG_STATIC=OFF -DPNG_EXECUTABLES=OFF -DPNG_TESTS=OFF";
    };
    sqlite = mkCmakePkg {
      inherit (pkgs.sqlite) pname version src;
      postPatch = ''
        ln -s ${pkgs.writeText "sqlite-CMakeLists.txt" ''
          cmake_minimum_required(VERSION 3.19)
          project(sqlite)
          set(CMAKE_CXX_VISIBILITY_PRESET hidden)
          set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
          set(CMAKE_INTERPROCEDURAL_OPTIMIZAITON ON)
          set(CMAKE_CXX_EXTENSIONS OFF)
          add_library(sqlite STATIC sqlite3.c)
          set_property(TARGET sqlite PROPERTY PUBLIC_HEADER sqlite3.h)
          install(TARGETS sqlite EXPORT sqliteTargets ARCHIVE DESTINATION lib PUBLIC_HEADER DESTINATION include)
        ''} CMakeLists.txt
      '';
    };
    freetype = mkCmakePkg {
      inherit (pkgs.freetype) pname version src;
      buildInputs = [
        libpng
      ];
      cmakeFlags = "-DBUILD_SHARED_LIBS=ON";
    };
    harfbuzz = mkCmakePkg rec {
      inherit (pkgs.harfbuzz) pname version src;
      buildInputs = [
        freetype
      ];
      cmakeFlags = "-DBUILD_SHARED_LIBS=ON -DHB_HAVE_FREETYPE=ON";
    };
    ogg = mkCmakePkg {
      inherit (pkgs.libogg) pname version src;
      cmakeFlags = "-DBUILD_SHARED_LIBS=ON";
    };
    opus = mkCmakePkg rec {
      pname = "opus";
      version = "1.3.1";
      # use git checkout instead of tarball to get cmake exports
      src = pkgs.fetchgit {
        url = "https://github.com/xiph/opus";
        rev = "v${version}";
        hash = "sha256-DO9JAO6907VpOUgBiJ4WIZm9hTAYBM2Qabi+x1ibqN4=";
      };
      cmakeFlags = "-DBUILD_SHARED_LIBS=ON -DOPUS_X86_MAY_HAVE_SSE4_1=OFF -DOPUS_X86_MAY_HAVE_AVX=OFF";
    };
    steam = toolchain-steam.sdk.overrideAttrs (attrs: {
      installPhase = (attrs.installPhase or "") + (finalizePkg {
        buildInputs = [];
      });
    });
    coil-core = mkCmakePkg {
      name = "coil-core";
      inherit (coil-core-nix) src;
      buildInputs = [
        nlohmann_json
        vulkan-headers
        vulkan-loader
        spirv-headers
        zstd
        SDL2
        libpng
        sqlite
        freetype
        harfbuzz
        ogg
        opus
        steam
      ];
    };
  });
  coil-core-windows = windows-pkgs.coil-core;

  touch = {
    inherit coil-core coil-core-ubuntu coil-core-windows;
  };
}
