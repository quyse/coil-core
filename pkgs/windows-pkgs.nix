{ pkgs
, lib
, coil
, dontRequireLibsList
}:

lib.makeExtensible (self: with self; {
  msvc = coil.toolchain-windows.msvc {};
  inherit (msvc) mkCmakePkg finalizePkg;

  nlohmann_json = mkCmakePkg {
    inherit (pkgs.nlohmann_json) pname version src;
    cmakeFlags = [
      "-DJSON_BuildTests=OFF"
    ];
  };
  vulkan-headers = mkCmakePkg {
    inherit (pkgs.vulkan-headers) pname version src;
  };
  vulkan-loader = mkCmakePkg {
    inherit (pkgs.vulkan-loader) pname version src;
    buildInputs = [
      vulkan-headers
    ];
    cmakeFlags = [
      "-DENABLE_WERROR=OFF"
    ];
  };
  spirv-headers = mkCmakePkg {
    inherit (pkgs.spirv-headers) pname version src;
  };
  zstd = mkCmakePkg {
    inherit (pkgs.zstd) pname version src;
    postPatch = ''
      # remove unsupported option
      grep -Fv -- '-z noexecstack' < build/cmake/CMakeModules/AddZstdCompilationFlags.cmake > build/cmake/CMakeModules/AddZstdCompilationFlags.cmake.new
      mv build/cmake/CMakeModules/AddZstdCompilationFlags.cmake{.new,}
    '';
    cmakeFlags = [
      "-DZSTD_MULTITHREAD_SUPPORT=OFF"
      "-DZSTD_BUILD_PROGRAMS=OFF"
      "-DZSTD_BUILD_TESTS=OFF"
    ];
    sourceDir = "build/cmake";
  };
  SDL2 = mkCmakePkg {
    inherit (pkgs.SDL2) pname version src;
    cmakeFlags = [
      "-DBUILD_SHARED_LIBS=ON"
    ];
  };
  zlib = mkCmakePkg {
    inherit (pkgs.zlib) pname version src;
  };
  libpng = mkCmakePkg {
    inherit (pkgs.libpng) pname version src;
    buildInputs = [
      zlib
    ];
    cmakeFlags = [
      "-DPNG_STATIC=OFF"
      "-DPNG_EXECUTABLES=OFF"
      "-DPNG_TESTS=OFF"
    ];
  };
  libsquish = mkCmakePkg rec {
    inherit (pkgs.libsquish) pname version src sourceRoot;
    cmakeFlags = [
      "-DBUILD_SHARED_LIBS=ON"
      "-DBUILD_SQUISH_WITH_OPENMP=OFF"
    ];
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
    cmakeFlags = [
      "-DBUILD_SHARED_LIBS=ON"
    ];
  };
  harfbuzz = mkCmakePkg rec {
    inherit (pkgs.harfbuzz) pname version src;
    buildInputs = [
      freetype
    ];
    cmakeFlags = [
      "-DBUILD_SHARED_LIBS=ON"
      "-DHB_HAVE_FREETYPE=ON"
    ];
  };
  ogg = mkCmakePkg {
    inherit (pkgs.libogg) pname version src;
    cmakeFlags = [
      "-DBUILD_SHARED_LIBS=ON"
    ];
  };
  opus = mkCmakePkg rec {
    pname = "opus";
    version = "1.4";
    # use git checkout instead of tarball to get cmake exports
    src = pkgs.fetchgit {
      url = "https://github.com/xiph/opus";
      rev = "v${version}";
      hash = "sha256-rA47xZB2mj2lhweA+pra1wjAjsFPMl2yMgX78jdn364=";
    };
    cmakeFlags = [
      "-DBUILD_SHARED_LIBS=ON"
      "-DOPUS_X86_MAY_HAVE_SSE4_1=OFF"
      "-DOPUS_X86_MAY_HAVE_AVX=OFF"
    ];
  };
  steam = if coil.toolchain-steam != null then coil.toolchain-steam.sdk.overrideAttrs (attrs: {
    installPhase = (attrs.installPhase or "") + (finalizePkg {
      buildInputs = [];
    });
  }) else null;
  coil-core = mkCmakePkg {
    name = "coil-core";
    inherit (pkgs.coil-core) src;
    buildInputs = [
      nlohmann_json
      vulkan-headers
      vulkan-loader
      spirv-headers
      zstd
      SDL2
      libpng
      libsquish
      sqlite
      freetype
      harfbuzz
      ogg
      opus
    ] ++ lib.optional (steam != null) steam;
    cmakeFlags = [
      "-DCOIL_CORE_DONT_REQUIRE_LIBS=${dontRequireLibsList}"
    ];
  };
})
