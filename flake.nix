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
        packages = {
          hsql-parser = pkgs.callPackage ./nix/hsql-parser.nix { };
        };

        devShells.default = pkgs.mkShell {
          inputsFrom = [ ];
          packages = with pkgs; [ self.packages.${system}.hsql-parser ];
        };
      });
}
