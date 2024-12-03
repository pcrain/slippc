let pkgs = import <nixpkgs> {};
in { stdenv ? pkgs.stdenv, xz ? pkgs.xz }:
stdenv.mkDerivation {
  name = "slippc";
  src = ./.;
  buildInputs = [ xz ];

  installPhase = ''
    mkdir -p $out/bin
    cp ./slippc $out/bin/
  '';
}
