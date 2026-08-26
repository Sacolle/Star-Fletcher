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
    disableCPUKernel ? false,
    enableTrace ? false,
    compileAsRelease ? true
}:
let 
    cudaNativeBuildInputs = with cudaPackages; [
        cuda_nvcc
        autoAddDriverRunpath
    ];
    
    localStarPU = StarPU.override {
        inherit cudaPackages;
        inherit stdenv;
        inherit enableTrace enableCUDA compileAsRelease;
        maxBuffers = 56;
    };
in
stdenv.mkDerivation {
    pname = "star-fletcher";
    system = "x86_64-linux";
    version = "0.1.2";

    src = ./.;

    nativeBuildInputs = [
      pkg-config
      python313
      localStarPU
    ] ++ lib.optionals enableCUDA cudaNativeBuildInputs;

    buildInputs = [
      localStarPU
    ];

    makeFlags = [ ] 
    ++ lib.optional compileAsRelease "RELEASE_MODE=1" 
    ++ lib.optional disableCPUKernel "NO_CPU_KERNEL=1" 
    ++ lib.optionals enableCUDA [
      "CUDA_BACKEND=1 "
      (if cuda_arch != "" then "ARCH=${cuda_arch}" else " ")
    ];

    installPhase = "mkdir -p $out/bin && cp main $out/bin/star-fletcher";
}
