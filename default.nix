{ pkgsFun ? import <nixpkgs>
, pkgs ? pkgsFun {}
, lib ? pkgs.lib
}:

rec {
  coil-core = pkgs.callPackage ./coil-core.nix {};

  touch = {
    inherit coil-core;
  };
}
