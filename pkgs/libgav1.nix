{ stdenv
, lib
, fetchgit
, cmake
}:

stdenv.mkDerivation rec {
  pname = "libgav1";
  version = "0.20.0";
  src = fetchgit {
    url = "https://chromium.googlesource.com/codecs/libgav1";
    rev = "v${version}";
    hash = "sha256-BgTfWmbcMvJB1KewJpRcMtbOd2FVuJ+fi1zAXBXfkrg=";
  };
  nativeBuildInputs = [
    cmake
  ];
  cmakeFlags = [
    # https://github.com/NixOS/nixpkgs/issues/144170
    "-DCMAKE_INSTALL_INCLUDEDIR=include"
    "-DCMAKE_INSTALL_LIBDIR=lib"
    # eliminate abseil dependency
    "-DLIBGAV1_THREADPOOL_USE_STD_MUTEX=1"
    "-DLIBGAV1_ENABLE_EXAMPLES=0"
    "-DLIBGAV1_ENABLE_TESTS=0"
  ];
  meta.license = lib.licenses.asl20;
}
