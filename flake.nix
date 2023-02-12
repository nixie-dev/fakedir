{ description = "Substitutes a directory elsewhere on macOS by replacing system calls";

  inputs."nixpkgs".url = github:NixOS/nixpkgs;
  inputs."utils".url = github:numtide/flake-utils;

  outputs = { self, nixpkgs, utils, ... }:
    utils.lib.eachSystem [ "x86_64-darwin" "aarch64-darwin" ]
    (system:
    let pkgs = import nixpkgs { inherit system; };
    in {
      packages.fakedir = pkgs.callPackage ./default.nix {};
    });
}
