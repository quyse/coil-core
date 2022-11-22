{ stdenv
, lib
, clang
, cmake
, pkg-config
, writeText
, SDL2
, vulkan-loader
, spirv-headers
, nlohmann_json
, zstd
, sqlite
, libpng
, freetype
, harfbuzz
}:

stdenv.mkDerivation {
  name = "coil-core";
  src = ./src/coil;
  nativeBuildInputs = [
    clang
    cmake
    pkg-config
  ];
  buildInputs = [
    SDL2
    vulkan-loader
    spirv-headers
    nlohmann_json
    zstd
    sqlite
    libpng
    freetype
    harfbuzz
  ];
  cmakeFlags = [
    "-DCMAKE_CXX_COMPILER=clang++"
    "-DCMAKE_C_COMPILER=clang"
  ];
}
