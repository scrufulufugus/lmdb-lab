{
  description = "BerkeleyDB based SQL Database";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    sql-parser = {
      url = "github:scrufulufugus/sql-parser";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = { self, nixpkgs, flake-utils, sql-parser, ... }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in
      {
        packages = rec {
          hsql-parser = sql-parser.packages.${system}.default;
          cpsc5300 = pkgs.callPackage ./. { inherit hsql-parser; };
          default = cpsc5300;
        };

        devShells.default = pkgs.mkShell {
          inputsFrom = [ self.packages.${system}.default ];
          packages = with pkgs; [ gnumake gcc gdb valgrind clang-tools ];
        };
      });
}
