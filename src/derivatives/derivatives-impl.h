#ifndef _DERIVATIVES_GUARD_
#define _DERIVATIVES_GUARD_

#include <stddef.h>

#include "floatingpoint.h"

#ifdef CUDA_CODE
#define ATTRIBUTE __device__
#else
#define ATTRIBUTE
#endif

#define L1 FP_LIT(0.8)                    // 4/5
#define L2 FP_LIT(-0.2)                   // -1/5
#define L3 FP_LIT(0.0380952380952381)     // 4/105
#define L4 FP_LIT(-0.0035714285714285713) // -1/280

// eight order finite differences coefficients of the cross second derivative

#define L11 FP_LIT(0.64)                    // L1*L1
#define L12 FP_LIT(-0.16)                   // L1*L2
#define L13 FP_LIT(0.03047619047619047618)  // L1*L2
#define L14 FP_LIT(-0.00285714285714285713) // L1*L4
#define L22 FP_LIT(0.04)                    // L2*L2
#define L23 FP_LIT(-0.00761904761904761904) // L2*L3
#define L24 FP_LIT(0.00071428571428571428)  // L2*L4
#define L33 FP_LIT(0.00145124716553287981)  // L3*L3
#define L34 FP_LIT(-0.00013605442176870748) // L3*L4
#define L44 FP_LIT(0.00001275510204081632)  // L4*L4

// eight order finite differences coefficients of the second derivative
#define K0 FP_LIT(-2.84722222222222222222) // -205/72
#define K1 FP_LIT(1.6)                     // 8/5
#define K2 FP_LIT(-0.2)                    // -1/5
#define K3 FP_LIT(0.02539682539682539682)  // 8/315
#define K4 FP_LIT(-0.00178571428571428571) // -1/560



ATTRIBUTE FP snd_deriv_dir(
    const FP *block, const FP *block_minus, const FP *block_plus, const int dir,
    const size_t base_idx, const int stride, const FP d2inv,
    const int cube_width
);

ATTRIBUTE FP cross_deriv_ddir(
    const FP *block, const size_t base_idx, const size_t dir1,
    const FP *block_minus_d1, const FP *block_plus_d1, const int stride_d1,
    const size_t dir2, const FP *block_minus_d2, const FP *block_plus_d2,
    const int stride_d2, const FP *block_diagonal_plus_plus,
    const FP *block_diagonal_plus_minus, const FP *block_diagonal_minus_plus,
    const FP *block_diagonal_minus_minus, const int cube_width,
    const FP dinv
);

// add the impl
#ifdef CODE_IMPL


// bit tuple for easier switch statments
#define MAX_ITEM_BITS (32 - __builtin_clz(MAX_MASK_NUM))
#define BITMASK_PAIR(x, y) (((x) << MAX_ITEM_BITS) | (y))





// espelha no eixo
// a deirvação baseia-se na fórmula do índice f(x, y, z) = x + Wy + W²z
// f(W - 1 - x, y, z) é o ponto espelhado no eixo x, então na lista 0, 1, 2, 3, 4. g(4) = 0 e g(1) = 3.
// subsitituido isso na fórmula do índice, temos: 
// f(W - 1 - x, y, z) = W - 1 - x + Wy + W²z
// que pode ser simplificado nas seguintes etapas para
// f(W - 1 - x, y, z) = W - 1 - x - x + x + Wy + W²z
// f(W - 1 - x, y, z) = W - 1 - 2x + f(x, y, z)
// As formulas para Y e Z são derivadas da mesma forma sendo:
// f(x, W - 1 - y, z) = W² - W - 2Wy + f(x, y, z)
// f(x, y, W - 1 - z) = W³ - W² - 2W²z + f(x, y, z)
//
// Tudo isso pode ser reduzido para:
// W * stride - stride - 2 * stride * dir + idx
// pois a cada direção tem seu stride associado (x: 1, y: W, z: W²)
// Portanto
// flip no eixo X deve-se passar stride = 1
// flip no eixo Y deve-se passar stride = cube_width
// flip no eixo Z deve-se passar stride = cube_width²
ATTRIBUTE static inline size_t flip(const size_t line_idx, const size_t idx, const int stride, const int cube_width){
    return (stride * cube_width - stride) - 2 * line_idx * stride + idx;
}

#include "./cross-deriv.gen.c"

ATTRIBUTE FP snd_deriv_dir_pos(
    const FP* block, const FP* block_plus, 
    const int dir, const size_t base_idx, const int stride, 
    const FP d2inv, const int cube_width
){
    // get how far the dir is 
    const int depth = cube_width - dir - 1;
    const size_t border_idx = flip(cube_width - 1, base_idx + depth * stride, stride, cube_width);
    switch (depth)
    {
    case 0:
        /* right at the border */
        return (
            K0 * block[base_idx] + 
            K1 * (block_plus[border_idx + 0 * stride] + block[base_idx - 1 * stride]) + 
            K2 * (block_plus[border_idx + 1 * stride] + block[base_idx - 2 * stride]) + 
            K3 * (block_plus[border_idx + 2 * stride] + block[base_idx - 3 * stride]) + 
            K4 * (block_plus[border_idx + 3 * stride] + block[base_idx - 4 * stride])
        ) * (d2inv);
    case 1:
        /* right before the border */
        return (
            K0 * block[base_idx] + 
            K1 * (block[base_idx + 1 * stride] + block[base_idx - 1 * stride]) + 
            K2 * (block_plus[border_idx + 0 * stride] + block[base_idx - 2 * stride]) + 
            K3 * (block_plus[border_idx + 1 * stride] + block[base_idx - 3 * stride]) + 
            K4 * (block_plus[border_idx + 2 * stride] + block[base_idx - 4 * stride])
        ) * (d2inv);
    case 2:
        /* 2 before the border */
        return (
            K0 * block[base_idx] + 
            K1 * (block[base_idx + 1 * stride] + block[base_idx - 1 * stride]) + 
            K2 * (block[base_idx + 2 * stride] + block[base_idx - 2 * stride]) + 
            K3 * (block_plus[border_idx + 0 * stride] + block[base_idx - 3 * stride]) + 
            K4 * (block_plus[border_idx + 1 * stride] + block[base_idx - 4 * stride])
        ) * (d2inv);

    case 3:
        /* 3 before the border */
        return (
            K0 * block[base_idx] + 
            K1 * (block[base_idx + 1 * stride] + block[base_idx - 1 * stride]) + 
            K2 * (block[base_idx + 2 * stride] + block[base_idx - 2 * stride]) + 
            K3 * (block[base_idx + 3 * stride] + block[base_idx - 3 * stride]) + 
            K4 * (block_plus[border_idx + 0 * stride] + block[base_idx - 4 * stride])
        ) * (d2inv);
    
    default:
        /* 4 and less before the border */
        return (
            K0 * block[base_idx] + 
            K1 * (block[base_idx + 1 * stride] + block[base_idx - 1 * stride]) + 
            K2 * (block[base_idx + 2 * stride] + block[base_idx - 2 * stride]) + 
            K3 * (block[base_idx + 3 * stride] + block[base_idx - 3 * stride]) + 
            K4 * (block[base_idx + 4 * stride] + block[base_idx - 4 * stride])
        ) * (d2inv);
    }
}


ATTRIBUTE FP snd_deriv_dir_neg(
    const FP* block, const FP* block_minus, 
    const int dir, const size_t base_idx, const int stride, 
    const FP d2inv, const int cube_width
){
    // get how far the dir is 
    const int depth = dir;
    const size_t border_idx = flip(0, base_idx - depth * stride, stride, cube_width);
    switch (depth)
    {
    case 0:
        /* right at the border */
        return (
            K0 * block[base_idx] + 
            K1 * (block[base_idx + 1 * stride] + block_minus[border_idx - 0 * stride]) + 
            K2 * (block[base_idx + 2 * stride] + block_minus[border_idx - 1 * stride]) + 
            K3 * (block[base_idx + 3 * stride] + block_minus[border_idx - 2 * stride]) + 
            K4 * (block[base_idx + 4 * stride] + block_minus[border_idx - 3 * stride])
        ) * (d2inv);
    case 1:
        /* right before the border */
        return (
            K0 * block[base_idx] + 
            K1 * (block[base_idx + 1 * stride] + block[base_idx - 1 * stride]) + 
            K2 * (block[base_idx + 2 * stride] + block_minus[border_idx - 0 * stride]) + 
            K3 * (block[base_idx + 3 * stride] + block_minus[border_idx - 1 * stride]) + 
            K4 * (block[base_idx + 4 * stride] + block_minus[border_idx - 2 * stride])
        ) * (d2inv);
    case 2:
        /* 2 before the border */
        return (
            K0 * block[base_idx] + 
            K1 * (block[base_idx + 1 * stride] + block[base_idx - 1 * stride]) + 
            K2 * (block[base_idx + 2 * stride] + block[base_idx - 2 * stride]) + 
            K3 * (block[base_idx + 3 * stride] + block_minus[border_idx - 0 * stride]) + 
            K4 * (block[base_idx + 4 * stride] + block_minus[border_idx - 1 * stride])
        ) * (d2inv);

    case 3:
        /* 3 before the border */
        return (
            K0 * block[base_idx] + 
            K1 * (block[base_idx + 1 * stride] + block[base_idx - 1 * stride]) + 
            K2 * (block[base_idx + 2 * stride] + block[base_idx - 2 * stride]) + 
            K3 * (block[base_idx + 3 * stride] + block[base_idx - 3 * stride]) + 
            K4 * (block[base_idx + 4 * stride] + block_minus[border_idx - 0 * stride])
        ) * (d2inv);
    
    default:
        /* 4 and less before the border */
        return (
            K0 * block[base_idx] + 
            K1 * (block[base_idx + 1 * stride] + block[base_idx - 1 * stride]) + 
            K2 * (block[base_idx + 2 * stride] + block[base_idx - 2 * stride]) + 
            K3 * (block[base_idx + 3 * stride] + block[base_idx - 3 * stride]) + 
            K4 * (block[base_idx + 4 * stride] + block[base_idx - 4 * stride])
        ) * (d2inv);
    }
}

ATTRIBUTE FP snd_deriv_dir(
    const FP* block, const FP* block_minus, const FP* block_plus, 
    const int dir, const size_t base_idx, const int stride, 
    const FP d2inv, const int cube_width
){
    // neg case
    if(dir < 4){
        return snd_deriv_dir_neg(block, block_minus, dir, base_idx, stride, d2inv, cube_width);
    }else{
        return snd_deriv_dir_pos(block, block_plus, dir, base_idx, stride, d2inv, cube_width);
    }
}


ATTRIBUTE FP cross_deriv_ddir(
    const FP* block, const size_t base_idx,
    const size_t dir1, const FP* block_minus_d1, const FP* block_plus_d1, const int stride_d1,
    const size_t dir2, const FP* block_minus_d2, const FP* block_plus_d2, const int stride_d2,
    const FP* block_diagonal_plus_plus, const FP* block_diagonal_plus_minus, 
    const FP* block_diagonal_minus_plus, const FP* block_diagonal_minus_minus,
    const int cube_width, const FP dinv
){
    #define MAX_MASK_NUM 1
    // assuming that truth is one
    // 0 is center, 1 is plus and 2 is minus
    int d1 = BITMASK_PAIR(dir1 < 4, dir1 > cube_width - 1 - 4);
    int d2 = BITMASK_PAIR(dir2 < 4, dir2 > cube_width - 1 - 4);
    #undef MAX_MASK_NUM

    #define ARGS block, base_idx, \
        dir1, block_minus_d1, block_plus_d1, stride_d1, \
        dir2, block_minus_d2, block_plus_d2, stride_d2, \
        block_diagonal_plus_plus, block_diagonal_plus_minus, \
        block_diagonal_minus_plus, block_diagonal_minus_minus, \
        cube_width, dinv


    #define MAX_MASK_NUM 2
    switch (BITMASK_PAIR(d1, d2))
    {
    case BITMASK_PAIR(1, 0): return cross_deriv_pos_center(ARGS);
    case BITMASK_PAIR(1, 1): return cross_deriv_pos_pos(ARGS);
    case BITMASK_PAIR(1, 2): return cross_deriv_pos_neg(ARGS);
    case BITMASK_PAIR(2, 0): return cross_deriv_neg_center(ARGS);
    case BITMASK_PAIR(2, 1): return cross_deriv_neg_pos(ARGS);
    case BITMASK_PAIR(2, 2): return cross_deriv_neg_neg(ARGS);
    case BITMASK_PAIR(0, 1): return cross_deriv_center_pos(ARGS);
    case BITMASK_PAIR(0, 2): return cross_deriv_center_neg(ARGS);
    case BITMASK_PAIR(0, 0): 
    default: return cross_deriv_center_center(ARGS);
    }
    #undef MAX_MASK_NUM
}


#endif

#endif /* CODE_IMPL */
