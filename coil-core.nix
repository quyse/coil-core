{ stdenv
, lib
, clang
, cmake
, pkg-config
, writeText
, SDL2
, vulkan-headers
, spirv-headers
, nlohmann_json
, libpng
, freetype
, harfbuzz
}: let
  sources = lib.pipe ./src/coil [
    builtins.readDir
    (lib.filterAttrs (file: type: lib.hasSuffix ".cpp" file && type == "regular"))
    lib.attrNames
  ];
  buildfile = writeText "coil-core.ninja" ''
    rule cxx
      command = clang++ -c -std=c++20 -o $out $in
    rule ar
      command = ar -r $out $in
    ${lib.concatStrings (map (source: ''
      build ${source}.o: cxx ${source}
    '') sources)}
    build libcoilcore.a: ar ${lib.concatStringsSep " " (map (source: "${source}.o") sources)}
  '';
in stdenv.mkDerivation {
  name = "coil-core";
  src = ./src/coil;
  nativeBuildInputs = [
    clang
    cmake
    pkg-config
  ];
  buildInputs = [
    SDL2
    vulkan-headers
    spirv-headers
    nlohmann_json
    libpng
    freetype
    harfbuzz
  ];
  cmakeFlags = [
    "-DCMAKE_CXX_COMPILER=clang++"
    "-DCMAKE_C_COMPILER=clang"
  ];
  installPhase = ''
    mkdir $out
    mv *.a $out/
  '';
}
