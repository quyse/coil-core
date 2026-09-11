{ stdenv
, lib
, fetchurl
, cmake
}:

stdenv.mkDerivation rec {
  pname = "libsquish";
  version = "1.15";
  src = fetchurl {
    url = "https://downloads.sourceforge.net/libsquish/libsquish-${version}.tgz";
    hash = "sha256-YoeW7rpgiGYYOmHQgNRpZ8ndpnI7wKPsUjJMhdIUcmk=";
  };
  sourceRoot = ".";
  postPatch = ''
    substituteInPlace CMakeLists.txt --replace-fail 'CMAKE_MINIMUM_REQUIRED(VERSION 2.8.3)' 'CMAKE_MINIMUM_REQUIRED(VERSION 3.28.2)'
  '';
  nativeBuildInputs = [
    cmake
  ];
  cmakeFlags = [
    "-DBUILD_SHARED_LIBS=ON"
  ];
  meta.license = lib.licenses.mit;
}
