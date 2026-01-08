#ifndef GUARD_KERNEL
#define GUARD_KERNEL

#include <stddef.h>
#include "floatingpoint.h"

typedef struct rtm_args {
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
} rtm_args_t;

int make_rtm_args(rtm_args_t** rtm_args, 
    const size_t x_start, const size_t x_end,
    const size_t y_start, const size_t y_end,
    const size_t z_start, const size_t z_end,
    const FP dx, const FP dy, const FP dz, const FP dt);

typedef struct perturb_args{
    size_t source_idx;
    FP perturb_value;
    size_t t;
} perturb_args_t;

int make_perturb_args(perturb_args_t** perturb_args, const size_t idx, const FP value, const size_t t);


void rtm_kernel(void *descr[], void *cl_args);

void perturbation_kernel(void *descr[], void *cl_args);

#endif