{ stdenv
, fetchgit
, cmake
}:

stdenv.mkDerivation rec {
  pname = "libjuice";
  version = "1.5.2";

  src = fetchgit {
    url = "https://github.com/paullouisageneau/libjuice.git";
    rev = "v${version}";
    hash = "sha256-e0HenTl5hGTyU+ljFZa5fJ9q/ZwL6/9o0DqdbC0gP7A=";
  };

  nativeBuildInputs = [
    cmake
  ];
}
