{
  description = "LMDB based SQL Database";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils, ... }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in
      {
        packages = rec {
          hsql-parser = pkgs.callPackage ./nix/hsql-parser.nix { };
          lmdb-lab = pkgs.callPackage ./. { inherit hsql-parser; };
          default = lmdb-lab;
        };

        devShells.default = pkgs.mkShell {
          inputsFrom = [ self.packages.${system}.lmdb-lab ];
          packages = with pkgs; [ gnumake ];
        };
      });
}
