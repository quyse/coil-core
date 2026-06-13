{ pkgs
, lib
, coil
, dontRequireLibsList
, fixeds
}:

lib.makeExtensible (self: with self; {
  inherit (coil.toolchain-windows) msvc;
  inherit (msvc) mkCmakePkg finalizePkg buildEnvWithModulesSupport;

  nlohmann_json = mkCmakePkg {
    inherit (pkgs.nlohmann_json) pname version src meta;
    cmakeFlags = [
      "-DJSON_BuildTests=OFF"
    ];
  };

  vulkan-headers = mkCmakePkg {
    inherit (pkgs.vulkan-headers) pname version src meta;
    postPatch = ''
      sed -ie 's?-Werror??;s?/WX??' tests/integration/CMakeLists.txt
    '';
    cmakeFlags = [
      "-DVULKAN_HEADERS_ENABLE_MODULE=OFF"
    ];
  };

  vulkan-loader = mkCmakePkg {
    inherit (pkgs.vulkan-loader) pname version src meta;
    buildInputs = [
      vulkan-headers
    ];
    cmakeFlags = [
      "-DENABLE_WERROR=OFF"
    ];
  };

  spirv-headers = mkCmakePkg {
    inherit (pkgs.spirv-headers) pname version src meta;
  };

  zstd = mkCmakePkg {
    name = "zstd";
    src = pkgs.fetchgit {
      inherit (fixeds.fetchgit."https://github.com/facebook/zstd.git##latest_release") url rev sha256;
    };
    cmakeFlags = [
      "-DZSTD_MULTITHREAD_SUPPORT=OFF"
      "-DZSTD_BUILD_PROGRAMS=OFF"
      "-DZSTD_BUILD_TESTS=OFF"
    ];
    sourceDir = "build/cmake";
    meta.license = lib.licenses.bsd3;
  };

  sdl3 = mkCmakePkg {
    inherit (pkgs.sdl3) pname version src meta;
    cmakeFlags = [
      "-DCMAKE_DISABLE_PRECOMPILE_HEADERS=ON"
      "-DSDL_TESTS=OFF"
    ];
  };

  zlib = mkCmakePkg {
    name = "zlib";
    src = pkgs.fetchgit {
      inherit (fixeds.fetchgit."https://github.com/madler/zlib.git##latest_release") url rev sha256;
    };
    cmakeFlags = [
      "-DZLIB_BUILD_SHARED=ON"
      "-DZLIB_BUILD_STATIC=OFF"
    ];
    meta.license = lib.licenses.zlib;
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
    meta = pkgs.libpng.meta // {
      outputsToInstall = null;
    };
  };

  libsquish = mkCmakePkg rec {
    inherit (pkgs.libsquish) pname version src sourceRoot postPatch meta;
    cmakeFlags = [
      "-DBUILD_SQUISH_WITH_OPENMP=OFF"
    ];
  };

  sqlite = mkCmakePkg rec {
    pname = "sqlite";
    version = "3510200";
    src = pkgs.fetchzip {
      url = "https://sqlite.org/2026/sqlite-amalgamation-${version}.zip";
      hash = "sha256-WUH/22OMthEsDTGrpZ8dLCts6890wSLGMmPWt725Gss=";
    };
    postPatch = ''
      ln -s ${pkgs.writeText "sqlite-CMakeLists.txt" ''
        cmake_minimum_required(VERSION 3.19)
        project(sqlite)
        set(CMAKE_CXX_VISIBILITY_PRESET hidden)
        set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
        set(CMAKE_CXX_EXTENSIONS OFF)
        add_library(sqlite STATIC sqlite3.c)
        set_property(TARGET sqlite PROPERTY PUBLIC_HEADER sqlite3.h)
        install(TARGETS sqlite EXPORT sqliteTargets ARCHIVE DESTINATION lib PUBLIC_HEADER DESTINATION include)
      ''} CMakeLists.txt
    '';
    meta = pkgs.sqlite.meta // {
      outputsToInstall = null;
    };
  };

  mbedtls = mkCmakePkg {
    inherit (pkgs.mbedtls) pname version src meta;
    cmakeFlags = [
      "-DMBEDTLS_FATAL_WARNINGS=OFF"
      "-DENABLE_PROGRAMS=OFF"
      "-DENABLE_TESTING=OFF"
    ];
    postPatch = ''
      echo '#undef MBEDTLS_AESNI_C' >> include/mbedtls/mbedtls_config.h
      echo '#define MBEDTLS_SSL_DTLS_SRTP' >> include/mbedtls/mbedtls_config.h
    '';
  };

  freetype = mkCmakePkg {
    inherit (pkgs.freetype) pname version src meta;
    buildInputs = [
      libpng
    ];
    cmakeFlags = [
      "-DBUILD_SHARED_LIBS=ON"
    ];
  };

  harfbuzz = mkCmakePkg rec {
    inherit (pkgs.harfbuzz) pname version src meta;
    buildInputs = [
      freetype
    ];
    cmakeFlags = [
      "-DBUILD_SHARED_LIBS=ON"
      "-DHB_HAVE_FREETYPE=ON"
    ];
  };

  libdatachannel = mkCmakePkg rec {
    inherit (pkgs.libdatachannel) pname version src;

    postPatch = ''
      sed -ie 's/JUICE_INCLUDE_DIRS/JUICE_INCLUDE_DIR/' cmake/Modules/FindLibJuice.cmake
      # workaround for mbedtls bug
      sed -ie 's/PRIVATE MbedTLS::MbedTLS/PRIVATE MbedTLS::MbedTLS bcrypt.lib/' CMakeLists.txt
    '';

    buildInputs = [
      plog
      usrsctp
      mbedtls
      libjuice
      nlohmann_json
    ];

    cmakeFlags = [
      # do not use submodules
      "-DPREFER_SYSTEM_LIB=ON"
      # exclude unsupported stuff
      "-DNO_WEBSOCKET=ON"
      "-DNO_MEDIA=ON"
      # use mbedtls for crypto
      "-DUSE_MBEDTLS=ON"
      "-DUsrsctp_INCLUDE_DIR=${usrsctp}/include"
      "-DUsrsctp_LIBRARY=${usrsctp}/lib/usrsctp.lib"
    ];
  };
  plog = mkCmakePkg {
    inherit (pkgs.plog) pname version src;
    cmakeFlags = [
      "-DPLOG_BUILD_SAMPLES=OFF"
    ];
  };
  usrsctp = mkCmakePkg {
    inherit (pkgs.usrsctp) pname version src patches;
    cmakeFlags = [
      "-Dsctp_werror=OFF"
      # "-Dsctp_build_shared_lib=ON"
      "-Dsctp_build_programs=OFF"
    ];
  };
  libjuice = mkCmakePkg {
    inherit (pkgs.libjuice) pname version src;
  };

  ogg = mkCmakePkg {
    inherit (pkgs.libogg) pname version src meta;
    cmakeFlags = [
      "-DBUILD_SHARED_LIBS=ON"
    ];
  };

  opus = mkCmakePkg rec {
    pname = "opus";
    version = "1.5.2";
    # use git checkout instead of tarball to get cmake exports
    src = pkgs.fetchFromGitHub {
      owner = "xiph";
      repo = "opus";
      rev = "v${version}";
      hash = "sha256-M1G7ypcfs7nJmXgkyoG96jT/CkgN5BOzy+DGO4LVCvA=";
    };
    cmakeFlags = [
      "-DBUILD_SHARED_LIBS=ON"
      "-DOPUS_X86_MAY_HAVE_SSE4_1=OFF"
      "-DOPUS_X86_MAY_HAVE_AVX2=OFF"
    ];
    meta.license = lib.licenses.bsd3;
  };

  libwebm = mkCmakePkg rec {
    inherit (pkgs.libwebm) pname version src meta;
    cmakeFlags = [
      "-DBUILD_SHARED_LIBS=ON"
      "-DENABLE_WEBM_PARSER=ON"
      "-DENABLE_WEBMINFO=OFF"
      "-DENABLE_SAMPLE_PROGRAMS=OFF"
    ];
  };

  libgav1 = mkCmakePkg rec {
    inherit (pkgs.libgav1) pname version src meta;
    postPatch = ''
      sed -ie 's?-Wall??;s?-Wextra??' cmake/libgav1_build_definitions.cmake
      sed -ie 's?__declspec(dllimport)??' src/gav1/symbol_visibility.h
      cat <<'EOF' > cmake/libgav1_install.cmake
      macro(libgav1_setup_install_target)
        include(GNUInstallDirs)
        install(
          FILES ''${libgav1_api_includes}
          DESTINATION "''${CMAKE_INSTALL_INCLUDEDIR}/gav1"
        )
        install(
          TARGETS libgav1_shared
        )
      endmacro()
      EOF
    '';
    cmakeFlags = [
      "-DBUILD_SHARED_LIBS=ON"
      # eliminate abseil dependency
      "-DLIBGAV1_THREADPOOL_USE_STD_MUTEX=1"
      "-DLIBGAV1_ENABLE_EXAMPLES=0"
      "-DLIBGAV1_ENABLE_TESTS=0"
    ];
  };

  curl = mkCmakePkg {
    inherit (pkgs.curl) pname version src;
    buildInputs = [
      c-ares
      mbedtls
      zlib
      zstd
    ];
    cmakeFlags = [
      "-DBUILD_CURL_EXE=OFF"
      "-DBUILD_SHARED_LIBS=ON"
      "-DENABLE_ARES=ON"
      "-DENABLE_UNICODE=ON"
      "-DCURL_LTO=ON"
      "-DUSE_ZLIB=ON"
      "-DCURL_ZSTD=ON"
      "-DCURL_USE_MBEDTLS=ON"
      "-DCURL_USE_LIBPSL=OFF"
    ];
    meta = pkgs.curl.meta // {
      outputsToInstall = null;
    };
  };

  c-ares = mkCmakePkg {
    inherit (pkgs.c-ares) pname version src;
    meta = pkgs.c-ares.meta // {
      outputsToInstall = null;
    };
  };

  boost = mkCmakePkg {
    name = "boost";
    src = pkgs.fetchgit {
      inherit (fixeds.fetchgit."https://github.com/boostorg/boost.git##latest_release") url rev sha256;
      fetchSubmodules = true;
    };
    buildInputs = [
      zlib
      zstd
    ];
    cmakeFlags = [
      "-DBUILD_SHARED_LIBS=ON"
      "-DBOOST_ENABLE_PYTHON=OFF"
      "-DBOOST_IOSTREAMS_ENABLE_ZLIB=ON"
      "-DBOOST_IOSTREAMS_ENABLE_ZSTD=ON"
    ];
    doCheck = false;
    meta.license = lib.licenses.boost;
  };

  steam = if coil.toolchain-steam != null then coil.toolchain-steam.sdk.overrideAttrs (attrs: {
    installPhase = (attrs.installPhase or "") + (finalizePkg {
      buildInputs = [];
    });
  }) else null;

  coil-core = mkCmakePkg {
    inherit (pkgs.coil.core) name src meta;
    buildEnv = buildEnvWithModulesSupport;
    buildInputs = [
      nlohmann_json
      vulkan-headers
      vulkan-loader
      spirv-headers
      zstd
      sdl3
      libpng
      libsquish
      sqlite
      mbedtls
      freetype
      harfbuzz
      libdatachannel
      ogg
      libwebm
      opus
      libgav1
      curl
    ] ++ lib.optional (steam != null) steam;
    cmakeFlags = [
      "-DCOIL_CORE_DONT_REQUIRE_LIBS=${dontRequireLibsList}"
    ];
    postPatch = ''
      sed -iE 's/cxx_std_26/cxx_std_23/' CMakeLists.txt
      sed -iE 's/CMAKE_CXX_STANDARD 26/CMAKE_CXX_STANDARD 23/' CMakeLists.txt
    '';
  };
})
