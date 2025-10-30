#ifndef PRECISION_H
#define PRECISION_H

#include <math.h>

// Choose precision type with -DFP_FLOAT or -DFP_LONG_DOUBLE (default = double)

// --- Type selection ---
#if defined(FP_FLOAT)
    typedef float FP;
    #define FP_SQRT  sqrtf
    #define FP_EXP   expf
    #define FP_SIN   sinf
    #define FP_COS   cosf
    #define FP_TAN   tanf
    #define FP_LOG   logf
    #define FP_POW   powf
    #define FP_LIT(x) x##f

#elif defined(FP_LONG_DOUBLE)
    typedef long double FP;
    #define FP_SQRT  sqrtl
    #define FP_EXP   expl
    #define FP_SIN   sinl
    #define FP_COS   cosl
    #define FP_TAN   tanl
    #define FP_LOG   logl
    #define FP_POW   powl
    #define FP_LIT(x) x##L

#else
    // Default: double precision
    typedef double FP;
    #define FP_SQRT  sqrt
    #define FP_EXP   exp
    #define FP_SIN   sin
    #define FP_COS   cos
    #define FP_TAN   tan
    #define FP_LOG   log
    #define FP_POW   pow
    #define FP_LIT(x) x
#endif

#endif