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
        
        # gets the proper version of the CUDA packages for compilation
        cudaNixpkgs.url = "github:nixos/nixpkgs/1da52dd49a127ad74486b135898da2cef8c62665";
        madagascar.url = "github:Sacolle/nix-madagascar";
    };

    outputs = { self, nixpkgs, cudaNixpkgs, StarPU, madagascar, nix-gl-host }: 
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

	cudaPacks = cudapkgs.cudaPackages_12_2;

        pkgs = import nixpkgs pkgsconfigs;

        myStarPU = StarPU.packages.${system}.default.override {
                enableCUDA = true; 
                maxBuffers = 56;
                enableTrace = true;
        };

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
                cuda_cudart
                cuda_nvcc
                cuda_cccl
                cuda_nvml_dev.dev
                libcusparse.dev
                cuda_cuobjdump
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

            # on relase this is overwritten
            COMPILE_MODE = "debug"; 
        } // extraArgs);

        star-fletcher = pkgs.callPackage ./star-fletcher.nix {
          StarPU = StarPU.packages.${system}.default;
        };

        star-fletcher-cuda = star-fletcher.override {
          enableCUDA = true;
        };

	starpu-cuda = StarPU.packages.${system}.default.override {
          enableCUDA = true;
	  compileAsRelease = true;
	  enableTrace = false;
          maxBuffers = 56;
	  cudaPackages = cudaPacks;
	  # Stupid hack, but ig is the way to do
	  gcc13Stdenv = cudapkgs.gcc12Stdenv;
	};

        nixglhost = nix-gl-host.defaultPackage.${system};
    in
    {
        devShells.${system} = {
            default = baseShell myStarPU {};

            release = (baseShell (myStarPU.override {
                enableTrace = false;
                compileAsRelease = true;
            })) { COMPILE_MODE = "release"; };

            pcad_experiments = (baseShell (myStarPU.override {
                enableTrace = true;
                enableCUDA = false;
                compileAsRelease = true;
                # necessário na poti, pois a estrutura de 20 cores com 28 threads buga o StarPU
                # versões mais recentes do StarPU resolvem isso aparentemente
                # mais recentes as in 0aru3/26
                extraOptions = [ "--enable-maxcpus=256" ]; 
            })) { 
                COMPILE_MODE = "release"; 
            };
            test = pkgs.mkShell.override { stdenv = cudapkgs.gcc12Stdenv; } {
                buildInputs = [ 
	 	    pkgs.cudaPackages.cuda_gdb
                    #star-fletcher-cuda
		    starpu-cuda
	            pkgs.pkg-config
	            pkgs.hwloc
                    #pkgs.gdb
                    nixglhost 
                    cudaPacks.cuda_cuobjdump
                ] ++ (with cudaPacks; [
		    cuda_nvcc
		    cuda_cudart
		    cuda_cccl
		    cuda_nvml_dev
		    libcublas 
		    libcusparse
		    libcusolver
		    libcufft
		]);
            };
        };
        packages.${system} = {
          default = star-fletcher;
          inherit star-fletcher star-fletcher-cuda;
        };
    };
}
