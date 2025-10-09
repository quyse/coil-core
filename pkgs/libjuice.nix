{ stdenv
, fetchFromGitHub
, cmake
}:

stdenv.mkDerivation rec {
  pname = "libjuice";
  version = "1.6.2";

  src = fetchFromGitHub {
    owner = "paullouisageneau";
    repo = "libjuice";
    rev = "v${version}";
    hash = "sha256-i+Hx0Qg9g35L5gg5OMVnitmQ84BnE+0IDXdvrLLdz8Y=";
  };

  nativeBuildInputs = [
    cmake
  ];
}
