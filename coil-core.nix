{ stdenv
, lib
, ninja
, writeText
}: let
  libraries = [
    "base"
    "data"
  ];
  buildfile = writeText "coil-core.ninja" ''
    rule cxx
      command = gcc -c -std=c++17 -o $out $in
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
  configurePhase = ''
    ln -s ${buildfile} build.ninja
  '';
  installPhase = ''
    mkdir $out
    mv *.a $out/
  '';
}
