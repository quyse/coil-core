{ pkgsFun ? import <nixpkgs>
, pkgs ? pkgsFun {}
, lib ? pkgs.lib
, toolchain-windows
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

  # Windows build
  windows-pkgs = let
    coil-core-nix = coil-core;
  in lib.makeExtensible (self: with self; {
    msvc = toolchain-windows.msvc {};
    inherit (msvc) mkCmakePkg;

    nlohmann_json = mkCmakePkg {
      name = "nlohmann_json";
      inherit (pkgs.nlohmann_json) src;
      cmakeFlags = "-DJSON_BuildTests=OFF";
    };
    vulkan-headers = mkCmakePkg {
      name = "vulkan-headers";
      inherit (pkgs.vulkan-headers) src;
    };
    vulkan-loader = mkCmakePkg {
      name = "vulkan-loader";
      inherit (pkgs.vulkan-loader) src;
      buildInputs = [
        vulkan-headers
      ];
      cmakeFlags = "-DENABLE_WERROR=OFF";
    };
    spirv-headers = mkCmakePkg {
      name = "spirv-headers";
      inherit (pkgs.spirv-headers) src;
    };
    zstd = mkCmakePkg {
      name = "zstd";
      inherit (pkgs.zstd) src;
      cmakeFlags = "-DZSTD_MULTITHREAD_SUPPORT=OFF -DZSTD_BUILD_PROGRAMS=OFF -DZSTD_BUILD_TESTS=OFF";
      sourceDir = "build/cmake";
    };
    SDL2 = mkCmakePkg {
      name = "SDL2";
      inherit (pkgs.SDL2) src;
      cmakeFlags = "-DBUILD_SHARED_LIBS=ON";
    };
    zlib = mkCmakePkg {
      name = "zlib";
      src = pkgs.zlib.src;
    };
    libpng = mkCmakePkg {
      name = "libpng";
      inherit (pkgs.libpng) src;
      buildInputs = [
        zlib
      ];
      cmakeFlags = "-DPNG_STATIC=OFF -DPNG_EXECUTABLES=OFF -DPNG_TESTS=OFF";
    };
    sqlite = mkCmakePkg {
      name = "sqlite";
      inherit (pkgs.sqlite) src;
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
      name = "freetype";
      inherit (pkgs.freetype) src;
      buildInputs = [
        libpng
      ];
      cmakeFlags = "-DBUILD_SHARED_LIBS=ON";
    };
    harfbuzz = mkCmakePkg {
      name = "harfbuzz";
      # harfbuzz 6.0 is not in nixpkgs yet
      src = pkgs.fetchgit {
        url = "https://github.com/harfbuzz/harfbuzz.git";
        rev = "6.0.0";
        hash = "sha256-AnmpJjzbVE1KxuOdJOfNehQC+oxt1+4e6j8oGw1EDFk=";
      };
      buildInputs = [
        freetype
      ];
      cmakeFlags = "-DBUILD_SHARED_LIBS=ON -DHB_HAVE_FREETYPE=ON";
    };
    ogg = mkCmakePkg {
      name = "ogg";
      inherit (pkgs.libogg) src;
      cmakeFlags = "-DBUILD_SHARED_LIBS=ON";
    };
    opus = mkCmakePkg {
      name = "opus";
      # use git checkout instead of tarball to get cmake exports
      src = pkgs.fetchgit {
        url = "https://github.com/xiph/opus";
        rev = "v1.3.1";
        hash = "sha256-DO9JAO6907VpOUgBiJ4WIZm9hTAYBM2Qabi+x1ibqN4=";
      };
      cmakeFlags = "-DBUILD_SHARED_LIBS=ON -DOPUS_X86_MAY_HAVE_SSE4_1=OFF -DOPUS_X86_MAY_HAVE_AVX=OFF";
    };
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
        zlib
        libpng
        sqlite
        freetype
        harfbuzz
        ogg
        opus
      ];
      doCheck = false;
    };
  });
  coil-core-windows = windows-pkgs.coil-core;

  touch = {
    inherit coil-core coil-core-ubuntu coil-core-windows;
  };
}
