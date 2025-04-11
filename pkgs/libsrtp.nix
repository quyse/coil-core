{ stdenv
, fetchFromGitHub
, fetchpatch
, cmake
, ninja
, pkg-config
, mbedtls
}:

stdenv.mkDerivation rec {
  pname = "libsrtp";
  version = "2.7.0";

  src = fetchFromGitHub {
    owner = "cisco";
    repo = "libsrtp";
    rev = "v${version}";
    hash = "sha256-5AFsigie3YUrfvZYEIopjBJSNdoKoFlMBP9lv68+f6Q=";
  };

  nativeBuildInputs = [
    cmake
    ninja
    pkg-config
  ];

  buildInputs = [
    mbedtls
  ];

  cmakeFlags = [
    "-DENABLE_MBEDTLS=ON"
  ];
}
