{
    description = "A very basic flake";

    inputs = {
        #nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-25.11";
        nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
        madagascar.url = "github:Sacolle/nix-madagascar";
    };

    outputs = { self, nixpkgs, madagascar }: 
    let 
        system = "x86_64-linux";
        fxtOverlay = f: p: {
            fxt = p.callPackage ../../fxt.nix { static = true; };
        };
        starpuOverlay = f: p: {
            StarPU = p.callPackage ../../starpu.nix { 
                enableCUDA = true; 
                maxBuffers = 56; 
                # enableTrace = true; 
            };
        };
        pkgs = import nixpkgs {
            inherit system;
            overlays = [ fxtOverlay starpuOverlay ];
            config = { 
                allowUnfree = true;
                cudaSupport = true;
                cudaVersion = "13";
            };
        };
        #StarPU = pkgs.callPackage ../../starpu.nix { 
        #    maxBuffers = 56; 
        #    enableTrace = true; 
        #    enableCUDA = true;
        #};
        #StarPURelease = pkgs.callPackage ../../starpu.nix { 
        #    maxBuffers = 56; 
        #    buildMode = "release";
        #   enableCUDA = true;
        #};

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
                # gcc
                valgrind
                # scripting
                python313
                python313Packages.numpy

                #madagascar
                madagascar.packages.${system}.default
            ];
            # export StarPU and hwloc store locations 
            # for use in vscode intellisence
            # GCC_STORE_PATH = "${pkgs.gcc}";
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
        packages.${system}.default = pkgs.StarPU;
        devShells.${system} = {
            # default shell, compiles starpu in debug mode
            default = baseShell pkgs.StarPU { COMPILE_MODE = "debug"; };
            # release = baseShell pkgs.StarPURelease { COMPILE_MODE = "release"; };
        };
    };
}
