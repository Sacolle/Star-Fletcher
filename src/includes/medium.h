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
// calculates the stability condition of the time step
FP medium_stability_condition(
    const FP dx, const FP dy, const FP dz,
    FP *restrict vpz, FP *restrict epsilon, size_t size
);

// initialize the mediums with values based on the medium form
void medium_initialize(
    const enum Form medium, const size_t size,
    FP *restrict vpz, FP *restrict vsv, FP *restrict epsilon,
    FP *restrict delta, FP *restrict phi, FP *restrict theta
);

// sets up a random value for the boundry zone of the medium
void medium_random_velocity_boundary(
    const size_t border_width, const size_t absorb_width,
    FP *restrict vpz, FP *restrict vsv
);

// calculates intermediary values for future computations
void medium_calc_intermediary_values(
    const FP* vpz, const FP* vsv, const FP* epsilon,
    const FP* delta, const FP* phi, const FP* theta,
    FP **restrict ch1dxx, FP **restrict ch1dyy, FP **restrict ch1dzz, 
    FP **restrict ch1dxy, FP **restrict ch1dyz, FP **restrict ch1dxz, 
    FP **restrict v2px, FP **restrict v2pz, FP **restrict v2sz, FP **restrict v2pn
);

#endif