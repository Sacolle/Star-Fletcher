#include "derivatives.h"


#include<stdbool.h>
#include<assert.h>

#include "macros.h"


#define KERNEL_SIZE 4

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
static inline size_t flip(const size_t line_idx, const size_t idx, const int stride, const int cube_width){
    return (stride * cube_width - stride) - 2 * line_idx * stride + idx;
}

// `NOTE`: this might do some insane overflow errors
// Agrega os 4 itens no sentido `sign_for_idx`, na dimensão indicada por `stride`;
// soma ou subtrai dependendo do `sign_for_math`.
FP deriv_kernel_sum(
    const FP* block, const FP* block_minus, const FP* block_plus, 
    const int dir, const int sign_for_idx, const int sign_for_math,
    const size_t base_idx, const int stride, 
    const FP coef[4], const int cube_width
) { 
    FP aggr = 0.0;
    const FP* access_block = block;
    int idx = base_idx;
    int line_idx = dir;
    for(int k = 1; k <= KERNEL_SIZE; k++){  
        line_idx += sign_for_idx;
        if(line_idx >= cube_width){
            access_block = block_plus;
            //flip from the extreme plus to the extreme minus
            idx = flip(cube_width - 1, idx, stride, cube_width);
            //set line idx to be the corret one after the flip
            line_idx = 0;
        }else if(line_idx < 0){
            access_block = block_minus;
            idx = flip(0, idx, stride, cube_width);
            line_idx = cube_width - 1;
        }else{
            idx = (idx + sign_for_idx * stride);
        }
        aggr += sign_for_math * coef[k - 1] * access_block[idx];  
    }  
    return aggr;  
}
/*
    #define Der1(p, i, s, dinv) (
    L1 * (p[i + s] - p[i - s]) + 
    L2 * (p[i + 2 * s] - p[i - 2 * s]) + 
    L3 * (p[i + 3 * s] - p[i - 3 * s]) + 
    L4 * (p[i + 4 * s] - p[i - 4 * s])
    ) * (dinv)
*/
FP fst_deriv_dir(
    const FP* block, const FP* block_minus, const FP* block_plus, 
    const int dir, const size_t base_idx, const int stride, 
    const FP dinv, const int cube_width
){
    static const FP fst_deriv_coef[4] = {L1, L2, L3, L4};  
    FP aggr = 0.0;
    for(int sign = -1; sign <= 1; sign += 2){
        aggr += deriv_kernel_sum(
            block, block_minus, block_plus, 
            dir, sign, sign, base_idx, stride, 
            fst_deriv_coef, cube_width
        );
    }
    return aggr * dinv;
}

/*
    #define Der2(p, i, s, d2inv) ((
    K0 * p[i] + 
    K1 * (p[i + s] + p[i - s]) + 
    K2 * (p[i + 2 * s] + p[i - 2 * s]) + 
    K3 * (p[i + 3 * s] + p[i - 3 * s]) + 
    K4 * (p[i + 4 * s] + p[i - 4 * s])
    ) * (d2inv))

*/

FP snd_deriv_dir(
    const FP* block, const FP* block_minus, const FP* block_plus, 
    const int dir, const size_t base_idx, const int stride, 
    const FP d2inv, const int cube_width
){
    static const FP snd_deriv_coef[4] = {K1, K2, K3, K4};  
    FP aggr = K0 * block[base_idx];
    for(int sign = -1; sign <= 1; sign += 2){
        aggr += deriv_kernel_sum(
            block, block_minus, block_plus, 
            dir, sign, 1, base_idx, stride, 
            snd_deriv_coef, cube_width
        );
    }
    return aggr * d2inv;
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

#define WIDTH_OF_BLOCKPOS_MASK 2
enum BlockPos {
    Center = 0, 
    Minus = 1, 
    Plus = 2
};

#define LSSIZE 4

//usando uma tupla para poder usar o switch na hora de determinar os casos
#define TUP(t1, t2) (((t1) << WIDTH_OF_BLOCKPOS_MASK) | (t2))

//const bool s2neg = sign2 == -1;
//FP* other_block_d2 = s2neg ? block_minus_d2 : block_plus_d2; 

FP cross_deriv_ddir(
    const FP* block, const size_t base_idx,
    const size_t dir1, const FP* block_minus_d1, const FP* block_plus_d1, const int stride_d1,
    const size_t dir2, const FP* block_minus_d2, const FP* block_plus_d2, const int stride_d2,
    const FP* block_diagonal_plus_plus, const FP* block_diagonal_plus_minus, 
    const FP* block_diagonal_minus_plus, const FP* block_diagonal_minus_minus,
    const int cube_width
){
    static const FP ls[LSSIZE * LSSIZE] = {
        L11, L12, L13, L14,
        L12, L22, L23, L24,
        L13, L23, L33, L34,
        L14, L24, L34, L44,
    };
    FP aggr = 0.0;  

    // do for minus the idx and plus the idx
    // sign is either -1 or 1.
    for(int sign1 = -1; sign1 <= 1; sign1 += 2){
        for(int sign2 = -1; sign2 <= 1; sign2 += 2){

            int line_idx1 = dir1;
            enum BlockPos access_block_d1 = Center;
            size_t outer_idx = base_idx;
            for(int k1 = 1; k1 <= KERNEL_SIZE; k1++){
                line_idx1 += sign1;

                if(line_idx1 >= cube_width){
                    access_block_d1 = Plus;
                    outer_idx = flip(cube_width - 1, outer_idx, stride_d1, cube_width);
                    line_idx1 = 0;
                }else if(line_idx1 < 0){
                    access_block_d1 = Minus;
                    outer_idx = flip(0, outer_idx, stride_d1, cube_width);
                    line_idx1 = cube_width - 1;
                }else{
                    outer_idx = (outer_idx + sign1 * stride_d1);
                }
                //loop variables
                int line_idx2 = dir2;
                enum BlockPos access_block_d2 = Center;
                size_t idx = outer_idx;
                for(int k2 = 1; k2 <= KERNEL_SIZE; k2++){
                    line_idx2 += sign2;

                    if(line_idx2 >= cube_width){
                        access_block_d2 = Plus;
                        idx = flip(cube_width - 1, idx, stride_d2, cube_width);
                        line_idx2 = 0;
                    }else if(line_idx2 < 0){
                        access_block_d2 = Minus;
                        idx = flip(0, idx, stride_d2, cube_width);
                        line_idx2 = cube_width - 1;
                    }else{
                        idx = (idx + sign2 * stride_d2);
                    }
                    const FP* access_block;
                    switch (TUP(access_block_d1, access_block_d2)){
                    case TUP(Minus, Minus):  access_block = block_diagonal_minus_minus; break;
                    case TUP(Minus, Center): access_block = block_minus_d1; break;
                    case TUP(Minus, Plus):   access_block = block_diagonal_minus_plus; break;
                    case TUP(Center, Minus): access_block = block_minus_d2; break;
                    case TUP(Center, Center):access_block = block; break;
                    case TUP(Center, Plus):  access_block = block_plus_d2; break;
                    case TUP(Plus, Minus):   access_block = block_diagonal_plus_minus; break;
                    case TUP(Plus, Center):  access_block = block_plus_d1; break;
                    case TUP(Plus, Plus):    access_block = block_diagonal_plus_plus; break;
                    default:
                        assert(false && "Unreachable!");
                        break;
                    }
                    const FP coef = ls[(k1 - 1) * LSSIZE + (k2 - 1)];

                    aggr += coef * (sign1 * sign2) * (access_block[idx]);
                }
            }
        }
    }
    return aggr;
}