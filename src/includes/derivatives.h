#ifndef _DERIVATIVE_GUARD
#define _DERIVATIVE_GUARD

#include <stdint.h>

#include <stddef.h>

#include "floatingpoint.h"

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
static inline size_t flip(const size_t line_idx, const size_t idx, const int stride, const int cube_width){
    return (stride * cube_width - stride) - 2 * line_idx * stride + idx;
}

FP snd_deriv_dir(
    const FP* block, const FP* block_minus, const FP* block_plus, 
    const int dir, const size_t base_idx, const int stride, 
    const FP d2inv, const int cube_width 
);

FP cross_deriv_ddir(
    const FP* block, const size_t base_idx,
    const size_t dir1, const FP* block_minus_d1, const FP* block_plus_d1, const int stride_d1,
    const size_t dir2, const FP* block_minus_d2, const FP* block_plus_d2, const int stride_d2,
    const FP* block_diagonal_plus_plus, const FP* block_diagonal_plus_minus, 
    const FP* block_diagonal_minus_plus, const FP* block_diagonal_minus_minus,
    const int cube_width, const FP dinv
);


#endif