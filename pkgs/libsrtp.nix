{ stdenv
, fetchgit
, fetchpatch
, cmake
, ninja
, pkg-config
, mbedtls
}:

stdenv.mkDerivation rec {
  pname = "libsrtp";
  version = "2.7.0";

  src = fetchgit {
    url = "https://github.com/cisco/libsrtp.git";
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
