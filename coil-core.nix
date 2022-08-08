{ stdenv
, lib
, ninja
, writeText
, SDL2
}: let
  libraries = [
    "base"
    "data"
  ];
  buildfile = writeText "coil-core.ninja" ''
    rule cxx
      command = gcc -c -std=c++20 -o $out $in
    rule ar
      command = ar -r $out $in
    ${lib.concatStrings (map (library: ''
      build ${library}.o: cxx ${library}.cpp
      build libcoil-${library}.a: ar ${library}.o
    '') libraries)}
  '';
in stdenv.mkDerivation {
  name = "coil-core";
  src = ./src;
  nativeBuildInputs = [
    ninja
  ];
  buildInputs = [
    SDL2
  ];
  configurePhase = ''
    ln -s ${buildfile} build.ninja
  '';
  installPhase = ''
    mkdir $out
    mv *.a $out/
  '';
}
