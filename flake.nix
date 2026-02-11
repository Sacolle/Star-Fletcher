{
    description = "A very basic flake";

    inputs = {
        nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
    };

    outputs = { self, nixpkgs }: 
    let 
        system = "x86_64-linux";
        starpuOverlay = f: p: {
            StarPU = p.callPackage ../../starpu.nix { enableCUDA = true; };
        };
        fxtOverlay = f: p: {
            fxt = p.callPackage ../../fxt.nix { static = true; };
        };
        pkgs = import nixpkgs {
            inherit system;
            overlays = [ starpuOverlay fxtOverlay ];
            config = { 
                allowUnfree = true;
                cudaSupport = true;
                cudaVersion = "13";
            };
        };
    in
    {
        packages.${system}.default = pkgs.StarPU;
        devShells.${system}.default = pkgs.mkShell {
            buildInputs = with pkgs; [
                pkg-config
                StarPU
                hwloc
                # for bash to work properlly inside vscode
                bashInteractive
                gdb
                gcc
                valgrind
            ];
            # export StarPU and hwloc store locations 
            # for use in vscode intellisence
            GCC_STORE_PATH = "${pkgs.gcc}";
            STARPU_STORE_PATH = "${pkgs.StarPU}";
            HWLOC_STORE_PATH = "${pkgs.hwloc.dev}";

            shellHook = ''
                export SHELL=/run/current-system/sw/bin/bash
                echo Added StarPU, Hwloc and gcc to ENV
                echo ${pkgs.fxt}
            '';
        };
    };
}
