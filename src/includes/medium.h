#ifndef _MEDIUM_GUARD_
#define _MEDIUM_GUARD_

#include <stddef.h>

#include "floatingpoint.h"

//TODO: define these properly
#define SIGMA 1.0
#define MAX_SIGMA 10.0
#define MI 1.0

enum Form { ISO, VTI, TTI };

int str_to_medium(const char* str);

//int medium_initialize();

FP medium_stability_condition(
    const FP dx, const FP dy, const FP dz,
    FP *restrict vpz, FP *restrict epsilon, size_t size
);

void medium_initialize(
    const enum Form medium, const size_t size,
    FP *restrict vpz, FP *restrict vsv, FP *restrict epsilon,
    FP *restrict delta, FP *restrict phi, FP *restrict theta
);

void medium_random_velocity_boundary(
    const size_t border_width, const size_t absorb_width,
    FP *restrict vpz, FP *restrict vsv
);

#endif