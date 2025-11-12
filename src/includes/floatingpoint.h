#ifndef PRECISION_H
#define PRECISION_H

#include <math.h>
#include <stdlib.h>

// Choose precision type with -DFP_FLOAT or -DFP_LONG_DOUBLE (default = double)

// --- Type selection ---
#if defined(FP_FLOAT)
    typedef float FP;
    #define FP_SQRT  sqrtf
    #define FP_EXP   expf
    #define FP_SIN   sinf
    #define FP_COS   cosf
    #define FP_TAN   tanf
    #define FP_ATAN  atanf
    #define FP_LOG   logf
    #define FP_POW   powf
    #define FP_CEIL  ceilf
    #define FP_MIN   fminf
    #define FP_MAX   fmaxf
    #define FP_ABS   fabsf
    #define FP_ARG   ARG_f32
    #define FP_RAND()  ((float) rand() / (float) RAND_MAX)
    #define FP_LIT(x) x##f

#elif defined(FP_LONG_DOUBLE)
    typedef long double FP;
    #define FP_SQRT  sqrtl
    #define FP_EXP   expl
    #define FP_SIN   sinl
    #define FP_COS   cosl
    #define FP_TAN   tanl
    #define FP_ATAN  atanl
    #define FP_LOG   logl
    #define FP_POW   powl
    #define FP_CEIL  ceill
    #define FP_MIN   fminl
    #define FP_MAX   fmaxl
    #define FP_ABS   fabsl
    #define FP_ARG   error
    #define FP_RAND  error
    #define FP_LIT(x) x##L

#else
    // Default: double precision
    typedef double FP;
    #define FP_SQRT  sqrt
    #define FP_EXP   exp
    #define FP_SIN   sin
    #define FP_COS   cos
    #define FP_TAN   tan
    #define FP_ATAN  atan
    #define FP_LOG   log
    #define FP_POW   pow
    #define FP_CEIL  ceil
    #define FP_MIN   fmin
    #define FP_MAX   fmax
    #define FP_ABS   fabs
    #define FP_ARG   ARG_f64
    // essa implementação de rand para double é meio ruim, melhorar TODO:
    #define FP_RAND()  ((double) rand() / (double) RAND_MAX)
    #define FP_LIT(x) x
#endif

#endif