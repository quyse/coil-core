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
  version = "2.6.0";

  src = fetchgit {
    url = "https://github.com/cisco/libsrtp.git";
    rev = "v${version}";
    hash = "sha256-vWL5bksKT5NUoNkIRiJ2FeGODQthD8SgXjCaA7SeTe4=";
  };

  patches = [
    (fetchpatch {
      url = "https://github.com/cisco/libsrtp/pull/702.patch";
      hash = "sha256-BwKiY4CQ9lPyDTWVOtt1FKES4Sj9LdkGU6+EdqnlNbE=";
    })
  ];

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
