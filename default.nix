{ pkgsFun ? import <nixpkgs>
, pkgs ? pkgsFun {}
, lib ? pkgs.lib
, toolchain
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

  # overlay with Coil Toolchain
  overlay = self: super: let
    stdenv = (toolchain.llvm14 rec {
      pkgs = super;
    }).stdenv;
  in ((toolchain.libs {
    inherit stdenv;
  }).overlay self super) // {
    coil-core = self.callPackage ./coil-core.nix {
      inherit stdenv;
    };
  };

  linuxGlibc = pkgsFun {
    overlays = [ overlay ];
  };

  touch = {
    inherit coil-core;
    linuxGlibcCoilCore = linuxGlibc.coil-core;
  };
}
