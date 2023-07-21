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
, libsquish
, freetype
, harfbuzz
, wayland
, wayland-protocols
, libxkbcommon
, pulseaudio
, libogg
, libwebm
, libopus
, libgav1
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
    libsquish
    freetype
    harfbuzz
    libogg
    libwebm
    libopus
    libgav1
    steam-sdk
  ] ++ lib.optionals stdenv.hostPlatform.isLinux [
    wayland
    wayland-protocols
    libxkbcommon
    pulseaudio
  ];
  doCheck = true;
  meta.license = lib.licenses.mit;
}
