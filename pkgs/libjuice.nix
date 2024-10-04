{ stdenv
, fetchgit
, cmake
}:

stdenv.mkDerivation rec {
  pname = "libjuice";
  version = "1.5.4";

  src = fetchgit {
    url = "https://github.com/paullouisageneau/libjuice.git";
    rev = "v${version}";
    hash = "sha256-NV3Bn+4FKkaboYr/vhXD0lhXjQtsTlTWJiphY/sMBXU=";
  };

  nativeBuildInputs = [
    cmake
  ];
}
