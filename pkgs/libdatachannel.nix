{ stdenv
, fetchgit
, cmake
, plog
, usrsctp
, mbedtls
, libjuice
, libsrtp
, nlohmann_json
}:

stdenv.mkDerivation rec {
  pname = "libdatachannel";
  version = "0.21.2";

  src = fetchgit {
    url = "https://github.com/paullouisageneau/libdatachannel.git";
    rev = "v${version}";
    hash = "sha256-3fax57oaJvOgbTDPCiiUdtsfAGhICfPkuMihawq06SA=";
    fetchSubmodules = false;
  };

  nativeBuildInputs = [
    cmake
  ];

  buildInputs = [
    plog
    usrsctp
    mbedtls
    libjuice
    libsrtp
    nlohmann_json
  ];

  cmakeFlags = [
    # do not use submodules
    "-DPREFER_SYSTEM_LIB=ON"
    # use mbedtls for crypto
    "-DUSE_MBEDTLS=ON"
  ];
}
