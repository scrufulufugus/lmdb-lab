{ lib,
  stdenv,
  fetchFromGitHub,
  #makeWrapper,
  bison,
  flex
}:

stdenv.mkDerivation rec {
  pname = "sql-parser";
  version = "31aaf8d7a45e0ef76b5fafe090418a47a0f7f355";

  src = fetchFromGitHub {
    owner = "klundeen";
    repo = "sql-parser";
    rev = version;
    sha256 = "sha256-+tlUGhG7+BojB1AICBnU6/zjSib2Lsy6BnWYTF8YkH0=";
  };

  buildInputs = [ bison flex ];

  enableParallelBuilding = true;

  installPhase = ''
    install -Dm644 src/*.h -t "$out/include/hsql/"
    install -Dm644 src/sql/*.h -t "$out/include/hsql/sql/"
    install -Dm755 libsqlparser.so -t "$out/lib/"
  '';

  doCheck = false;

  checkPhase = ''
    runHook preCheck
    make test_install
    runHook postCheck
  '';

  meta = with lib; {
    homepage = "https://github.com/klundeen/sql-parser";
    description = "C++ SQL Parser";
    platforms = platforms.unix;
  };
}
