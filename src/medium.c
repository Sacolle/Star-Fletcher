#include "medium.h"

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "argparse.h"
#include "macros.h"

//TODO: define these
#define SIGMA 1.0
#define MAX_SIGMA 10.0
#define MI 1.0

int str_to_medium(const char *str){
    return str_to_enum(str, 3, "ISO", ISO, "VTI", VTI, "TTI", TTI);
}

// TODO: test
// stability condition
FP medium_stability_condition(
    FP dx, FP dy, FP dz,
    FP *restrict vpz, FP *restrict epsilon, size_t size
){
    FP maxvel = vpz[0] * FP_SQRT(1.0 + 2 * epsilon[0]);
    for (size_t i = 1; i < size; i++){
        maxvel = FP_MAX(maxvel, vpz[i] * FP_SQRT(1.0 + 2 * epsilon[i]));
    }
    const FP mindelta = FP_MIN(FP_MIN(dx, dy), dz);

    const FP recdt = (MI * mindelta) / maxvel;
    return recdt;
}


// TODO: test
void medium_initialize(
    enum Form medium, size_t size,
    FP *restrict vpz, FP *restrict vsv, FP *restrict epsilon,
    FP *restrict delta, FP *restrict phi, FP *restrict theta
){
    switch (medium) 
    {
    case ISO:
    {
        for (size_t i = 0; i < size; i++){
            vpz[i]     = FP_LIT(3000.0);
            epsilon[i] = FP_LIT(0.0);
            delta[i]   = FP_LIT(0.0);
            phi[i]     = FP_LIT(0.0);
            theta[i]   = FP_LIT(0.0);
            vsv[i]     = FP_LIT(0.0);
        }
    }
    break;
    case VTI: 
    {
        if (SIGMA > MAX_SIGMA){
            printf("Since sigma (%f) is greater that threshold (%f), sigma is considered infinity and vsv is set to zero\n",
                SIGMA, MAX_SIGMA);
        }
        for (size_t i = 0; i < size; i++){
            vpz[i]     = FP_LIT(3000.0);
            epsilon[i] = FP_LIT(0.24);
            delta[i]   = FP_LIT(0.1);
            phi[i]     = FP_LIT(0.0);
            theta[i]   = FP_LIT(0.0);
            if (SIGMA > MAX_SIGMA){
                vsv[i] = FP_LIT(0.0);
            }else{
                vsv[i] = vpz[i] * FP_SQRT(FP_ABS(epsilon[i] - delta[i]) / SIGMA);
            }
        }
    }
    break;
    case TTI:
    {
        if (SIGMA > MAX_SIGMA)
        {
            printf("Since sigma (%f) is greater that threshold (%f), sigma is considered infinity and vsv is set to zero\n",
                SIGMA, MAX_SIGMA);
        }
        for (size_t i = 0; i < size; i++){
        {
            vpz[i]     = FP_LIT(3000.0);
            epsilon[i] = FP_LIT(0.24);
            delta[i]   = FP_LIT(0.1);
            phi[i]     = FP_LIT(1.0); // evitando coeficientes nulos
            theta[i]   = FP_ATAN(1.0);
            if (SIGMA > MAX_SIGMA){
                vsv[i] = FP_LIT(0.0);
            }else{
                vsv[i] = vpz[i] * FP_SQRT(FP_ABS(epsilon[i] - delta[i]) / SIGMA);
            }
        }
    }
    break;
    default: assert(0 && "Unreachable!");
    }
    }
}
//check if value is in the interval (inclusive)[lb, ub]
static inline bool inbounds(size_t val, size_t lb, size_t ub){
    return val >= lb && val <= ub;
}

void medium_random_velocity_boundary(
    uint32_t nx, uint32_t ny, uint32_t nz, 
    size_t border_width, size_t absorb_width,
    FP *restrict vpz, FP *restrict vsv
){
    //can mark as const?
    extern const size_t g_volume_width;
    const size_t outer_width = border_width + absorb_width;
    FP max_speed_p = FP_LIT(0.0), max_speed_s = FP_LIT(0.0);

    const size_t inside_start = outer_width;
    const size_t inside_end = g_volume_width - outer_width;
    for(size_t z = inside_start; z < inside_end; z++){
        for(size_t y = inside_start; y < inside_end; y++){
            for(size_t x = inside_start; x < inside_end; x++){
                const size_t i = volume_idx(x, y, z);

                max_speed_p = FP_MAX(vpz[i], max_speed_p);
                max_speed_s = FP_MAX(vsv[i], max_speed_s);
            }
        }
    }

    for(size_t z = 0; z < g_volume_width; z++){
        for(size_t y = 0; y < g_volume_width; y++){
            for(size_t x = 0; x < g_volume_width; x++){

                const size_t i = volume_idx(x, y, z);

                // do nothing inside input grid
                if(inbounds(z, inside_start, inside_end) &&
                    inbounds(y, inside_start, inside_end) && 
                    inbounds(x, inside_start, inside_end)){
                    continue;
                }

                // null speed at border
                // which is a point not within the bounds defided by the aborsb - inner - absobr
                if(!(inbounds(z, border_width, g_volume_width - border_width) &&
                    inbounds(y, border_width, g_volume_width - border_width) &&
                    inbounds(x, border_width, g_volume_width - border_width))
                ){
                    vpz[i] = FP_LIT(0.0);
                    vsv[i] = FP_LIT(0.0);
                }

                size_t ivelz = z, ively = y, ivelx = x;
                FP distz = FP_LIT(0.0), disty = FP_LIT(0.0), distx = FP_LIT(0.0);

                #define BOUNDS_CALC(var) do {\
                    if ((var) >= inside_end){   \
                        dist##var = (FP)(var - inside_end - 1); \
                        ivel##var = inside_end - 1; \
                    }else if ((var) < inside_start){ \
                        dist##var = (FP)(inside_start - var); \
                        ivel##var = inside_start; \
                    }} while(0)
                BOUNDS_CALC(z);
                BOUNDS_CALC(y);
                BOUNDS_CALC(x);

                // random speed inside absortion zone
                const FP dist = FP_MAX(distz, FP_MAX(disty, distx));
                const FP frac = FP_LIT(1.0)/((FP) absorb_width);

                const FP border_distance = dist * frac;
                const FP random_factor = FP_RAND();

                vpz[i] = vpz[ind(ivelx, ively, ivelz)] * (1.0 - border_distance) +
                            max_speed_p * random_factor * border_distance;
                vsv[i] = vsv[ind(ivelx, ively, ivelz)] * (1.0 - border_distance) +
                            max_speed_s * random_factor * border_distance;
            }
        }
    }
}