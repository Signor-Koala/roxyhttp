# default.nix
with import <nixpkgs> {};
  stdenv.mkDerivation {
    name = "dev-environment"; # Probably put a more meaningful name here
    buildInputs = [gcc gnumake cmake libbsd lua54Packages.lua lua54Packages.luasql-sqlite3];
  }
