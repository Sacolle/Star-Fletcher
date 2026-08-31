{
    # derivation dependencies
    lib,
    stdenv,
    cudaPackages,
    compileAsRelease ? true,
    autoAddDriverRunpath #
}:
let 
    cudaNativeBuildInputs = with cudaPackages; [ cuda_nvcc autoAddDriverRunpath ];
    cudaBuildInputs = with cudaPackages; [ cuda_cudart cuda_cccl ];

in
stdenv.mkDerivation (f: 
{
    pname = "kernel-test";
    system = "x86_64-linux";
    version = "0.1";

    src = ./.;

    nativeBuildInputs = cudaNativeBuildInputs;
    buildInputs = cudaBuildInputs;

    makeFlags = [
        "NIX_CUDA_CFLAGS=-I${cudaPackages.cuda_cudart.dev}/include"
	    "NIX_CUDA_LDFLAGS=-L${cudaPackages.cuda_cudart.lib}/lib"
    ] ++ lib.optional compileAsRelease "RELEASE_MODE=1";

    installPhase = "mkdir -p $out/bin && cp main $out/bin/fletcher-base";
})
