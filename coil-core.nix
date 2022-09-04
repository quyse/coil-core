{ stdenv
, lib
, ninja
, clang
, writeText
, SDL2
, vulkan-headers
, spirv-headers
, nlohmann_json
}: let
  sources = lib.pipe ./src [
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
  src = ./src;
  nativeBuildInputs = [
    ninja
    clang
  ];
  buildInputs = [
    SDL2
    vulkan-headers
    spirv-headers
    nlohmann_json
  ];
  configurePhase = ''
    ln -s ${buildfile} build.ninja
  '';
  installPhase = ''
    mkdir $out
    mv *.a $out/
  '';
}
