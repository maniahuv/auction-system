{ pkgs }: {
	deps = [
   pkgs.acct
   pkgs.haskellPackages.servant-multipart-client
		pkgs.clang
		pkgs.ccls
		pkgs.gdb
		pkgs.gnumake
	];
}