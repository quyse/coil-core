{ stdenv
, fetchFromGitHub
, cmake
}:

stdenv.mkDerivation rec {
  pname = "libjuice";
  version = "1.6.0";

  src = fetchFromGitHub {
    owner = "paullouisageneau";
    repo = "libjuice";
    rev = "v${version}";
    hash = "sha256-fC92Pf0jyL2pUcJFyl7AqjZHcnQEV1cknDwwUiWB1bk=";
  };

  nativeBuildInputs = [
    cmake
  ];
}
