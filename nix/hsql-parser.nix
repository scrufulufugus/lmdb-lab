{ lib,
  stdenv,
  fetchFromGitHub,
  makeWrapper,
  bison,
  flex
}:

stdenv.mkDerivation rec {
  pname = "hsql-parser";
  version = "c2471248cef8cd33081e698e8ac65d691283dbd4";

  src = fetchFromGitHub {
    owner = "hyrise";
    repo = "sql-parser";
    rev = version;
    sha256 = "sha256-TPySZmHdL/JVGTsmHmHexPLuHGVD8tqgmZGhw7wGhjE=";
  };

  buildInputs = [ bison flex ];
  nativeBuildInputs = [ makeWrapper ];

  enableParallelBuilding = true;

  preInstall = ''
    sed -i "s|^\(INSTALL\s*=\s*\).*|\1$out|g" Makefile;
    mkdir -p $out/{lib,include};
  '';

  doCheck = false;

  checkPhase = ''
    runHook preCheck
    make test_install
    runHook postCheck
  '';

  meta = with lib; {
    homepage = "https://github.com/hyrise/sql-parser";
    description = "C++ SQL Parser";
    platforms = platforms.unix;
  };
}
