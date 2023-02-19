{ description = "Substitutes a directory elsewhere on macOS by replacing system calls";

  inputs."nixpkgs".url = github:NixOS/nixpkgs;
  inputs."utils".url = github:numtide/flake-utils;

  outputs = { self, nixpkgs, utils, ... }:
    utils.lib.eachSystem [ "x86_64-darwin" "aarch64-darwin" ]
    (system:
    let pkgs = import nixpkgs { inherit system; };
    in {
      packages = rec {
        default = fakedir;
        fakedir = pkgs.callPackage ./default.nix {};
        fakedir-universal = pkgs.stdenv.mkDerivation {
          inherit (fakedir) version;
          pname = "fakedir-universal";

          nativeBuildInputs = with pkgs;
          [ bintools-unwrapped ];
          installPhase = ''
            mkdir -p $out
            cp ${self.packages.x86_64-darwin.fakedir}/lib/libfakedir.dylib libfakedir.x86_64.dylib
            cp ${self.packages.aarch64-darwin.fakedir}/lib/libfakedir.dylib libfakedir.aarch64.dylib
            lipo -create -arch x86_64 libfakedir.x86_64.dylib -arch arm64 libfakedir.aarch64.dylib -output $out/libfakedir.dylib
          '';
        };
      };
    });
}
