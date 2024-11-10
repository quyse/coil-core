{ stdenv
, fetchgit
, cmake
}:

stdenv.mkDerivation rec {
  pname = "libjuice";
  version = "1.5.7";

  src = fetchgit {
    url = "https://github.com/paullouisageneau/libjuice.git";
    rev = "v${version}";
    hash = "sha256-niMzFimcb+6FJq9ks7FJ2yrwJ7NnSEfj/bxLaPhdMDk=";
  };

  nativeBuildInputs = [
    cmake
  ];
}
