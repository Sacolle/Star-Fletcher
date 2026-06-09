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
    cudaNativeBuildInputs = with cudaPackages; [ cuda_nvcc autoAddDriverRunpath ];
    cudaBuildInputs = with cudaPackages; [ cuda_cudart cuda_cccl ];
    
    localStarPU = (StarPU.override { inherit cudaPackages; }).overrideAttrs (old: {
      inherit enableTrace enableCUDA compileAsRelease;
      maxBuffers = 56;
    });
in
stdenv.mkDerivation {
    pname = "star-fletcher";
    system = "x86_64-linux";
    version = "0.1";

    inherit enableTrace enableCUDA compileAsRelease;

    src = ./.;

    nativeBuildInputs = [
      pkg-config
      python313
      hwloc
      localStarPU
    ] ++ lib.optionals enableCUDA cudaNativeBuildInputs;

    buildInputs = [
      localStarPU
    ] ++ lib.optionals enableCUDA cudaBuildInputs;

    makeFlags = lib.optionals enableCUDA [
      "CUDA_BACKEND=1 "
      (if cuda_arch != "" then "ARCH=${cuda_arch}" else " ")
    ];

    # preciso também do treco de achar os drivers da máquina

    buildPhase = "COMPILE_MODE=${if compileAsRelease then "release" else "debug"} make";
    installPhase = "mkdir -p $out/bin && cp main $out/bin/star-fletcher";
}
