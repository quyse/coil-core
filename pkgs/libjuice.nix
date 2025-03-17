{ stdenv
, fetchgit
, cmake
}:

stdenv.mkDerivation rec {
  pname = "libjuice";
  version = "1.5.9";

  src = fetchgit {
    url = "https://github.com/paullouisageneau/libjuice.git";
    rev = "v${version}";
    hash = "sha256-31l/bcvmg1x8HJ4mrIaiCHWIqwrK/XQYJxee8S3+2f8=";
  };

  nativeBuildInputs = [
    cmake
  ];
}
