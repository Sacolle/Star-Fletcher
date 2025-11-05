{
    description = "A very basic flake";

    inputs = {
        nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
    };

    outputs = { self, nixpkgs }: 
    let 
        system = "x86_64-linux";
        starpuOverlay = f: p: {
            StarPU = p.callPackage ../../starpu.nix { maxBuffers = 10; };
        };
        pkgs = import nixpkgs {
            inherit system;
            overlays = [ starpuOverlay ];
        };
    in
    {
        devShells.${system}.default = pkgs.mkShell {
            buildInputs = with pkgs; [
                pkg-config
                hwloc
                StarPU
                # project libs
                criterion

                # for bash to work properlly inside vscode
                bashInteractive
                # for fuser
                psmisc
                # debug tools
                gdb
                gcc
                valgrind
            ];
            # export StarPU and hwloc store locations 
            # for use in vscode intellisence
            GCC_STORE_PATH = "${pkgs.gcc}";
            STARPU_STORE_PATH = "${pkgs.StarPU}";
            CRITERION_STORE_PATH = "${pkgs.criterion.dev}";
            HWLOC_STORE_PATH = "${pkgs.hwloc.dev}";

            shellHook = ''
                export SHELL=/run/current-system/sw/bin/bash
                echo Added StarPU, Hwloc, criterion and gcc to ENV
            '';
        };
    };
}
