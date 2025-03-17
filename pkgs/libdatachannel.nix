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
  version = "0.22.5";

  src = fetchgit {
    url = "https://github.com/paullouisageneau/libdatachannel.git";
    rev = "v${version}";
    hash = "sha256-6oJf7yHI47VOZtE2AKan+3GrcAgMMxJaZziNsSe7pdg=";
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
