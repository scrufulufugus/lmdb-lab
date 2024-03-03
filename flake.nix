{
  description = "LMDB based SQL Database";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    sql-parser = {
      url = "github:scrufulufugus/sql-parser/upstream";
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
          lmdb-lab = pkgs.callPackage ./. { inherit hsql-parser; };
          default = lmdb-lab;
        };

        devShells.default = pkgs.mkShell {
          inputsFrom = [ self.packages.${system}.lmdb-lab ];
          packages = with pkgs; [ gnumake ];
        };
      });
}
