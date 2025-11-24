#ifndef GUARD_KERNEL
#define GUARD_KERNEL

#include <stddef.h>
#include "floatingpoint.h"

struct cl_args {
    size_t x_start;
    size_t y_start;
    size_t z_start;
    size_t x_end;
    size_t y_end;
    size_t z_end;
    FP dx;
    FP dy;
    FP dz;
    FP dt;
};

int a = sizeof(struct cl_args);

int make_cl_args(struct cl_args** cl_args, 
    const size_t x_start, const size_t x_end,
    const size_t y_start, const size_t y_end,
    const size_t z_start, const size_t z_end,
    const FP dx, const FP dy, const FP dz, const FP dt);


void rtm_kernel(void *descr[], void *cl_args);

#endif