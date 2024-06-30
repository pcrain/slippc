let pkgs = import <nixpkgs> {};
in { stdenv ? pkgs.stdenv, xz ? pkgs.xz }:
stdenv.mkDerivation {
  name = "slippc";
  src = ./.;
  buildDepends = [ xz ];

  installPhase = ''
    mkdir -p $out/bin
    cp ./slippc $out/bin/
  '';
}
