{ pkgs ? import <nixpkgs> {}
, lib ? pkgs.lib
, coil
, features ? null
}:

lib.makeExtensible (self: with self; {
  # NixOS build
  nixos-pkgs = pkgs.extend (self: super: with self; {
    # expose compiler
    coil.llvmPackages = llvmPackages_21;
    coil.clang-tools = self.coil.llvmPackages.clang-tools.override { enableLibcxx = true; };
    coil.clang = self.coil.llvmPackages.clangUseLLVM.override {
      inherit (self.coil.llvmPackages) bintools; # use lld
    };
    # function applying adjustments to a package
    coil.compile-cpp = pkg: pkg.overrideAttrs (attrs: {
      nativeBuildInputs = (attrs.nativeBuildInputs or []) ++ [
        ninja
        self.coil.clang-tools # for C++ modules to work; must be before clang
        self.coil.clang
      ];
      cmakeFlags = (attrs.cmakeFlags or []) ++ [
        # force clang
        "-DCMAKE_CXX_COMPILER=clang++"
        "-DCMAKE_C_COMPILER=clang"
        # compile flags
        "-DCMAKE_CXX_FLAGS=${lib.concatStringsSep " " [
          # for std::jthread in libc++
          "-fexperimental-library"
          # work around linking issue https://github.com/NixOS/nixpkgs/issues/371540
          "-fno-builtin"
        ]}"
        # optimization
        "-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON"
      ];
      __structuredAttrs = true;
    });

    coil.core = self.coil.compile-cpp ((callPackage ./coil-core.nix {
      inherit features;
    }).overrideAttrs (attrs: {
      cmakeFlags = (attrs.cmakeFlags or []) ++ [
        # do not require some libs
        "-DCOIL_CORE_DONT_REQUIRE_LIBS=${dontRequireLibsList}"
      ];
    }));


    libsquish = self.coil.compile-cpp (callPackage ./pkgs/libsquish.nix {});

    libwebm = self.coil.compile-cpp (callPackage ./pkgs/libwebm.nix {});
    libgav1 = self.coil.compile-cpp (callPackage ./pkgs/libgav1.nix {});

    mbedtls = self.coil.compile-cpp (super.mbedtls.overrideAttrs (attrs: {
      postConfigure = (attrs.postConfigure or "") + ''
        perl scripts/config.pl set MBEDTLS_SSL_DTLS_SRTP
      '';
    }));

    inherit (self.coil.llvmPackages) openmp;

    libdatachannel = self.coil.compile-cpp (callPackage ./pkgs/libdatachannel.nix {});
    plog = self.coil.compile-cpp (callPackage ./pkgs/plog.nix {});
    libjuice = self.coil.compile-cpp (callPackage ./pkgs/libjuice.nix {});
    libsrtp = self.coil.compile-cpp (callPackage ./pkgs/libsrtp.nix {});

    steam-sdk = if coil.toolchain-steam != null then coil.toolchain-steam.sdk else null;
  });
  coil-core-nixos = nixos-pkgs.coil.core;

  # Ubuntu build
  ubuntu-pkgs = rec {
    clangVersion = "21";
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
    inherit
      coil-core-nixos
      coil-core-ubuntu
      coil-core-windows
    ;
    boost-windows = windows-pkgs.boost;
  };
})
