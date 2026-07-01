{
    # derivation dependencies
    lib,
    stdenv,
    pkg-config,
    hwloc,
    python313,

    StarPU,
    cudaPackages,
    autoAddDriverRunpath, #

    enableCUDA ? false,
    cuda_arch ? "",
    enableTrace ? false,
    compileAsRelease ? true
}:
let 
    cudaNativeBuildInputs = with cudaPackages; [
        cuda_nvcc
        autoAddDriverRunpath
    ];

    cudaBuildInputs = with cudaPackages; [
        cuda_cudart
        cuda_cccl
        cuda_nvml_dev
        libcublas 
        libcusparse
        libcusolver
        libcufft
    ];
    
    localStarPU = StarPU.override {
        inherit cudaPackages;
        inherit stdenv;
        inherit enableTrace enableCUDA compileAsRelease;
        maxBuffers = 56;
    };
    # find a better way to get the hwloc configured, in this case I think it is propagatedInputs	
    cudaHwloc = hwloc.override { 
        inherit cudaPackages; 
        enableCuda = true;
    };
    my-hwloc = if enableCUDA then cudaHwloc else hwloc;
in
stdenv.mkDerivation {
    pname = "star-fletcher";
    system = "x86_64-linux";
    version = "0.1";

    src = ./.;

    nativeBuildInputs = [
      pkg-config
      python313
      my-hwloc
      localStarPU
    ] ++ lib.optionals enableCUDA cudaNativeBuildInputs;

    buildInputs = [
      localStarPU
    ] ++ lib.optionals enableCUDA cudaBuildInputs;

    makeFlags = [ ] 
    ++ lib.optional compileAsRelease "RELEASE_MODE=1" 
    ++ lib.optionals enableCUDA [
      "CUDA_BACKEND=1 "
      (if cuda_arch != "" then "ARCH=${cuda_arch}" else " ")
    ];

    installPhase = "mkdir -p $out/bin && cp main $out/bin/star-fletcher";
}
