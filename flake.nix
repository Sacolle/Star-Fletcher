{
    description = "A very basic flake";

    inputs = {
        nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-26.05";
        StarPU = {
          url = "github:Sacolle/nix-starpu";
          inputs.nixpkgs.follows = "nixpkgs";
        };
        nix-gl-host = {
            url = "github:numtide/nix-gl-host";
            inputs.nixpkgs.follows = "nixpkgs";
        };
        eztrace = {
            url = "github:Sacolle/eztrace-pallas-nix";
            inputs.nixpkgs.follows = "nixpkgs";
        };

        # gets the proper version of the CUDA packages for compilation
        cudaNixpkgs.url = "github:nixos/nixpkgs/1da52dd49a127ad74486b135898da2cef8c62665";
        madagascar.url = "github:Sacolle/nix-madagascar";
    };

    outputs = { self, nixpkgs, cudaNixpkgs, StarPU, eztrace, madagascar, nix-gl-host }: 
    let 
        system = "x86_64-linux";
        pkgsconfigs = { 
            inherit system; 
            config.allowUnfree = true;
        };
        # import gcc13Stdenv from this because it uses the correct version of glibc (2.40-36)
        cudapkgs = import cudaNixpkgs pkgsconfigs;

        cudaPacks = cudapkgs.cudaPackages_12_2;

        pkgs = import nixpkgs pkgsconfigs;

        baseShell = StarPUVersion: extraArgs: pkgs.mkShell.override { stdenv = pkgs.gcc13Stdenv; } (rec {
            buildInputs = with pkgs; [
                pkg-config
                bear # for emacs eglot
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
                cuda_nvcc
                cuda_cudart
                cuda_cccl
                cuda_nvml_dev
                libcublas 
                libcusparse
                libcusolver
                libcufft
            ]);
            # export StarPU and hwloc store locations 
            # for use in vscode intellisence
            GCC_STORE_PATH =         "${pkgs.gcc}/include/";
            STARPU_STORE_PATH =      "${StarPUVersion}/include/starpu/1.4";
            CRITERION_STORE_PATH =   "${pkgs.criterion.dev}/include/";
            HWLOC_STORE_PATH =       "${pkgs.hwloc.dev}/include";
            # cudas
            CUDART_STORE_PATH =      "${cudapkgs.cudaPackages.cuda_cudart.dev}/include/";
            NVCC_STORE_PATH =        "${cudapkgs.cudaPackages.cuda_nvcc}/include/";
            NVML_STORE_PATH =        "${cudapkgs.cudaPackages.cuda_nvml_dev.dev}/include/";
            LIBCUSPARSE_STORE_PATH = "${cudapkgs.cudaPackages.libcusparse.dev}/include/";
        } // extraArgs);

        star-fletcher = pkgs.callPackage ./star-fletcher.nix {
            StarPU = StarPU.packages.${system}.default;
        };

        starpu-cuda = StarPU.packages.${system}.default.override {
            	enableCUDA = true;
	        compileAsRelease = true;
	        enableTrace = false;
            	maxBuffers = 56;
	        cudaPackages = cudaPacks;
	        stdenv = cudapkgs.gcc12Stdenv;
	    };

        star-fletcher-cuda = pkgs.callPackage ./star-fletcher.nix {
            StarPU = StarPU.packages.${system}.default; #starpu-cuda;
            cudaPackages = cudaPacks;
            enableCUDA = true;
            enableTrace = false;
            compileAsRelease = true;
            stdenv = cudapkgs.gcc12Stdenv;
        };

        star-fletcher-cuda-trace = pkgs.callPackage ./star-fletcher.nix {
            StarPU = StarPU.packages.${system}.default; #starpu-cuda;
            cudaPackages = cudaPacks;
            enableCUDA = true;
            enableTrace = true;
            compileAsRelease = true;
            stdenv = cudapkgs.gcc12Stdenv;
        };

        nixglhost = nix-gl-host.defaultPackage.${system};
    in
    {
        devShells.${system} = {
          default = baseShell starpu-cuda {};
          no-cuda = baseShell (StarPU.packages.${system}.default.override {
            	enableCUDA = false;
	            compileAsRelease = true;
	            enableTrace = false;
            	maxBuffers = 56;
	        }) {};
          eztrace-test = pkgs.mkShell {
                buildInputs = [
                    (eztrace.packages.${system}.new-starpu.override {
                        enableTrace = false;
                        maxBuffers = 56;
                        enableCUDA = false;
                        compileAsRelease = false;
                        stdenv = pkgs.gcc13Stdenv;
                    })
                    (eztrace.packages.${system}.eztrace.override {
                        pallas = eztrace.packages.${system}.pallas.override {
                            stdenv = pkgs.gcc13Stdenv;
                        };
                        stdenv = pkgs.gcc13Stdenv;
                        compileAsDebug = true;
                    })
                    pkgs.pkg-config
                    pkgs.hwloc
                    pkgs.python313
                    pkgs.gdb
                ];
            };
            cuda-test = pkgs.mkShell.override { stdenv = cudapkgs.gcc12Stdenv; } {
                buildInputs = [ 
                    pkgs.python313
                    pkgs.pkg-config
                    pkgs.hwloc

                    pkgs.cudaPackages.cuda_gdb
                    starpu-cuda
                    #pkgs.gdb
                    nixglhost 
                    cudaPacks.cuda_cuobjdump
                ] ++ (with cudaPacks; [
                    cuda_nvcc
                ]);
            };
            star-compilation = pkgs.mkShell {
                buildInputs = [ 
                    star-fletcher-cuda		
                    nixglhost 
                    cudaPacks.cuda_cuobjdump
                ];
            };

            star-fletcher-trace = pkgs.mkShell {
                buildInputs = [ 
                    star-fletcher-cuda-trace
                    nixglhost 
                ];
            };
        };
        packages.${system} = {
          default = star-fletcher;
          inherit star-fletcher star-fletcher-cuda;
        };
    };
}
