#include "medium.h"

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "argparse.h"
#include "macros.h"


int str_to_medium(const char *str){
    return str_to_enum(str, 3, "ISO", ISO, "VTI", VTI, "TTI", TTI);
}

// TODO: test
// stability condition
FP medium_stability_condition(
    const FP dx, const FP dy, const FP dz,
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
    const enum Form medium, const size_t size,
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
            //printf("Since sigma (%f) is greater that threshold (%f), sigma is considered infinity and vsv is set to zero\n", SIGMA, MAX_SIGMA);
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
            // printf("Since sigma (%f) is greater that threshold (%f), sigma is considered infinity and vsv is set to zero\n", SIGMA, MAX_SIGMA);
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

FP max_speed_in_cube_segment(FP *v, const size_t start_idx, const size_t end_idx){
    FP max_speed = FP_LIT(0.0);
    for(size_t z = start_idx; z < end_idx; z++){
        for(size_t y = start_idx; y < end_idx; y++){
            for(size_t x = start_idx; x < end_idx; x++){
                const size_t i = volume_idx(x, y, z);
                max_speed = FP_MAX(v[i], max_speed);
            }
        }
    }
    return max_speed;
}

void medium_random_velocity_boundary(
    const size_t border_width, const size_t absorb_width,
    FP *restrict vpz, FP *restrict vsv
){
    extern size_t g_volume_width;
    const size_t outer_width = border_width + absorb_width;

    const size_t inside_start = outer_width;
    const size_t inside_end = g_volume_width - outer_width;

    const FP max_speed_p = max_speed_in_cube_segment(vpz, inside_start, inside_end);
    const FP max_speed_s = max_speed_in_cube_segment(vsv, inside_start, inside_end);

    for(size_t z = 0; z < g_volume_width; z++){
        for(size_t y = 0; y < g_volume_width; y++){
            for(size_t x = 0; x < g_volume_width; x++){

                const size_t i = volume_idx(x, y, z);

                // do nothing inside input grid
                if(inbounds(z, inside_start, inside_end - 1) &&
                   inbounds(y, inside_start, inside_end - 1) && 
                   inbounds(x, inside_start, inside_end - 1)){
                    continue;
                }

                // null speed at border
                // which is a point not within the bounds defided by the aborsb - inner - absobr
                if(!(inbounds(z, border_width, g_volume_width - border_width - 1) &&
                     inbounds(y, border_width, g_volume_width - border_width - 1) &&
                     inbounds(x, border_width, g_volume_width - border_width - 1))
                ){
                    vpz[i] = FP_LIT(0.0);
                    vsv[i] = FP_LIT(0.0);
                    continue;
                }


                // find the greates distance from the border for each direction, 
                // if the direction is inside it's absobsion zone, also save the index of the closest
                // inner point to this absorpsion zone
                #define DIR_AMOUNT 3
                size_t ivel[DIR_AMOUNT] = {x, y, z};
                FP dist = FP_LIT(0.0);
                for(size_t dir_i = 0; dir_i < DIR_AMOUNT; dir_i++){
                    const size_t dir = ivel[dir_i];
                    if (dir >= inside_end){ 
                        dist = FP_MAX(dist, (FP)(dir - inside_end + 1 )); 
                        ivel[dir_i] = inside_end - 1; 
                    }else if (dir < inside_start){ 
                        dist = FP_MAX(dist, (FP)(inside_start - dir)); 
                        ivel[dir_i] = inside_start; 
                    }
                }

                // random speed inside absortion zone
                const size_t wave_idx = volume_idx(ivel[0], ivel[1], ivel[2]);
                const FP frac = FP_LIT(1.0)/((FP) absorb_width);
                const FP border_distance = dist * frac;
                const FP random_factor = FP_RAND();

                vpz[i] = vpz[wave_idx] * (1.0 - border_distance) +
                            max_speed_p * random_factor * border_distance;
                vsv[i] = vsv[wave_idx] * (1.0 - border_distance) +
                            max_speed_s * random_factor * border_distance;
                /*
                vpz[i] = border_distance;
                vsv[i] = border_distance;
                */
            }
        }
    }
}