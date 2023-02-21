{ stdenv, ... }:
stdenv.mkDerivation {
  pname = "fakedir";
  version = "0.1.0";

  src = ./.;

  installPhase = ''
    install -Dm755 libfakedir.dylib $out/lib/libfakedir.dylib
  '';
}
