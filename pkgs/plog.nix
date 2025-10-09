{ stdenv
, fetchFromGitHub
, cmake
}:

stdenv.mkDerivation rec {
  pname = "plog";
  version = "1.1.11";

  src = fetchFromGitHub {
    owner = "SergiusTheBest";
    repo = "plog";
    rev = version;
    hash = "sha256-/H7qNL6aPjmFYk0X1sx4CCSZWrAMQgPo8I9X/P50ln0=";
  };

  nativeBuildInputs = [
    cmake
  ];

  cmakeFlags = [
    "-DPLOG_BUILD_SAMPLES=OFF"
  ];
}
