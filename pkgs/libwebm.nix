{ stdenv
, lib
, fetchgit
, cmake
}:

stdenv.mkDerivation rec {
  pname = "libwebm";
  version = "1.0.0.32";
  src = fetchgit {
    url = "https://chromium.googlesource.com/webm/libwebm";
    rev = "libwebm-${version}";
    hash = "sha256-SxDGt7nPVkSxwRF/lMmcch1h+C2Dyh6GZUXoZjnXWb4=";
  };
  nativeBuildInputs = [
    cmake
  ];
  cmakeFlags = [
    "-DENABLE_WEBM_PARSER=ON"
  ];
  meta.license = lib.licenses.bsd3;
}
