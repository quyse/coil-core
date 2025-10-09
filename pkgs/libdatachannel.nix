{ stdenv
, fetchFromGitHub
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
  version = "0.23.2";

  src = fetchFromGitHub {
    owner = "paullouisageneau";
    repo = "libdatachannel";
    rev = "v${version}";
    hash = "sha256-76rlnO0wr7xQAGCc0GmpHQldzHKnZ8NZWpHmrE70h/c=";
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
