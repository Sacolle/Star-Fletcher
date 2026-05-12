{
  # derivation dependencies
  lib,
  stdenv,
  pkg-config,
  hwloc,
  python313,

  StarPU,

  cudaPackages,

  enableCUDA ? false,
  enableTrace ? false,
  compileAsRelease ? true
}:
let 
  cudaNativeBuildInputs = [];
  cudaBuildInputs = [];
  localStarPU = StarPU.overrideAttrs (old: {
    inherit enableTrace enableCUDA compileAsRelease;
    maxBuffers = 56;
  });
in
stdenv.mkDerivation (f: {
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
    ] ++ lib.optional f.enableCUDA cudaNativeBuildInputs;

    buildInputs = [
      localStarPU
    ] ++ lib.optional f.enableCUDA cudaBuildInputs;

    # preciso também do treco de achar os drivers da máquina

    buildPhase = "COMPILE_MODE=${if compileAsRelease then "release" else "debug"} make";
    installPhase = "mkdir -p $out/bin && cp main $out/bin/star-fletcher";
})
