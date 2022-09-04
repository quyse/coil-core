{ pkgsFun ? import <nixpkgs>
, pkgs ? pkgsFun {}
, lib ? pkgs.lib
}:

rec {
  coil-core = pkgs.callPackage ./coil-core.nix {
    clang = pkgs.clang_14;
  };

  touch = {
    inherit coil-core;
  };
}
