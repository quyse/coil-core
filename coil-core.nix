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
, wayland
, wayland-protocols
, libxkbcommon
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
  ] ++ lib.optionals stdenv.hostPlatform.isLinux [
    wayland
    wayland-protocols
    libxkbcommon
  ];
  doCheck = true;
}
