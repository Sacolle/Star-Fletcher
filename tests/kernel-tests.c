#include <criterion/criterion.h>
#include <criterion/theories.h>
#include <criterion/new/assert.h>
#include <criterion/logging.h>

#include "macros.h"
#include "derivatives.h"
#include "medium.h"
#include "mem.h"

#include <starpu.h>

#include <stdio.h>

#define Der2(p, i, s, d2inv) ((K0*p[i]+ K1*(p[i+s]+p[i-s])+ K2*(p[i+2*s]+p[i-2*s]) + K3*(p[i+3*s]+p[i-3*s]) + K4*(p[i+4*s]+p[i-4*s]))*(d2inv))
#define DerCross(p, i, s11, s21, dinv) ((L11*(p[i+s21+s11]-p[i+s21-s11]-p[i-s21+s11]+p[i-s21-s11])+                                                                                    \
       L12*(p[i+s21+(2*s11)]-p[i+s21-(2*s11)]-p[i-s21+(2*s11)]+p[i-s21-(2*s11)]+p[i+(2*s21)+s11]-p[i+(2*s21)-s11]-p[i-(2*s21)+s11]+p[i-(2*s21)-s11])+                                  \
       L13*(p[i+s21+(3*s11)]-p[i+s21-(3*s11)]-p[i-s21+(3*s11)]+p[i-s21-(3*s11)]+p[i+(3*s21)+s11]-p[i+(3*s21)-s11]-p[i-(3*s21)+s11]+p[i-(3*s21)-s11])+                                  \
       L14*(p[i+s21+(4*s11)]-p[i+s21-(4*s11)]-p[i-s21+(4*s11)]+p[i-s21-(4*s11)]+p[i+(4*s21)+s11]-p[i+(4*s21)-s11]-p[i-(4*s21)+s11]+p[i-(4*s21)-s11])+                                  \
       L22*(p[i+(2*s21)+(2*s11)]-p[i+(2*s21)-(2*s11)]-p[i-(2*s21)+(2*s11)]+p[i-(2*s21)-(2*s11)])+                                                                                      \
       L23*(p[i+(2*s21)+(3*s11)]-p[i+(2*s21)-(3*s11)]-p[i-(2*s21)+(3*s11)]+p[i-(2*s21)-(3*s11)]+p[i+(3*s21)+(2*s11)]-p[i+(3*s21)-(2*s11)]-p[i-(3*s21)+(2*s11)]+p[i-(3*s21)-(2*s11)])+  \
       L24*(p[i+(2*s21)+(4*s11)]-p[i+(2*s21)-(4*s11)]-p[i-(2*s21)+(4*s11)]+p[i-(2*s21)-(4*s11)]+p[i+(4*s21)+(2*s11)]-p[i+(4*s21)-(2*s11)]-p[i-(4*s21)+(2*s11)]+p[i-(4*s21)-(2*s11)])+  \
       L33*(p[i+(3*s21)+(3*s11)]-p[i+(3*s21)-(3*s11)]-p[i-(3*s21)+(3*s11)]+p[i-(3*s21)-(3*s11)])+                                                                                      \
       L34*(p[i+(3*s21)+(4*s11)]-p[i+(3*s21)-(4*s11)]-p[i-(3*s21)+(4*s11)]+p[i-(3*s21)-(4*s11)]+p[i+(4*s21)+(3*s11)]-p[i+(4*s21)-(3*s11)]-p[i-(4*s21)+(3*s11)]+p[i-(4*s21)-(3*s11)])+  \
       L44*(p[i+(4*s21)+(4*s11)]-p[i+(4*s21)-(4*s11)]-p[i-(4*s21)+(4*s11)]+p[i-(4*s21)-(4*s11)]))*(dinv))

#define DerCrossPrint(block, i, s11, s21, dinv) \
	printf("Computed operation Macro:\n %.9f *  (\n        %.9f - %.9f - %.9f + %.9f\n    ) +\n    %.9f *  (\n        %.9f - %.9f - %.9f + %.9f + \n        %.9f - %.9f - %.9f + %.9f\n    ) +\n    %.9f *  (\n        %.9f - %.9f - %.9f + %.9f +\n        %.9f - %.9f - %.9f + %.9f\n    ) +\n    %.9f *  (\n        %.9f - %.9f - %.9f + %.9f +\n        %.9f - %.9f - %.9f + %.9f\n    ) +\n    %.9f *  (\n        %.9f - %.9f - %.9f + %.9f\n    ) +\n    %.9f *  (\n        %.9f - %.9f - %.9f + %.9f + \n        %.9f - %.9f - %.9f + %.9f\n	) +        \n    %.9f *  (\n        %.9f - %.9f - %.9f + %.9f + \n        %.9f - %.9f - %.9f + %.9f\n	) +      \n    %.9f *  (\n        %.9f - %.9f - %.9f + %.9f\n\n	) + \n    %.9f *  (\n        %.9f - %.9f - %.9f + %.9f + \n        %.9f - %.9f - %.9f + %.9f\n	) + \n    %.9f *  (\n        %.9f - %.9f - %.9f + %.9f\n    )) * %.9f\n", \
       L11, block[i+s21+s11], block[i+s21-s11], block[i-s21+s11], block[i-s21-s11],                                                                                    \
       L12, block[i+s21+(2*s11)], block[i+s21-(2*s11)], block[i-s21+(2*s11)], block[i-s21-(2*s11)], block[i+(2*s21)+s11], block[i+(2*s21)-s11], block[i-(2*s21)+s11], block[i-(2*s21)-s11],                                  \
       L13, block[i+s21+(3*s11)], block[i+s21-(3*s11)], block[i-s21+(3*s11)], block[i-s21-(3*s11)], block[i+(3*s21)+s11], block[i+(3*s21)-s11], block[i-(3*s21)+s11], block[i-(3*s21)-s11],                                  \
       L14, block[i+s21+(4*s11)], block[i+s21-(4*s11)], block[i-s21+(4*s11)], block[i-s21-(4*s11)], block[i+(4*s21)+s11], block[i+(4*s21)-s11], block[i-(4*s21)+s11], block[i-(4*s21)-s11],                                  \
       L22, block[i+(2*s21)+(2*s11)], block[i+(2*s21)-(2*s11)], block[i-(2*s21)+(2*s11)], block[i-(2*s21)-(2*s11)],                                                                                      \
       L23, block[i+(2*s21)+(3*s11)], block[i+(2*s21)-(3*s11)], block[i-(2*s21)+(3*s11)], block[i-(2*s21)-(3*s11)], block[i+(3*s21)+(2*s11)], block[i+(3*s21)-(2*s11)], block[i-(3*s21)+(2*s11)], block[i-(3*s21)-(2*s11)],  \
       L24, block[i+(2*s21)+(4*s11)], block[i+(2*s21)-(4*s11)], block[i-(2*s21)+(4*s11)], block[i-(2*s21)-(4*s11)], block[i+(4*s21)+(2*s11)], block[i+(4*s21)-(2*s11)], block[i-(4*s21)+(2*s11)], block[i-(4*s21)-(2*s11)],  \
       L33, block[i+(3*s21)+(3*s11)], block[i+(3*s21)-(3*s11)], block[i-(3*s21)+(3*s11)], block[i-(3*s21)-(3*s11)],                                                                                      \
       L34, block[i+(3*s21)+(4*s11)], block[i+(3*s21)-(4*s11)], block[i-(3*s21)+(4*s11)], block[i-(3*s21)-(4*s11)], block[i+(4*s21)+(3*s11)], block[i+(4*s21)-(3*s11)], block[i-(4*s21)+(3*s11)], block[i-(4*s21)-(3*s11)],  \
       L44, block[i+(4*s21)+(4*s11)], block[i+(4*s21)-(4*s11)], block[i-(4*s21)+(4*s11)], block[i-(4*s21)-(4*s11)], dinv)


void base_computation_implementation(
    float dt, float dx, float dy, float dz, 
    size_t i, size_t med_i, size_t med_j, 
    float* pc, float* pp, float* qc, float* qp
){
    const int strideX = 1;
    const int strideY = g_volume_width;
    const int strideZ = g_volume_width * g_volume_width;

    const float dxxinv=1.0f/(dx*dx);
    const float dyyinv=1.0f/(dy*dy);
    const float dzzinv=1.0f/(dz*dz);
    const float dxyinv=1.0f/(dx*dy);
    const float dxzinv=1.0f/(dx*dz);
    const float dyzinv=1.0f/(dy*dz);

    // p derivatives, H1(p) and H2(p)

    const float pxx= Der2(pc, i, strideX, dxxinv);
    const float pyy= Der2(pc, i, strideY, dyyinv);
    const float pzz= Der2(pc, i, strideZ, dzzinv);
    const float pxy= DerCross(pc, i, strideX, strideY, dxyinv);
    const float pyz= DerCross(pc, i, strideY, strideZ, dyzinv);
    const float pxz= DerCross(pc, i, strideX, strideZ, dxzinv);

    const float cpxx=g_ch1dxx[med_i][med_j]*pxx;
    const float cpyy=g_ch1dyy[med_i][med_j]*pyy;
    const float cpzz=g_ch1dzz[med_i][med_j]*pzz;
    const float cpxy=g_ch1dxy[med_i][med_j]*pxy;
    const float cpxz=g_ch1dxz[med_i][med_j]*pxz;
    const float cpyz=g_ch1dyz[med_i][med_j]*pyz;
    const float h1p=cpxx+cpyy+cpzz+cpxy+cpxz+cpyz;
    const float h2p=pxx+pyy+pzz-h1p;

    // q derivatives, H1(q) and H2(q)

    const float qxx= Der2(qc, i, strideX, dxxinv);
    const float qyy= Der2(qc, i, strideY, dyyinv);
    const float qzz= Der2(qc, i, strideZ, dzzinv);
    const float qxy= DerCross(qc, i, strideX,  strideY, dxyinv);
    const float qyz= DerCross(qc, i, strideY,  strideZ, dyzinv);
    const float qxz= DerCross(qc, i, strideX,  strideZ, dxzinv);

    const float cqxx=g_ch1dxx[med_i][med_j]*qxx;
    const float cqyy=g_ch1dyy[med_i][med_j]*qyy;
    const float cqzz=g_ch1dzz[med_i][med_j]*qzz;
    const float cqxy=g_ch1dxy[med_i][med_j]*qxy;
    const float cqxz=g_ch1dxz[med_i][med_j]*qxz;
    const float cqyz=g_ch1dyz[med_i][med_j]*qyz;
    const float h1q=cqxx+cqyy+cqzz+cqxy+cqxz+cqyz;
    const float h2q=qxx+qyy+qzz-h1q;

    // p-q derivatives, H1(p-q) and H2(p-q)

    const float h1pmq=h1p-h1q;
    const float h2pmq=h2p-h2q;

    // rhs of p and q equations

    const float rhsp=g_v2px[med_i][med_j]*h2p + g_v2pz[med_i][med_j]*h1q + g_v2sz[med_i][med_j]*h1pmq;
    const float rhsq=g_v2pn[med_i][med_j]*h2p + g_v2pz[med_i][med_j]*h1q - g_v2sz[med_i][med_j]*h2pmq;

    // new p and q

    pp[i]=2.0f*pc[i] - pp[i] + rhsp*dt*dt;
    qp[i]=2.0f*qc[i] - qp[i] + rhsq*dt*dt;
}

#define SEED 42

#define EPSILON FLT_EPSILON
#define FP_CRIT flt

#define BORDER_WIDTH 4

#define TRY(x) TRYTO(x, {}, failed_setup)

void setup_seed(void){
    srand(SEED);
}

size_t g_volume_width;
size_t g_cube_width;
size_t g_width_in_cubes;

const size_t absorb_width = 4;

FP* g_volume_matrix_pp;
FP* g_volume_matrix_pc;
FP* g_volume_matrix_qp;
FP* g_volume_matrix_qc;
FP** g_segment_matrix_p[3];
FP** g_segment_matrix_q[3];

FP **g_ch1dxx, **g_ch1dyy, **g_ch1dzz, **g_ch1dxy, **g_ch1dyz, **g_ch1dxz, **g_v2px, **g_v2pz, **g_v2sz, **g_v2pn;

vector(void*) allocs = NULL;

void build_matricies(){
    setup_seed();

    // use 3 because it simplifies
    g_width_in_cubes = 3;
    
    //can change to see the effect of diferent values
    g_cube_width = 16;
    g_volume_width = g_cube_width * g_width_in_cubes;

    const enum Form form = TTI;
    
    vector(void*) medium_allocs = NULL;
    FP *vpz, *vsv, *epsilon, *delta, *phi, *theta;
    const size_t medium_size = sizeof(FP) * CUBE(g_volume_width);

    TRY(mem_allocate(medium_allocs, (void**) &vpz, medium_size));
    TRY(mem_allocate(medium_allocs, (void**) &vsv, medium_size));
    TRY(mem_allocate(medium_allocs, (void**) &epsilon, medium_size));
    TRY(mem_allocate(medium_allocs, (void**) &delta, medium_size));
    TRY(mem_allocate(medium_allocs, (void**) &phi, medium_size));
    TRY(mem_allocate(medium_allocs, (void**) &theta, medium_size));

    // inicialize the buffers above based on the type of medium
    medium_initialize(form, CUBE(g_volume_width), vpz, vsv, epsilon, delta, phi, theta);
    
    // set the absorption zone for vpz and vsv
    medium_random_velocity_boundary(BORDER_WIDTH, absorb_width, vpz, vsv);

    #define ALLOCATE_NESTED_BUFFER(v, cubes, sizes) \
        TRY(mem_allocate(allocs, (void**) &v, CUBE(cubes) * sizeof(FP*))); \
        for(size_t i = 0; i < CUBE(cubes); i++) \
            TRY(mem_allocate(allocs, (void**)(v + i), CUBE(sizes) * sizeof(FP)));

    ALLOCATE_NESTED_BUFFER(g_ch1dxx, g_width_in_cubes, g_cube_width);
    ALLOCATE_NESTED_BUFFER(g_ch1dyy, g_width_in_cubes, g_cube_width);
    ALLOCATE_NESTED_BUFFER(g_ch1dzz, g_width_in_cubes, g_cube_width);
    ALLOCATE_NESTED_BUFFER(g_ch1dxy, g_width_in_cubes, g_cube_width);
    ALLOCATE_NESTED_BUFFER(g_ch1dyz, g_width_in_cubes, g_cube_width);
    ALLOCATE_NESTED_BUFFER(g_ch1dxz, g_width_in_cubes, g_cube_width);
    ALLOCATE_NESTED_BUFFER(g_v2px, g_width_in_cubes, g_cube_width);
    ALLOCATE_NESTED_BUFFER(g_v2pz, g_width_in_cubes, g_cube_width);
    ALLOCATE_NESTED_BUFFER(g_v2sz, g_width_in_cubes, g_cube_width);
    ALLOCATE_NESTED_BUFFER(g_v2pn, g_width_in_cubes, g_cube_width);

    medium_calc_intermediary_values(
        vpz, vsv, epsilon, delta, phi, theta,
        g_ch1dxx,g_ch1dyy,g_ch1dzz,g_ch1dxy,g_ch1dyz,g_ch1dxz,g_v2px,g_v2pz,g_v2sz,g_v2pn
    );

    //at this point the values for the medium will not be used again
    vector_free_all(medium_allocs, free);

    TRY(mem_allocate(allocs, (void**) &g_volume_matrix_pp, CUBE(g_volume_width) * sizeof(FP))); 
    TRY(mem_allocate(allocs, (void**) &g_volume_matrix_pc, CUBE(g_volume_width) * sizeof(FP))); 
    TRY(mem_allocate(allocs, (void**) &g_volume_matrix_qp, CUBE(g_volume_width) * sizeof(FP))); 
    TRY(mem_allocate(allocs, (void**) &g_volume_matrix_qc, CUBE(g_volume_width) * sizeof(FP))); 

    //TODO: size is diferent due to border
    // I do not acess the inner values of the border ones:
    // so they do not need to be initialized, but i need to be able to acess them
    ALLOCATE_NESTED_BUFFER(g_segment_matrix_p[0], g_width_in_cubes + 2, g_cube_width);
    ALLOCATE_NESTED_BUFFER(g_segment_matrix_p[1], g_width_in_cubes + 2, g_cube_width);
    ALLOCATE_NESTED_BUFFER(g_segment_matrix_p[2], g_width_in_cubes + 2, g_cube_width);

    ALLOCATE_NESTED_BUFFER(g_segment_matrix_q[0], g_width_in_cubes + 2, g_cube_width);
    ALLOCATE_NESTED_BUFFER(g_segment_matrix_q[1], g_width_in_cubes + 2, g_cube_width);
    ALLOCATE_NESTED_BUFFER(g_segment_matrix_q[2], g_width_in_cubes + 2, g_cube_width);

    for(size_t k = 0; k < g_width_in_cubes; k++)
    for(size_t j = 0; j < g_width_in_cubes; j++)
    for(size_t i = 0; i < g_width_in_cubes; i++){
        for(size_t z = 0; z < g_cube_width; z++)
        for(size_t y = 0; y < g_cube_width; y++)
        for(size_t x = 0; x < g_cube_width; x++){
            const FP rand_val = FP_RAND();
            for(size_t d = 0; d < 3; d++){
                g_segment_matrix_p[d][block_idx(i, j, k)][cube_idx(x, y, z)] = 0.0f;
                g_segment_matrix_q[d][block_idx(i, j, k)][cube_idx(x, y, z)] = 0.0f;
            }

            g_volume_matrix_pp[block_cube_to_volume_idx(x, y, z, i, j, k)] = 0.0f;
            g_volume_matrix_pc[block_cube_to_volume_idx(x, y, z, i, j, k)] = 0.0f;

            g_volume_matrix_qp[block_cube_to_volume_idx(x, y, z, i, j, k)] = 0.0f;
            g_volume_matrix_qc[block_cube_to_volume_idx(x, y, z, i, j, k)] = 0.0f;
        }
    }

    return;

    failed_setup:
    vector_free_all(medium_allocs, free);
    vector_free_all(allocs, free);
    cr_assert(false);
}

void teardown_values(){
    vector_free_all(allocs, free);
}

TestSuite(fletcher_kernel, .init = build_matricies, .fini = teardown_values);


Test(fletcher_kernel, compute_one_step) {
    //insert source
    const FP dt = 0.001f;
    const FP dx = 12.5f, dy = 12.5f, dz = 12.5f;
    const FP source = medium_source_value(dt, 0);

    const size_t volume_center_idx = volume_idx(g_volume_width / 2, g_volume_width / 2, g_volume_width / 2);
    const size_t center_cube_idx = volume_to_block_idx(volume_center_idx) + block_idx(1, 1, 1);
    const size_t source_local_cube_idx = volume_to_cube_idx(volume_center_idx);

    g_volume_matrix_pc[volume_center_idx] += source;
    g_segment_matrix_p[1][center_cube_idx][source_local_cube_idx] += source;

    //compute one propagation from both
    //compare

    #define ASBLK(ptr) (struct starpu_block_interface) { \
        .id = STARPU_BLOCK_INTERFACE_ID, .ptr = ptr, \
        .nx = g_cube_width, .ny = g_cube_width, .nz = g_cube_width, \
        .ldy = g_cube_width, .ldz = g_cube_width * g_cube_width, .elemsize = sizeof(FP)}

    for(size_t k = 1; k < g_width_in_cubes + 1; k++)
    for(size_t j = 1; j < g_width_in_cubes + 1; j++)
    for(size_t i = 1; i < g_width_in_cubes + 1; i++){

        //TODO:

        for(size_t z = 0; z < g_cube_width; z++)
        for(size_t y = 0; y < g_cube_width; y++)
        for(size_t x = 0; x < g_cube_width; x++){
            const size_t vol_i = block_cube_to_volume_idx(x, y, z, i - 1, j - 1, k - 1);
            const size_t block_i = block_idx(i, j, k);
            const size_t cube_i = cube_idx(x, y, z);

            base_computation_implementation(dt, dx, dy, dz, 
                vol_i, block_i, cube_i, 
                g_volume_matrix_pc, g_volume_matrix_pp, g_volume_matrix_qc, g_volume_matrix_qp
            );


            //put da buffers in the structure
            //assert with pp
    }
}


/*
#define FSTBLK(blk) ((blk) == 1)
#define LSTBLK(blk) ((blk) == g_width_in_cubes)
#define START(blk, idx) (FSTBLK(blk) && (idx) < BORDER_WIDTH)
#define END(blk, idx) (LSTBLK(blk) && (idx) >= (g_cube_width - BORDER_WIDTH))
#define INEDGE(blk_idx, cidx) (START(blk_idx, cidx) || END(blk_idx, cidx))

#define EPSILON 0.0001
int has_clear_edge(FP* block, size_t i, size_t j, size_t k){
    for(size_t z = 0; z < g_cube_width; z++){
        for(size_t y = 0; y < g_cube_width; y++){
            for(size_t x = 0; x < g_cube_width; x++){
                const size_t idx = cube_idx(x, y, z);
                if(INEDGE(k, z) || INEDGE(j, y) || INEDGE(i, x)){
                    if(block[idx] < EPSILON || block[idx] > -EPSILON){

                    }else{
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}*/