#include <starpu.h>

#include "kernel.h"
#include "floatingpoint.h"
#include "macros.h"
#include "derivatives.h"


int make_rtm_args(rtm_args_t** rtm_args, 
    const size_t x_start, const size_t x_end,
    const size_t y_start, const size_t y_end,
    const size_t z_start, const size_t z_end,
    const FP dx, const FP dy, const FP dz, const FP dt
){
    *rtm_args = (rtm_args_t*) malloc(sizeof(rtm_args_t));
    if(*rtm_args == NULL){
        return 1;
    }
    **rtm_args = (rtm_args_t){
        .x_start = x_start, .x_end = x_end,
        .y_start = y_start, .y_end = y_end,
        .z_start = z_start, .z_end = z_end,
        .dx = dx, .dy = dy, .dz = dz, .dt = dt
    };
    return 0;
}


int make_perturb_args(perturb_args_t** perturb_args, const size_t idx, const FP value, const size_t t){
    *perturb_args = (perturb_args_t*) malloc(sizeof(perturb_args_t));
    if(*perturb_args == NULL){
        return 1;
    }
    (*perturb_args)->source_idx = idx;
    (*perturb_args)->perturb_value = value;
    (*perturb_args)->t = t;
    return 0;
}

void rtm_kernel(void *descr[], void *cl_args){
    // do stuff here
    const rtm_args_t* args = (rtm_args_t*) cl_args;

    const size_t x_start = args->x_start;
    const size_t y_start = args->y_start;
    const size_t z_start = args->z_start;
    const size_t x_end = args->x_end;
    const size_t y_end = args->y_end;
    const size_t z_end = args->z_end;

    const FP dx = args->dx;
    const FP dy = args->dy;
    const FP dz = args->dz;
    const FP dt = args->dt;

    const FP dxxinv = FP_LIT(1.0) / (dx * dx);
    const FP dyyinv = FP_LIT(1.0) / (dy * dy);
    const FP dzzinv = FP_LIT(1.0) / (dz * dz);
    const FP dxyinv = FP_LIT(1.0) / (dx * dy);
    const FP dxzinv = FP_LIT(1.0) / (dx * dz);
    const FP dyzinv = FP_LIT(1.0) / (dy * dz);

    const size_t cube_width_x = STARPU_BLOCK_GET_NX(descr[0]);
    const size_t cube_width_y = STARPU_BLOCK_GET_NY(descr[0]);
    const size_t cube_width_z = STARPU_BLOCK_GET_NZ(descr[0]);

    const size_t stride_x = 1;
    const size_t stride_y = STARPU_BLOCK_GET_LDY(descr[0]);
    const size_t stride_z = STARPU_BLOCK_GET_LDZ(descr[0]);


    // precomputed values
    const FP* ch1dxx = (FP*) STARPU_BLOCK_GET_PTR(descr[0]);
    const FP* ch1dyy = (FP*) STARPU_BLOCK_GET_PTR(descr[1]);
    const FP* ch1dzz = (FP*) STARPU_BLOCK_GET_PTR(descr[2]);
    const FP* ch1dxy = (FP*) STARPU_BLOCK_GET_PTR(descr[3]);
    const FP* ch1dyz = (FP*) STARPU_BLOCK_GET_PTR(descr[4]);
    const FP* ch1dxz = (FP*) STARPU_BLOCK_GET_PTR(descr[5]);
    const FP* v2px = (FP*) STARPU_BLOCK_GET_PTR(descr[6]);
    const FP* v2pz = (FP*) STARPU_BLOCK_GET_PTR(descr[7]);
    const FP* v2sz = (FP*) STARPU_BLOCK_GET_PTR(descr[8]);
    const FP* v2pn = (FP*) STARPU_BLOCK_GET_PTR(descr[9]);

    // w at (i, j, k) of t[0]
    FP *const pwwrite = (FP*) STARPU_BLOCK_GET_PTR(descr[10]);
    // primary wave
    // STARPU_R, // r at (i, j, k) of t[1]
    const FP* pwcentralt1 = (FP*) STARPU_BLOCK_GET_PTR(descr[11]);
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
    const FP* pwip0jp0km1 = (FP*) STARPU_BLOCK_GET_PTR(descr[12]);
    const FP* pwip0jm1km1 = (FP*) STARPU_BLOCK_GET_PTR(descr[13]);
    const FP* pwim1jp0km1 = (FP*) STARPU_BLOCK_GET_PTR(descr[14]);
    const FP* pwip1jp0km1 = (FP*) STARPU_BLOCK_GET_PTR(descr[15]);
    const FP* pwip0jp1km1 = (FP*) STARPU_BLOCK_GET_PTR(descr[16]);

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
    const FP* pwim1jm1kp0 = (FP*) STARPU_BLOCK_GET_PTR(descr[17]);
    const FP* pwip0jm1kp0 = (FP*) STARPU_BLOCK_GET_PTR(descr[18]);
    const FP* pwip1jm1kp0 = (FP*) STARPU_BLOCK_GET_PTR(descr[19]);
    const FP* pwim1jp0kp0 = (FP*) STARPU_BLOCK_GET_PTR(descr[20]);
    const FP* pwip1jp0kp0 = (FP*) STARPU_BLOCK_GET_PTR(descr[21]);
    const FP* pwim1jp1kp0 = (FP*) STARPU_BLOCK_GET_PTR(descr[22]);
    const FP* pwip0jp1kp0 = (FP*) STARPU_BLOCK_GET_PTR(descr[23]);
    const FP* pwip1jp1kp0 = (FP*) STARPU_BLOCK_GET_PTR(descr[24]);

    // // layer when k + 1
    // // o x o
    // // x x x 
    // // o x o
    // STARPU_R, // r at (i + 0, j + 0, k + 1) of t[1]
    // STARPU_R, // r at (i + 0, j - 1, k + 1) of t[1]
    // STARPU_R, // r at (i - 1, j + 0, k + 1) of t[1]
    // STARPU_R, // r at (i + 1, j + 0, k + 1) of t[1]
    // STARPU_R, // r at (i + 0, j + 1, k + 1) of t[1]
    const FP* pwip0jp0kp1 = (FP*) STARPU_BLOCK_GET_PTR(descr[25]);
    const FP* pwip0jm1kp1 = (FP*) STARPU_BLOCK_GET_PTR(descr[26]);
    const FP* pwim1jp0kp1 = (FP*) STARPU_BLOCK_GET_PTR(descr[27]);
    const FP* pwip1jp0kp1 = (FP*) STARPU_BLOCK_GET_PTR(descr[28]);
    const FP* pwip0jp1kp1 = (FP*) STARPU_BLOCK_GET_PTR(descr[29]);

    // STARPU_R  // r at (i, j, k) of t[2]
    const FP* pwcentralt2 = (FP*) STARPU_BLOCK_GET_PTR(descr[30]);

    // secondary wave
    FP *const qwwrite = (FP*) STARPU_BLOCK_GET_PTR(descr[31]);

    const FP* qwcentralt1 = (FP*) STARPU_BLOCK_GET_PTR(descr[32]);

    // layer when k - 1
    const FP* qwip0jp0km1 = (FP*) STARPU_BLOCK_GET_PTR(descr[33]);
    const FP* qwip0jm1km1 = (FP*) STARPU_BLOCK_GET_PTR(descr[34]);
    const FP* qwim1jp0km1 = (FP*) STARPU_BLOCK_GET_PTR(descr[35]);
    const FP* qwip1jp0km1 = (FP*) STARPU_BLOCK_GET_PTR(descr[36]);
    const FP* qwip0jp1km1 = (FP*) STARPU_BLOCK_GET_PTR(descr[37]);

    // layer when k
    const FP* qwim1jm1kp0 = (FP*) STARPU_BLOCK_GET_PTR(descr[38]);
    const FP* qwip0jm1kp0 = (FP*) STARPU_BLOCK_GET_PTR(descr[39]);
    const FP* qwip1jm1kp0 = (FP*) STARPU_BLOCK_GET_PTR(descr[40]);
    const FP* qwim1jp0kp0 = (FP*) STARPU_BLOCK_GET_PTR(descr[41]);
    const FP* qwip1jp0kp0 = (FP*) STARPU_BLOCK_GET_PTR(descr[42]);
    const FP* qwim1jp1kp0 = (FP*) STARPU_BLOCK_GET_PTR(descr[43]);
    const FP* qwip0jp1kp0 = (FP*) STARPU_BLOCK_GET_PTR(descr[44]);
    const FP* qwip1jp1kp0 = (FP*) STARPU_BLOCK_GET_PTR(descr[45]);

    // layer when k + 1
    const FP* qwip0jp0kp1 = (FP*) STARPU_BLOCK_GET_PTR(descr[46]);
    const FP* qwip0jm1kp1 = (FP*) STARPU_BLOCK_GET_PTR(descr[47]);
    const FP* qwim1jp0kp1 = (FP*) STARPU_BLOCK_GET_PTR(descr[48]);
    const FP* qwip1jp0kp1 = (FP*) STARPU_BLOCK_GET_PTR(descr[49]);
    const FP* qwip0jp1kp1 = (FP*) STARPU_BLOCK_GET_PTR(descr[50]);

    const FP* qwcentralt2 = (FP*) STARPU_BLOCK_GET_PTR(descr[51]);


    for(size_t z = 0; z < cube_width_z; z++)
    for(size_t y = 0; y < cube_width_y; y++)
    for(size_t x = 0; x < cube_width_x; x++){
        const size_t idx = cube_idx(x, y, z);

        if(
            (z < z_start || z >= z_end) || 
            (y < y_start || y >= y_end) || 
            (x < x_start || x >= x_end)
        ){
            pwwrite[idx] = FP_LIT(0.0);
            qwwrite[idx] = FP_LIT(0.0);
            continue;
        }

        const FP pxx = snd_deriv_dir(pwcentralt1, pwim1jp0kp0, pwip1jp0kp0, x, idx, stride_x, dxxinv, cube_width_x);
        const FP pyy = snd_deriv_dir(pwcentralt1, pwip0jm1kp0, pwip0jp1kp0, y, idx, stride_y, dyyinv, cube_width_y);
        const FP pzz = snd_deriv_dir(pwcentralt1, pwip0jp0km1, pwip0jp0kp1, z, idx, stride_z, dzzinv, cube_width_z);
        const FP pxy = cross_deriv_ddir(
            pwcentralt1, idx, 
            x, pwim1jp0kp0, pwip1jp0kp0, stride_x, 
            y, pwip0jm1kp0, pwip0jp1kp0, stride_y, 
            pwip1jp1kp0, pwip1jm1kp0, pwim1jp1kp0, pwim1jm1kp0,
            cube_width_x, dxyinv
        ); 
        const FP pyz = cross_deriv_ddir(
            pwcentralt1, idx, 
            y, pwip0jm1kp0, pwip0jp1kp0, stride_y, 
            z, pwip0jp0km1, pwip0jp0kp1, stride_z, 
            pwip0jp1kp1, pwip0jp1km1, pwip0jm1kp1, pwip0jm1km1,
            cube_width_y, dyzinv
        ); 
        const FP pxz = cross_deriv_ddir(
            pwcentralt1, idx, 
            x, pwim1jp0kp0, pwip1jp0kp0, stride_x, 
            z, pwip0jp0km1, pwip0jp0kp1, stride_z, 
            pwip1jp0kp1, pwip1jp0km1, pwim1jp0kp1, pwim1jp0km1,
            cube_width_x, dxzinv
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
        const FP qxx = snd_deriv_dir(qwcentralt1, qwim1jp0kp0, qwip1jp0kp0, x, idx, stride_x, dxxinv, cube_width_x);
        const FP qyy = snd_deriv_dir(qwcentralt1, qwip0jm1kp0, qwip0jp1kp0, y, idx, stride_y, dyyinv, cube_width_y);
        const FP qzz = snd_deriv_dir(qwcentralt1, qwip0jp0km1, qwip0jp0kp1, z, idx, stride_z, dzzinv, cube_width_z);
        const FP qxy = cross_deriv_ddir(
            qwcentralt1, idx, 
            x, qwim1jp0kp0, qwip1jp0kp0, stride_x, 
            y, qwip0jm1kp0, qwip0jp1kp0, stride_y, 
            qwip1jp1kp0, qwip1jm1kp0, qwim1jp1kp0, qwim1jm1kp0,
            cube_width_x, dxyinv
        ); 
        const FP qyz = cross_deriv_ddir(
            qwcentralt1, idx, 
            y, qwip0jm1kp0, qwip0jp1kp0, stride_y, 
            z, qwip0jp0km1, qwip0jp0kp1, stride_z, 
            qwip0jp1kp1, qwip0jp1km1, qwip0jm1kp1, qwip0jm1km1,
            cube_width_y, dyzinv
        ); 
        const FP qxz = cross_deriv_ddir(
            qwcentralt1, idx, 
            x, qwim1jp0kp0, qwip1jp0kp0, stride_x, 
            z, qwip0jp0km1, qwip0jp0kp1, stride_z, 
            qwip1jp0kp1, qwip1jp0km1, qwim1jp0kp1, qwim1jp0km1,
            cube_width_x, dxzinv
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
        pwwrite[idx] = FP_LIT(2.0) * pwcentralt1[idx] - pwcentralt2[idx] + rhsp * dt * dt;
        qwwrite[idx] = FP_LIT(2.0) * qwcentralt1[idx] - qwcentralt2[idx] + rhsq * dt * dt;
    }
}

void perturbation_kernel(void *descr[], void *cl_args){
    const perturb_args_t* args = (perturb_args_t*) cl_args;

    const size_t idx = args->source_idx;
    const FP value = args->perturb_value;

    FP *const p_wave_block = (FP *const) STARPU_BLOCK_GET_PTR(descr[0]);
    FP *const q_wave_block = (FP *const) STARPU_BLOCK_GET_PTR(descr[1]);

    printf("%ld: %.9f + %.9f\n", args->t, p_wave_block[idx], value);

    p_wave_block[idx] += value;
    q_wave_block[idx] += value;
}