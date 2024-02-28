{ stdenv
, lib
, features ? null
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
, mbedtls
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
, curl
, steam-sdk
}: let

  hasFeature = feature: if features == null then true else features."${feature}" or false;

in stdenv.mkDerivation {
  name = "coil-core";
  src = ./src/coil;
  nativeBuildInputs = [
    cmake
    ninja
  ] ++ lib.optionals stdenv.buildPlatform.isLinux [
    pkg-config
  ];
  propagatedBuildInputs = [
    nlohmann_json
    zstd
    sqlite
    mbedtls
  ]
  ++ lib.optionals (hasFeature "graphics") [
    SDL2
    vulkan-headers
    vulkan-loader
    spirv-headers
    libpng
    libsquish
    freetype
    harfbuzz
  ]
  ++ lib.optionals (hasFeature "audio") [
    libogg
    libopus
  ]
  ++ lib.optionals (hasFeature "video") [
    libwebm
    libgav1
  ]
  ++ lib.optionals (hasFeature "network") [
    curl
  ]
  ++ lib.optionals (hasFeature "steam") [
    steam-sdk
  ]
  ++ lib.optionals (hasFeature "graphics" && stdenv.hostPlatform.isLinux) [
    wayland
    wayland-protocols
    libxkbcommon
  ]
  ++ lib.optionals (hasFeature "audio" && stdenv.hostPlatform.isLinux) [
    pulseaudio
  ];
  cmakeFlags = lib.optional (features != null) "-DCOIL_CORE_REQUIRE_LIBS=";
  doCheck = true;
  meta.license = lib.licenses.mit;
}
