{
    description = "A very basic flake";

    inputs = {
        nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
        # gets the proper version of the CUDA packages for compilation
        cudaNixpkgs.url = "github:nixos/nixpkgs/1da52dd49a127ad74486b135898da2cef8c62665";

        madagascar.url = "github:Sacolle/nix-madagascar";
    };

    outputs = { self, nixpkgs, cudaNixpkgs, madagascar }: 
    let 
        system = "x86_64-linux";
        # import gcc13Stdenv from this because it uses the correct version of glibc (2.40-36)
        cudapkgs = import cudaNixpkgs { 
            inherit system; 
            config = { 
                allowUnfree = true;
                cudaSupport = true;
                cudaVersion = "13";
            };
        };

        starpuOverlay = f: p: {
            StarPU = p.callPackage ../../starpu.nix { 
                enableCUDA = true; 
                cudaPackages  = cudapkgs.cudaPackages;
                linuxPackages = cudapkgs.linuxPackages;

                maxBuffers = 56;
                enableTrace = true;
            };
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
            ] ++ (with cudapkgs.cudaPackages; [ 
                cuda_cudart
                cuda_nvcc
                cuda_nvml_dev.dev
                libcusparse.dev
            ]);
            # export StarPU and hwloc store locations 
            # for use in vscode intellisence
            STARPU_STORE_PATH = "${StarPUVersion}";
            CRITERION_STORE_PATH = "${pkgs.criterion.dev}";
            HWLOC_STORE_PATH = "${pkgs.hwloc.dev}";

            # cudas
            CUDART_STORE_PATH =      "${cudapkgs.cudaPackages.cuda_cudart.dev}";
            NVCC_STORE_PATH =        "${cudapkgs.cudaPackages.cuda_nvcc}";
            NVML_STORE_PATH =        "${cudapkgs.cudaPackages.cuda_nvml_dev.dev}";
            LIBCUSPARSE_STORE_PATH = "${cudapkgs.cudaPackages.libcusparse.dev}";


            # on relase this is overwritten
            COMPILE_MODE = "debug"; 

            shellHook = ''zsh'';
        } // extraArgs);
    in
    {
        devShells.${system} = {
            default = baseShell pkgs.StarPU {};
            release = (baseShell (pkgs.StarPU.overrideAttrs (oldAttrs: {
                enableTrace = false;
                buildMode = "release";
            }))) { COMPILE_MODE = "release"; };
            pcad_experiments = (baseShell (pkgs.StarPU.overrideAttrs (oldAttrs: {
                enableTrace = true;
                enableCUDA = false;
                buildMode = "release";
            }))) { 
                COMPILE_MODE = "release"; 
                shellHook = '''';
            };
        };
        packages.${system} = {
            star-fletcher = 
                let 
                    localStarPU = (pkgs.StarPU.overrideAttrs (oldAttrs: {
                        enableTrace = true;
                        enableCUDA = false;
                        buildMode = "release";
                    }));
                in pkgs.stdenv.mkDerivation {
                pname = "star-fletcher";
                version = "0.1";
                src = ./.;
                nativeBuildInputs = with pkgs; [
                    pkg-config
                    hwloc
                    localStarPU
                ];
                buildInputs = [
                    pkgs.python313
                    localStarPU
                ];
                buildPhase = "COMPILE_MODE=release make";
                installPhase = "mkdir -p $out/bin && cp main $out/bin/star-fletcher";
            };
        };
    };
}
