{ lib,
  stdenv,
  gtest,
  db,
  hsql-parser,
}:

stdenv.mkDerivation rec {
  pname = "sql5300";
  version = "0.0.1";

  src = ./.;

  buildInputs = [ db hsql-parser ];
  nativeCheckInputs = [ gtest ];

  enableParallelBuilding = true;

  installPhase = ''
    install -Dm755 sql5300 -t "$out/bin/"
  '';

  doCheck = false;

  checkPhase = ''
    runHook preCheck
    make test
    ./test
    runHook postCheck
  '';

  meta = with lib; {
    homepage = "https://github.com/klundeen/5300-Antelope";
    description = "A SQL Database using BerkeleyDB";
    platforms = platforms.unix;
  };
}
