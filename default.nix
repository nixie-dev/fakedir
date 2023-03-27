{ stdenv
, debug ? false
, ... }:
stdenv.mkDerivation {
  pname = "fakedir";
  version = "0.1.2";

  src = ./.;

  CFLAGS = if debug then "-g" else "-Ofast -DSTRIP_DEBUG";

  installPhase = ''
    install -Dm755 libfakedir.dylib $out/lib/libfakedir.dylib
  '';
}
