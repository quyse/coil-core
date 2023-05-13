{ stdenv
, lib
, cmake
, ninja
, pkg-config
, writeText
, SDL2
, vulkan-headers
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
, pulseaudio
, libogg
, libopus
, steam-sdk
}:

stdenv.mkDerivation {
  name = "coil-core";
  src = ./src/coil;
  nativeBuildInputs = [
    cmake
    ninja
  ] ++ lib.optionals stdenv.hostPlatform.isLinux [
    pkg-config
  ];
  propagatedBuildInputs = [
    SDL2
    vulkan-headers
    vulkan-loader
    spirv-headers
    nlohmann_json
    zstd
    sqlite
    libpng
    freetype
    harfbuzz
    libogg
    libopus
    steam-sdk
  ] ++ lib.optionals stdenv.hostPlatform.isLinux [
    wayland
    wayland-protocols
    libxkbcommon
    pulseaudio
  ];
  doCheck = true;
}
