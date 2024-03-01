{ lib,
  stdenv,
  gtest,
  lmdb,
  lmdbxx,
  hsql-parser,
}:

stdenv.mkDerivation rec {
  pname = "lmdb-lab";
  version = "0.0.1";

  src = ./.;

  buildInputs = [ lmdb lmdbxx hsql-parser ];
  nativeCheckInputs = [ gtest ];

  enableParallelBuilding = true;

  installPhase = ''
    install -Dm755 lmdb-lab -t "$out/bin/"
  '';

  doCheck = true;

  checkPhase = ''
    runHook preCheck
    make test
    ./test
    runHook postCheck
  '';

  meta = with lib; {
    homepage = "https://github.com/walicar/lmdb-lab";
    description = "A SQL Database using LMDB";
    platforms = platforms.unix;
  };
}
