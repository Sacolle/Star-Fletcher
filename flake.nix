{
    description = "A very basic flake";

    inputs = {
        nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
        # gets the proper version of the CUDA packages for compilation
        cudaNixpkgs.url = "github:nixos/nixpkgs/1da52dd49a127ad74486b135898da2cef8c62665";
        StarPU.url = "github:Sacolle/nix-starpu";
        madagascar.url = "github:Sacolle/nix-madagascar";
    };

    outputs = { self, nixpkgs, cudaNixpkgs, StarPU, madagascar }: 
    let 
        system = "x86_64-linux";
        pkgsconfigs = { 
            inherit system; 
            config = { 
                allowUnfree = true;
                cudaSupport = true;
                cudaVersion = "13";
            };
        };
        # import gcc13Stdenv from this because it uses the correct version of glibc (2.40-36)
        cudapkgs = import cudaNixpkgs pkgsconfigs;
        pkgs = import nixpkgs pkgsconfigs;

        myStarPU = StarPU.packages.${system}.default.override {
                enableCUDA = true; 
                maxBuffers = 56;
                enableTrace = true;
        };

        baseShell = StarPUVersion: extraArgs: pkgs.mkShell ({
            buildInputs = with pkgs; [
                pkg-config
                hwloc
                StarPUVersion
                
                # project libs
                criterion

                # for bash to work properlly inside vscode
                bashInteractive
                moreutils
                # for fuser
                psmisc
                # debug tools
                gdb
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
            GCC_STORE_PATH = "${pkgs.gcc}";
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
        } // extraArgs);
    in
    {
        devShells.${system} = {
            default = baseShell myStarPU {};

            release = (baseShell (myStarPU.overrideAttrs (oldAttrs: {
                enableTrace = false;
                compileAsRelease = true;
            }))) { COMPILE_MODE = "release"; };

            pcad_experiments = (baseShell (myStarPU.overrideAttrs (oldAttrs: {
                enableTrace = true;
                enableCUDA = false;
                compileAsRelease = true;
                # necessário na poti, pois a estrutura de 20 cores com 28 threads buga o StarPU
                # versões mais recentes do StarPU resolvem isso aparentemente
                # mais recentes as in 03/26
                extraOptions = [ "--enable-maxcpus=256" ]; 
            }))) { 
                COMPILE_MODE = "release"; 
            };
        };
        packages.${system} = 
        let
            star-fletcher = 
                let 
                    localStarPU = (myStarPU.overrideAttrs (oldAttrs: {
                        enableTrace = true;
                        enableCUDA = false;
                        compileAsRelease = true;
                    }));
                in 
                pkgs.stdenv.mkDerivation {
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
            star-fletcher-debug = 
                let 
                    localStarPU = (myStarPU.overrideAttrs (oldAttrs: {
                        enableTrace = true;
                        enableCUDA = false;
                        #compileAsRelease = true;
                    }));
                in 
                pkgs.stdenv.mkDerivation {
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
                    buildPhase = "COMPILE_MODE=debug make";
                    installPhase = "mkdir -p $out/bin && cp main $out/bin/star-fletcher";
                };
        in
        {
            default = star-fletcher;
            inherit star-fletcher star-fletcher-debug;
        };
    };
}
