{ stdenv
, lib
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
  doCheck = true;
}
