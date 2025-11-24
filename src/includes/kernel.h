#ifndef GUARD_KERNEL
#define GUARD_KERNEL

#include <stddef.h>
#include "floatingpoint.h"

struct cl_args {
    //char name[64];
    size_t i;
    size_t j;
    size_t k;
    size_t t;
    FP dx;
    FP dy;
    FP dz;
    FP dt;
};

int make_cl_args(struct cl_args** cl_args, 
    const size_t i, const size_t j, const size_t k, const size_t t, 
    const FP dx, const FP dy, const FP dz, const FP dt);


void rtm_kernel(void *descr[], void *cl_args);

#endif