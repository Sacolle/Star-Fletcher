#include "derivatives.h"

FP snd_deriv_dir_pos(
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
FP snd_deriv_dir_neg(
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

FP snd_deriv_dir(
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
/*

    // assumindo s11 sendo o stride de x e s21 sendo o stride em Y
    // com uma notação vetorial tem a redução do https://github.com/gabrielfrtg/fletcher-base/blob/main/original/derivatives.h
    DerCross(p, i, strideX, strideY) 
    L11 * (
        p[i + (1,1,0)] - p[i + (-1,1,0)] - p[i + (1,-1,0)] + p[i + (-1,-1,0)]
    ) +
    L12 * (
        p[i + (2,1,0)] - p[i + (-2,1,0)] - p[i +(2,-1,0)] + p[i + (-2,-1,0)] + 
        p[i + (1,2,0)] - p[i + (-1,2,0)] - p[i +(1,-2,0)] + p[i + (-1,-2,0)]
    ) + 
    L13 * (
        p[i + (3,1,0)] - p[i + (-3,1,0)] - p[i +(3,-1,0)] + p[i + (-3,-1,0)] + 
        p[i + (1,3,0)] - p[i + (-1,3,0)] - p[i +(1,-3,0)] + p[i + (-1,-3,0)]
    ) +
    L14 * (
        p[i + (4,1,0)] - p[i + (-4,1,0)] - p[i +(4,-1,0)] + p[i + (-4,-1,0)] + 
        p[i + (1,4,0)] - p[i + (-4,1,0)] - p[i +(1,-4,0)] + p[i + (-1,-4,0)]
    ) +
    L22 * (
        p[i + (2,2,0)] - p[i + (-2,2,0)] - p[i +(2,-2,0)] + p[i + (-2,-2,0)]
    ) + 
    L23 * (
        p[i + (3,2,0)] - p[i + (-3,2,0)] - p[i +(3,-2,0)] + p[i + (-3,-2,0)] + 
        p[i + (2,3,0)] - p[i + (-2,3,0)] - p[i +(2,-3,0)] + p[i + (-2,-3,0)]
    ) + 
    L24 * (
        p[i + (4,2,0)] - p[i + (-4,2,0)] - p[i +(4,-2,0)] + p[i + (-4,-2,0)] + 
        p[i + (2,4,0)] - p[i + (-2,4,0)] - p[i +(2,-4,0)] + p[i + (-2,-4,0)]
    ) +
    L33 * (
        p[i + (3,3,0)] - p[i + (-3,3,0)] - p[i +(3,-3,0)] + p[i + (-3,-3,0)]
    ) +
    L34 * (
        p[i + (4,3,0)] - p[i + (-4,3,0)] - p[i +(4,-3,0)] + p[i + (-4,-3,0)] +
        p[i + (3,4,0)] - p[i + (-3,4,0)] - p[i +(3,-4,0)] + p[i + (-3,-4,0)] 
    ) +
    L44 * (
        p[i + (4,4,0)] - p[i + (-4,4,0)] - p[i +(4,-4,0)] + p[i + (-4,-4,0)]
    ) 
*/
/*



*/
#include "cross-deriv.gen.c"


FP cross_deriv_ddir(
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