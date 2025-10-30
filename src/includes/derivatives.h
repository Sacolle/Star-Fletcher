#ifndef _DERIVATIVE_GUARD
#define _DERIVATIVE_GUARD

#include <stdint.h>

#include "floatingpoint.h"
#include "macros.h"

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


#define KERNEL_SIZE 4

extern uint64_t g_cube_width;

//flip along each respective axis, so in a
// 0, 1, 2
// 3, 4, 5
// 6, 7, 8
// flip_x(5) returns 3
static inline size_t flip_x(size_t idx){
    return g_cube_width - 1 - idx;
}

static inline size_t flip_y(size_t idx){
    return SQUARE(g_cube_width) - 1 - idx;
}

static inline size_t flip_z(size_t idx){
    return CUBE(g_cube_width) - 1 - idx;
}

//NOTE: this might do some insane overflow errors
/*
    #define Der1(p, i, s, dinv) (
    L1 * (p[i + s] - p[i - s]) + 
    L2 * (p[i + 2 * s] - p[i - 2 * s]) + 
    L3 * (p[i + 3 * s] - p[i - 3 * s]) + 
    L4 * (p[i + 4 * s] - p[i - 4 * s])
    ) * (dinv)
*/
#define DECL_DIRECTIONS_FST_DERIV(dir) \
static inline FP fst_deriv_d##dir(FP* block, FP* block_minus, FP* block_plus, \
    const size_t x, const size_t y, const size_t z, \
    const size_t stride, const FP dinv \
){ \
    const size_t base_idx = cube_idx(x,y,z); \
    const FP ls[4] = {L1, L2, L3, L4}; \
    FP aggr = 0.0; \
    size_t idx = base_idx; \
    for(size_t k = 1; k <= KERNEL_SIZE; k++){ \
        const size_t dim_idx = dir + k; \
        /*if it overflows the cube in this dimension*/ \
        if(dim_idx >= g_cube_width){ \
            /* star in the same point on the next cube */ \
            /* this is equivalent to fliping the idx over the axis */ \
            idx = flip_##dir(idx); \
            aggr += ls[k] * block_plus[idx]; \
        }else{ \
            idx += stride * k; \
            aggr += ls[k] * block[idx]; \
        } \
    } \
    idx = base_idx; \
    for(int k = 1; k <= KERNEL_SIZE; k++){ \
        const int dim_idx = ((int)dir) - k; \
        if(dim_idx < 0){ \
            idx = flip_##dir(idx); \
            aggr -= ls[k] * block_minus[idx]; \
        }else{ \
            idx -= stride * k; \
            aggr -= ls[k] * block[idx]; \
        } \
    } \
    return aggr * dinv; \
}

DECL_DIRECTIONS_FST_DERIV(x)
DECL_DIRECTIONS_FST_DERIV(y)
DECL_DIRECTIONS_FST_DERIV(z)
/*
    #define Der2(p, i, s, d2inv) ((
    K0 * p[i] + 
    K1 * (p[i + s] + p[i - s]) + 
    K2 * (p[i + 2 * s] + p[i - 2 * s]) + 
    K3 * (p[i + 3 * s] + p[i - 3 * s]) + 
    K4 * (p[i + 4 * s] + p[i - 4 * s])
    ) * (d2inv))

*/
static inline FP second_derivative(){

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
static inline FP cross_derivative(){

}

#endif