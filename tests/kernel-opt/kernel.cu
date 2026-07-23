__device__ static inline size_t cuda_idx(
    const size_t x, const size_t y, const size_t z, 
    const size_t ldy, const size_t ldz
){
    return x + ldy * y + z * ldz;
}

#define STARPU_BLOCK_GET_PTR(x) x

#define CUDA_CALL(call) do{ \
   const cudaError_t err = call; \
   if (err != cudaSuccess){ \
     fprintf(stderr, "CUDA ERROR: %s on %s:%d\n", cudaGetErrorString(err), __FILE__, __LINE__);\
     exit(1); \
   }}while(0)


#define CODE_IMPL
#define CUDA_CODE
#include "../derivatives/derivatives-impl.h"
#undef CUDA_CODE
#undef CODE_IMPL

struct rtm_kernel_params {
    // Spatial bounds & dimensions
    size_t x_start, y_start, z_start;
    size_t x_end, y_end, z_end;
    size_t cube_width_x, cube_width_y, cube_width_z;
    size_t stride_x, stride_y, stride_z;

    // Finite difference coefficients
    FP dt, dxxinv, dyyinv, dzzinv, dxyinv, dxzinv, dyzinv;

    // Buffer pointers
    FP* ptrs[52];
};

extern size_t g_cube_width;

__global__ void rtm_cuda_kernel_impl(struct rtm_kernel_params p) {
    /*

    const FP dxxinv = FP_LIT(1.0) / (dx * dx);
    const FP dyyinv = FP_LIT(1.0) / (dy * dy);
    const FP dzzinv = FP_LIT(1.0) / (dz * dz);
    const FP dxyinv = FP_LIT(1.0) / (dx * dy);
    const FP dxzinv = FP_LIT(1.0) / (dx * dz);
    const FP dyzinv = FP_LIT(1.0) / (dy * dz);

    // global

    const size_t cube_width_x = g_cube_width;
    const size_t cube_width_y = g_cube_width;
    const size_t cube_width_z = g_cube_width;

    const size_t stride_x = 1;
    const size_t stride_y = g_cube_width;
    const size_t stride_z = g_cube_width * g_cube_width;

    const size_t x_start,
    const size_t y_start,
    const size_t z_start,
    const size_t x_end,
    const size_t y_end,
    const size_t z_end,
    const size_t cube_width_x,
    const size_t cube_width_y,
    const size_t cube_width_z,
    const size_t stride_x,
    const size_t stride_y,
    const size_t stride_z,
    const FP dt,
    const FP dxxinv,
    const FP dyyinv,
    const FP dzzinv,
    const FP dxyinv,
    const FP dxzinv,
    const FP dyzinv, 
    const FP* ch1dxx,
    const FP* ch1dyy,
    const FP* ch1dzz, 
    const FP* ch1dxy,
    const FP* ch1dyz,
    const FP* ch1dxz,
    const FP* v2px,
    const FP* v2pz,
    const FP* v2sz,
    const FP* v2pn,
    FP *const pwwrite,
    const FP* pwcentralt1,
    const FP* pwip0jp0km1,
    const FP* pwip0jm1km1,
    const FP* pwim1jp0km1,
    const FP* pwip1jp0km1,
    const FP* pwip0jp1km1,
    const FP* pwim1jm1kp0,
    const FP* pwip0jm1kp0,
    const FP* pwip1jm1kp0,
    const FP* pwim1jp0kp0,
    const FP* pwip1jp0kp0,
    const FP* pwim1jp1kp0,
    const FP* pwip0jp1kp0,
    const FP* pwip1jp1kp0,
    const FP* pwip0jp0kp1,
    const FP* pwip0jm1kp1,
    const FP* pwim1jp0kp1,
    const FP* pwip1jp0kp1,
    const FP* pwip0jp1kp1,
    const FP* pwcentralt2,
    FP *const qwwrite,
    const FP* qwcentralt1,
    const FP* qwip0jp0km1,
    const FP* qwip0jm1km1,
    const FP* qwim1jp0km1,
    const FP* qwip1jp0km1,
    const FP* qwip0jp1km1,
    const FP* qwim1jm1kp0,
    const FP* qwip0jm1kp0,
    const FP* qwip1jm1kp0,
    const FP* qwim1jp0kp0,
    const FP* qwip1jp0kp0,
    const FP* qwim1jp1kp0,
    const FP* qwip0jp1kp0,
    const FP* qwip1jp1kp0,
    const FP* qwip0jp0kp1,
    const FP* qwip0jm1kp1,
    const FP* qwim1jp0kp1,
    const FP* qwip1jp0kp1,
    const FP* qwip0jp1kp1,
    const FP* qwcentralt2
*/

    // precomputed values
    const FP* ch1dxx = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[0]);
    const FP* ch1dyy = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[1]);
    const FP* ch1dzz = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[2]);
    const FP* ch1dxy = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[3]);
    const FP* ch1dyz = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[4]);
    const FP* ch1dxz = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[5]);
    const FP* v2px = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[6]);
    const FP* v2pz = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[7]);
    const FP* v2sz = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[8]);
    const FP* v2pn = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[9]);

    // w at (i, j, k) of t[0]
    FP *const pwwrite = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[10]);
    // primary wave
    // STARPU_R, // r at (i, j, k) of t[1]
    const FP* pwcentralt1 = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[11]);
    // // layer when k - 1
    // // o x o
    // // x x x 
    // // o x o
    // STARPU_R, // r at (i + 0, j + 0, k - 1) of t[1]
    // STARPU_R, // r at (i + 0, j - 1, k - 1) of t[1]
    // STARPU_R, // r at (i - 1, j + 0, k - 1) of t[1]
    // STARPU_R, // r at (i + 1, j + 0, k - 1) of t[1]
    // STARPU_R, // r at (i + 0, j + 1, k - 1) of t[1]
    // nomenclatura de variável é:
    // pw (onda primária do bloco)  ip0 (i + 0)  jp0 (j + 0)  km1 (k - 1), em relação ao central
    const FP* pwip0jp0km1 = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[12]);
    const FP* pwip0jm1km1 = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[13]);
    const FP* pwim1jp0km1 = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[14]);
    const FP* pwip1jp0km1 = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[15]);
    const FP* pwip0jp1km1 = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[16]);

    // // layer when k
    // // x x x
    // // x o x 
    // // x x x
    // STARPU_R, // r at (i - 1, j - 1, k + 0) of t[1]
    // STARPU_R, // r at (i + 0, j - 1, k + 0) of t[1]
    // STARPU_R, // r at (i + 1, j - 1, k + 0) of t[1]
    // STARPU_R, // r at (i - 1, j + 0, k + 0) of t[1]
    // STARPU_R, // r at (i + 1, j + 0, k + 0) of t[1]
    // STARPU_R, // r at (i - 1, j + 1, k + 0) of t[1]
    // STARPU_R, // r at (i + 0, j + 1, k + 0) of t[1]
    // STARPU_R, // r at (i + 1, j + 1, k + 0) of t[1]
    const FP* pwim1jm1kp0 = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[17]);
    const FP* pwip0jm1kp0 = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[18]);
    const FP* pwip1jm1kp0 = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[19]);
    const FP* pwim1jp0kp0 = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[20]);
    const FP* pwip1jp0kp0 = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[21]);
    const FP* pwim1jp1kp0 = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[22]);
    const FP* pwip0jp1kp0 = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[23]);
    const FP* pwip1jp1kp0 = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[24]);

    // // layer when k + 1
    // // o x o
    // // x x x 
    // // o x o
    // STARPU_R, // r at (i + 0, j + 0, k + 1) of t[1]
    // STARPU_R, // r at (i + 0, j - 1, k + 1) of t[1]
    // STARPU_R, // r at (i - 1, j + 0, k + 1) of t[1]
    // STARPU_R, // r at (i + 1, j + 0, k + 1) of t[1]
    // STARPU_R, // r at (i + 0, j + 1, k + 1) of t[1]
    const FP* pwip0jp0kp1 = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[25]);
    const FP* pwip0jm1kp1 = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[26]);
    const FP* pwim1jp0kp1 = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[27]);
    const FP* pwip1jp0kp1 = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[28]);
    const FP* pwip0jp1kp1 = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[29]);

    // STARPU_R  // r at (i, j, k) of t[2]
    const FP* pwcentralt2 = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[30]);

    // secondary wave
    FP *const qwwrite = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[31]);

    const FP* qwcentralt1 = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[32]);

    // layer when k - 1
    const FP* qwip0jp0km1 = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[33]);
    const FP* qwip0jm1km1 = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[34]);
    const FP* qwim1jp0km1 = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[35]);
    const FP* qwip1jp0km1 = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[36]);
    const FP* qwip0jp1km1 = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[37]);

    // layer when k
    const FP* qwim1jm1kp0 = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[38]);
    const FP* qwip0jm1kp0 = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[39]);
    const FP* qwip1jm1kp0 = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[40]);
    const FP* qwim1jp0kp0 = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[41]);
    const FP* qwip1jp0kp0 = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[42]);
    const FP* qwim1jp1kp0 = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[43]);
    const FP* qwip0jp1kp0 = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[44]);
    const FP* qwip1jp1kp0 = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[45]);

    // layer when k + 1
    const FP* qwip0jp0kp1 = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[46]);
    const FP* qwip0jm1kp1 = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[47]);
    const FP* qwim1jp0kp1 = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[48]);
    const FP* qwip1jp0kp1 = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[49]);
    const FP* qwip0jp1kp1 = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[50]);

    const FP* qwcentralt2 = (FP*) STARPU_BLOCK_GET_PTR(p.ptrs[51]);



    // 1. Calculate thread's 3D coordinate
    const size_t x = blockIdx.x * blockDim.x + threadIdx.x;
    const size_t y = blockIdx.y * blockDim.y + threadIdx.y;
    const size_t z = blockIdx.z * blockDim.z + threadIdx.z;

    // 2. Bounds check against the total cube dimension
    if (x >= p.cube_width_x || y >= p.cube_width_y || z >= p.cube_width_z) {
        return;
    }

    const size_t idx = cuda_idx(x, y, z, p.stride_y, p.stride_z);

    // 3. Apply the internal boundary logic from your original code
    if (
        (z < p.z_start || z >= p.z_end) || 
        (y < p.y_start || y >= p.y_end) || 
        (x < p.x_start || x >= p.x_end)
    ) {
        pwwrite[idx] = FP_LIT(0.0);
        qwwrite[idx] = FP_LIT(0.0);
        return;
    }

    const FP pxx = snd_deriv_dir(pwcentralt1, pwim1jp0kp0, pwip1jp0kp0, x, idx, p.stride_x, p.dxxinv, p.cube_width_x);
    const FP pyy = snd_deriv_dir(pwcentralt1, pwip0jm1kp0, pwip0jp1kp0, y, idx, p.stride_y, p.dyyinv, p.cube_width_y);
    const FP pzz = snd_deriv_dir(pwcentralt1, pwip0jp0km1, pwip0jp0kp1, z, idx, p.stride_z, p.dzzinv, p.cube_width_z);
    const FP pxy = cross_deriv_ddir(
        pwcentralt1, idx, 
        x, pwim1jp0kp0, pwip1jp0kp0, p.stride_x, 
        y, pwip0jm1kp0, pwip0jp1kp0, p.stride_y, 
        pwip1jp1kp0, pwip1jm1kp0, pwim1jp1kp0, pwim1jm1kp0,
        p.cube_width_x, p.dxyinv
    ); 
    const FP pyz = cross_deriv_ddir(
        pwcentralt1, idx, 
        y, pwip0jm1kp0, pwip0jp1kp0, p.stride_y, 
        z, pwip0jp0km1, pwip0jp0kp1, p.stride_z, 
        pwip0jp1kp1, pwip0jp1km1, pwip0jm1kp1, pwip0jm1km1,
        p.cube_width_y, p.dyzinv
    ); 
    const FP pxz = cross_deriv_ddir(
        pwcentralt1, idx, 
        x, pwim1jp0kp0, pwip1jp0kp0, p.stride_x, 
        z, pwip0jp0km1, pwip0jp0kp1, p.stride_z, 
        pwip1jp0kp1, pwip1jp0km1, pwim1jp0kp1, pwim1jp0km1,
        p.cube_width_x, p.dxzinv
    ); 

    const FP cpxx = ch1dxx[idx] * pxx;
    const FP cpyy = ch1dyy[idx] * pyy;
    const FP cpzz = ch1dzz[idx] * pzz;
    const FP cpxy = ch1dxy[idx] * pxy;
    const FP cpxz = ch1dxz[idx] * pxz;
    const FP cpyz = ch1dyz[idx] * pyz;
    const FP h1p = cpxx + cpyy + cpzz + cpxy + cpxz + cpyz;
    const FP h2p = pxx + pyy + pzz - h1p;

    // q derivatives, H1(q) and H2(q)
    const FP qxx = snd_deriv_dir(qwcentralt1, qwim1jp0kp0, qwip1jp0kp0, x, idx, p.stride_x, p.dxxinv, p.cube_width_x);
    const FP qyy = snd_deriv_dir(qwcentralt1, qwip0jm1kp0, qwip0jp1kp0, y, idx, p.stride_y, p.dyyinv, p.cube_width_y);
    const FP qzz = snd_deriv_dir(qwcentralt1, qwip0jp0km1, qwip0jp0kp1, z, idx, p.stride_z, p.dzzinv, p.cube_width_z);
    const FP qxy = cross_deriv_ddir(
        qwcentralt1, idx, 
        x, qwim1jp0kp0, qwip1jp0kp0, p.stride_x, 
        y, qwip0jm1kp0, qwip0jp1kp0, p.stride_y, 
        qwip1jp1kp0, qwip1jm1kp0, qwim1jp1kp0, qwim1jm1kp0,
        p.cube_width_x, p.dxyinv
    ); 
    const FP qyz = cross_deriv_ddir(
        qwcentralt1, idx, 
        y, qwip0jm1kp0, qwip0jp1kp0, p.stride_y, 
        z, qwip0jp0km1, qwip0jp0kp1, p.stride_z, 
        qwip0jp1kp1, qwip0jp1km1, qwip0jm1kp1, qwip0jm1km1,
        p.cube_width_y, p.dyzinv
    ); 
    const FP qxz = cross_deriv_ddir(
        qwcentralt1, idx, 
        x, qwim1jp0kp0, qwip1jp0kp0, p.stride_x, 
        z, qwip0jp0km1, qwip0jp0kp1, p.stride_z, 
        qwip1jp0kp1, qwip1jp0km1, qwim1jp0kp1, qwim1jp0km1,
        p.cube_width_x, p.dxzinv
    ); 

    const FP cqxx = ch1dxx[idx] * qxx;
    const FP cqyy = ch1dyy[idx] * qyy;
    const FP cqzz = ch1dzz[idx] * qzz;
    const FP cqxy = ch1dxy[idx] * qxy;
    const FP cqxz = ch1dxz[idx] * qxz;
    const FP cqyz = ch1dyz[idx] * qyz;
    const FP h1q = cqxx + cqyy + cqzz + cqxy + cqxz + cqyz;
    const FP h2q = qxx + qyy + qzz - h1q;

    // p-q derivatives, H1(p-q) and H2(p-q)
    const FP h1pmq = h1p - h1q;
    const FP h2pmq = h2p - h2q;

    // rhs of p and q equations
    const FP rhsp = v2px[idx] * h2p + v2pz[idx] * h1q + v2sz[idx] * h1pmq;
    const FP rhsq = v2pn[idx] * h2p + v2pz[idx] * h1q - v2sz[idx] * h2pmq;

    // new p and q
    pwwrite[idx] = FP_LIT(2.0) * pwcentralt1[idx] - pwcentralt2[idx] + rhsp * p.dt * p.dt;
    qwwrite[idx] = FP_LIT(2.0) * qwcentralt1[idx] - qwcentralt2[idx] + rhsq * p.dt * p.dt;
}


extern "C" void rtm_kernel_cuda(struct rtm_kernel_params* p){

    // Define CUDA Grid and Block Dimensions
    // A 3D block of 8x8x8 is 512 threads, which is a standard starting point for 3D stencils.
    dim3 threads_per_block(8, 8, 8); 
    dim3 num_blocks(
        (p->cube_width_x + threads_per_block.x - 1) / threads_per_block.x,
        (p->cube_width_y + threads_per_block.y - 1) / threads_per_block.y,
        (p->cube_width_z + threads_per_block.z - 1) / threads_per_block.z
    );

    // Launch the kernel asynchronously on StarPU's managed stream
    rtm_cuda_kernel_impl<<<num_blocks, threads_per_block>>>(*p);

    CUDA_CALL(cudaGetLastError());
}
