{
    description = "A very basic flake";

    inputs = {
        nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
    };

    outputs = { self, nixpkgs }: 
    let 
        system = "x86_64-linux";
        pkgs = import nixpkgs { inherit system; };
        StarPU = pkgs.callPackage ../../starpu.nix { maxBuffers = 56; };
        StarPURelease = pkgs.callPackage ../../starpu.nix { 
            maxBuffers = 56; 
            buildMode = "release";
        };

        baseShell = StarPUVersion: extraArgs: pkgs.mkShell ({
            buildInputs = with pkgs; [
                pkg-config
                hwloc
                # StarPU
                StarPUVersion
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
            STARPU_STORE_PATH = "${StarPUVersion}";
            CRITERION_STORE_PATH = "${pkgs.criterion.dev}";
            HWLOC_STORE_PATH = "${pkgs.hwloc.dev}";

            shellHook = ''
                export SHELL=/run/current-system/sw/bin/bash
                echo Added StarPU, Hwloc, criterion and gcc to ENV
                zsh
            '';
        } // extraArgs);
    in
    {
        devShells.${system} = {
            # default shell, compiles starpu in debug mode
            default = baseShell StarPU { COMPILE_MODE = "debug"; };
            release = baseShell StarPURelease { COMPILE_MODE = "release"; };
        };
    };
}
