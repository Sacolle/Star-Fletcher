#include "derivatives.h"


#include<stdbool.h>



#define KERNEL_SIZE 4

extern uint64_t g_cube_width;

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
// pois a direção correlaciona ao stride
// Portanto
// flip no eixo X deve-se passar stride = 1
// flip no eixo Y deve-se passar stride = cube_width
// flip no eixo Y deve-se passar stride = cube_width²
static inline flip(const size_t line_idx, const size_t idx, const size_t stride){
    return (stride * g_cube_width - stride) - 2 * line_idx * stride + idx;
}

//NOTE: this might do some insane overflow errors
//TODO: 
FP deriv_dir(
    FP* block, FP* block_minus, FP* block_plus, 
    const int dir, const size_t base_idx, const int stride, 
    const FP dinv, FP aggr, const FP const ls[4]
) {  
    // sign is either -1 or 1.
    for(int sign = -1; sign <= 1; sign += 2){
        FP* access_block = block;
        size_t idx = base_idx;
        int line_idx = dir;
        for(int k = 1; k <= KERNEL_SIZE; k++){  
            line_idx += sign;

            if(line_idx >= g_cube_width){
                access_block = block_plus;
                line_idx = 0;
                idx = flip(0, idx, stride);
            }else if(line_idx < 0){
                access_block = block_minus;
                line_idx = g_cube_width;
                idx = flip(g_cube_width, idx, stride);
            }else{
                idx = (idx + sign * stride);
            }

            const FP coef = ls[k - 1];
            // if overflowed access the other buffer
            aggr += sign * coef * access_block[idx];  
        }  
    }
    return aggr * dinv;  
}

const FP const fst_deriv_coef[4] = {L1, L2, L3, L4};  
/*
    #define Der1(p, i, s, dinv) (
    L1 * (p[i + s] - p[i - s]) + 
    L2 * (p[i + 2 * s] - p[i - 2 * s]) + 
    L3 * (p[i + 3 * s] - p[i - 3 * s]) + 
    L4 * (p[i + 4 * s] - p[i - 4 * s])
    ) * (dinv)
*/
FP fst_deriv_dir(
    FP* block, FP* block_minus, FP* block_plus, 
    const int dir, const size_t base_idx, const int stride, 
    const FP dinv 
){
    return deriv_dir(
        block, block_minus, block_plus, 
        dir, base_idx, stride, 
        dinv, 0.0, fst_deriv_coef
    );
}

const FP const snd_deriv_coef[4] = {K1, K2, K3, K4};  
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
    FP* block, FP* block_minus, FP* block_plus, 
    const int dir, const size_t base_idx, const int stride, 
    const FP d2inv 
){
    const FP center = K0 * block[base_idx];
    return deriv_dir(
        block, block_minus, block_plus, 
        dir, base_idx, stride,  
        d2inv, center, snd_deriv_coef
    );
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
#define LSSIZE 4

//usando uma tupla booleana para simplificar os casos
#define TUP(t1, t2) ((t1 << 1) | t2)

//TODO: validar esse bloco
FP cross_deriv_ddir(
    FP* block, const size_t base_idx,
    const size_t dir1, FP* block_minus_d1, FP* block_plus_d1, const size_t stride_d1,
    const size_t dir2, FP* block_minus_d2, FP* block_plus_d2, const size_t stride_d2,
    FP* block_diagonal_plus_plus, FP* block_diagonal_plus_minus, 
    FP* block_diagonal_minus_plus, FP* block_diagonal_minus_minus
){
    const FP const ls[LSSIZE * LSSIZE] = {
        L11, L12, L13, L14,
        L12, L22, L23, L24,
        L13, L23, L33, L34,
        L14, L24, L34, L44,
    };
    FP aggr = 0.0;  

    // do for minus the idx and plus the idx
    // sign is either -1 or 1.
    for(int sign1 = -1; sign1 <= 1; sign1 += 2){
        const bool s1neg = sign1 == -1;
        FP* other_block_d1 = s1neg ? block_minus_d1 : block_plus_d1; 
        for(int sign2 = -1; sign2 <= 1; sign2 += 2){
            const bool s2neg = sign2 == -1;
            FP* other_block_d2 = s2neg ? block_minus_d2 : block_plus_d2; 

            //find which diagonal is going to be possibly acessed
            FP* diagonal_block;
            switch (TUP(s1neg, s2neg)){
                case (TUP(true, true)): diagonal_block = block_diagonal_minus_minus; break;
                case (TUP(true, false)): diagonal_block = block_diagonal_minus_plus; break;
                case (TUP(false, true)): diagonal_block = block_diagonal_plus_minus; break;
                case (TUP(false, false)): diagonal_block = block_diagonal_plus_plus; break;
            }

            size_t idx = base_idx;

            for(int k1 = 1; k1 <= KERNEL_SIZE; k1++){
                const int dim_idx_dir1 = dir1 + (k1 * sign1);
                const bool overflow_dir1 = dim_idx_dir1 >= g_cube_width || dim_idx_dir1 < 0;

                // move idx to the next spot in this direction
                idx = overflow_dir1 ? flip(dim_idx_dir1 - sign1, idx, stride_d1) : (idx + sign1 * stride_d1);

                for(int k2 = 1; k2 <= KERNEL_SIZE; k2++){
                    const FP coef = ls[(k1 - 1) * LSSIZE + (k2 - 1)];

                    const int dim_idx_dir2 = dir2 + (k2 * sign2);
                    const bool overflow_dir2 = dim_idx_dir2 >= g_cube_width || dim_idx_dir2 < 0;

                    // move in this direction
                    idx = overflow_dir2 ? flip(dim_idx_dir2 - sign2, idx, stride_d2) : (idx + sign2 * stride_d2);
                    
                    // comparação como se fosse tuplas
                    FP* acess_block;
                    switch (TUP(overflow_dir1, overflow_dir2)){
                        case (TUP(true, true)): acess_block = diagonal_block; break;
                        case (TUP(true, false)): acess_block = other_block_d1; break;
                        case (TUP(false, true)): acess_block = other_block_d2; break;
                        case (TUP(false, false)): acess_block = block; break;
                    }

                    aggr += coef * (sign1 * sign2) * (acess_block[idx]);
                }
            }
        }
    }

}