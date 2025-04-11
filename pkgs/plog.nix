{ stdenv
, fetchFromGitHub
, cmake
}:

stdenv.mkDerivation rec {
  pname = "plog";
  version = "1.1.10";

  src = fetchFromGitHub {
    owner = "SergiusTheBest";
    repo = "plog";
    rev = version;
    hash = "sha256-NZphrg9OB1FTY2ifu76AXeCyGwW2a2BkxMGjZPf4uM8=";
  };

  nativeBuildInputs = [
    cmake
  ];

  cmakeFlags = [
    "-DPLOG_BUILD_SAMPLES=OFF"
  ];
}
