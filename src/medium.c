#include "medium.h"

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <starpu.h>

#include "argparse.h"
#include "macros.h"

//return != 0 if err
err_t str_to_medium(const char *str, enum Form* form){
    err_t err;
    int ret;
    if((err = str_to_enum(str, &ret, 3, "ISO", ISO, "VTI", VTI, "TTI", TTI)) != 0){
        return err;
    }
    *form = ret;
    return 0;
}

// TODO: test
// stability condition
FP medium_stability_condition(
    const FP dx, const FP dy, const FP dz,
    FP *restrict vpz, FP *restrict epsilon, const size_t size
){
    FP maxvel = vpz[0] * FP_SQRT(FP_LIT(1.0) + FP_LIT(2.0) * epsilon[0]);
    for (size_t i = 1; i < size; i++){
        maxvel = FP_MAX(maxvel, vpz[i] * FP_SQRT(FP_LIT(1.0) + FP_LIT(2.0) * epsilon[i]));
    }
    const FP mindelta = FP_MIN(FP_MIN(dx, dy), dz);

    const FP recdt = (MI * mindelta) / maxvel;
    return recdt;
}


void medium_initialize(
    const enum Form medium, const size_t size,
    FP *restrict vpz, FP *restrict vsv, FP *restrict epsilon,
    FP *restrict delta, FP *restrict phi, FP *restrict theta
){
    switch (medium) 
    {
    case ISO:
    {
	#pragma omp parallel for
        for (size_t i = 0; i < size; i++){
            vpz[i]     = 3000.0;
            epsilon[i] = 0.0;
            delta[i]   = 0.0;
            phi[i]     = 0.0;
            theta[i]   = 0.0;
            vsv[i]     = 0.0;
        }
    }
    break;
    case VTI: 
    {
        if (SIGMA > MAX_SIGMA){
            //printf("Since sigma (%f) is greater that threshold (%f), sigma is considered infinity and vsv is set to zero\n", SIGMA, MAX_SIGMA);
        }
	#pragma omp parallel for
        for (size_t i = 0; i < size; i++){
            vpz[i]     = 3000.0;
            epsilon[i] = 0.24;
            delta[i]   = 0.1;
            phi[i]     = 0.0;
            theta[i]   = 0.0;
            if (SIGMA > MAX_SIGMA){
                vsv[i] = 0.0;
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
	#pragma omp parallel for
        for (size_t i = 0; i < size; i++){
            vpz[i]     = 3000.0;
            epsilon[i] = 0.24;
            delta[i]   = 0.1;
            phi[i]     = 1.0; // evitando coeficientes nulos
            theta[i]   = FP_ATAN(1.0);
            if (SIGMA > MAX_SIGMA){
                vsv[i] = 0.0;
            }else{
                vsv[i] = vpz[i] * FP_SQRT(FP_ABS(epsilon[i] - delta[i]) / SIGMA);
            }
        }
    }
    break;
    default: assert(0 && "Unreachable!");
    }
}
//check if value is in the interval (inclusive)[lb, ub]
static inline bool inbounds(size_t val, size_t lb, size_t ub){
    return val >= lb && val <= ub;
}

FP max_speed_in_cube_segment(FP *v, const size_t start_idx, const size_t end_idx){
    FP max_speed = FP_LIT(0.0);

    #pragma omp parallel for
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

    #pragma omp parallel for
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

typedef struct intermed_values_args {
  size_t i, j, k;
} intermed_values_args_t;

void medium_calc_intermediary_values_task(void *descr[], void *cl_args) {
    extern size_t g_cube_width;
  
    const intermed_values_args_t* args = (intermed_values_args_t*) cl_args;
    const size_t i = args->i, j = args->j, k = args->k;

    const FP* vpz = (FP*) STARPU_BLOCK_GET_PTR(descr[0]);
    const FP* vsv = (FP*) STARPU_BLOCK_GET_PTR(descr[1]);
    const FP* epsilon = (FP*) STARPU_BLOCK_GET_PTR(descr[2]);

    const FP* delta = (FP*) STARPU_BLOCK_GET_PTR(descr[3]);
    const FP* phi = (FP*) STARPU_BLOCK_GET_PTR(descr[4]);
    const FP* theta = (FP*) STARPU_BLOCK_GET_PTR(descr[5]);


    FP *const ch1dxx = (FP*) STARPU_BLOCK_GET_PTR(descr[6]);
    FP *const ch1dyy = (FP*) STARPU_BLOCK_GET_PTR(descr[7]);
    FP *const ch1dzz = (FP*) STARPU_BLOCK_GET_PTR(descr[8]);
    FP *const ch1dxy = (FP*) STARPU_BLOCK_GET_PTR(descr[9]);
    FP *const ch1dyz = (FP*) STARPU_BLOCK_GET_PTR(descr[10]);
    FP *const ch1dxz = (FP*) STARPU_BLOCK_GET_PTR(descr[11]);
    FP *const v2px = (FP*) STARPU_BLOCK_GET_PTR(descr[12]);
    FP *const v2pz = (FP*) STARPU_BLOCK_GET_PTR(descr[13]);
    FP *const v2sz = (FP*) STARPU_BLOCK_GET_PTR(descr[14]);
    FP *const v2pn = (FP*) STARPU_BLOCK_GET_PTR(descr[15]);


    for(size_t z = 0; z < g_cube_width; z++){
	for(size_t y = 0; y < g_cube_width; y++){
	    for(size_t x = 0; x < g_cube_width; x++){
		const size_t c_i = cube_idx(x, y, z);
		const size_t vol_i = volume_idx(x + i * g_cube_width, y + j * g_cube_width, z + k * g_cube_width);

		// changing the data type TO be the same as in the original
		const FP sinTheta = sin(theta[vol_i]);
		const FP cosTheta = cos(theta[vol_i]);
		const FP sin2Theta = sin(2.0 * theta[vol_i]);
		const FP sinPhi = sin(phi[vol_i]);
		const FP cosPhi = cos(phi[vol_i]);
		const FP sin2Phi = sin(2.0 * phi[vol_i]);

		ch1dxx[c_i] = sinTheta * sinTheta * cosPhi * cosPhi;
		ch1dyy[c_i] = sinTheta * sinTheta * sinPhi * sinPhi;
		ch1dzz[c_i] = cosTheta * cosTheta;
		ch1dxy[c_i] = sinTheta * sinTheta * sin2Phi;
		ch1dyz[c_i] = sin2Theta * sinPhi;
		ch1dxz[c_i] = sin2Theta * cosPhi;

		// coeficients of H1 and H2 at PDEs
		const FP l_v2pz = vpz[vol_i] * vpz[vol_i];
		v2sz[c_i] = vsv[vol_i] * vsv[vol_i];
		v2pz[c_i] = l_v2pz;
		v2px[c_i] = l_v2pz * (1.0 + 2.0 * epsilon[vol_i]);
		v2pn[c_i] = l_v2pz * (1.0 + 2.0 * delta[vol_i]);
	    }
	}
    }
}


err_t make_intermed_values_args(intermed_values_args_t **args, const size_t i,
                                const size_t j, const size_t k) {
  
    if((*args = (intermed_values_args_t*) malloc(sizeof(intermed_values_args_t))) == NULL){
	return errno;
    }
    **args = (intermed_values_args_t) {i, j, k};

    return 0;
}

struct starpu_codelet intermed_values_codelet = {
    .cpu_funcs = { medium_calc_intermediary_values_task },
    .nbuffers = 16,
    .modes = {
      STARPU_R,
      STARPU_R,
      STARPU_R,
      STARPU_R,
      STARPU_R,
      STARPU_R,
      STARPU_W,
      STARPU_W,
      STARPU_W,
      STARPU_W,
      STARPU_W,
      STARPU_W,
      STARPU_W,
      STARPU_W,
      STARPU_W,
      STARPU_W,
    },
    .model = &starpu_perfmodel_nop
};


err_t medium_calc_intermediary_values(
    const FP* vpz, const FP* vsv, const FP* epsilon,
    const FP* delta, const FP* phi, const FP* theta,
    starpu_data_handle_t* hdl_ch1dxx, starpu_data_handle_t* hdl_ch1dyy, starpu_data_handle_t* hdl_ch1dzz, 
    starpu_data_handle_t* hdl_ch1dxy, starpu_data_handle_t* hdl_ch1dyz, starpu_data_handle_t* hdl_ch1dxz, 
    starpu_data_handle_t* hdl_v2px, starpu_data_handle_t* hdl_v2pz, starpu_data_handle_t* hdl_v2sz, starpu_data_handle_t* hdl_v2pn
){
    extern size_t g_width_in_cubes;
    extern size_t g_cube_width;
    extern size_t g_volume_width;

    err_t err;

    starpu_data_handle_t vpz_handle, vsv_handle, epsilon_handle, delta_handle, phi_handle, theta_handle;
    // TODO: register the handles

    #define BLOCK_REGISTER(handle, ptr) starpu_block_data_register((handle), STARPU_MAIN_RAM, (uintptr_t) (ptr), \
        g_volume_width, SQUARE(g_volume_width), g_volume_width, g_volume_width, g_volume_width, sizeof(FP))

    BLOCK_REGISTER(&vpz_handle, vpz);
    BLOCK_REGISTER(&vsv_handle, vsv);
    BLOCK_REGISTER(&epsilon_handle, epsilon);
    BLOCK_REGISTER(&delta_handle, delta);
    BLOCK_REGISTER(&phi_handle, phi);
    BLOCK_REGISTER(&theta_handle, theta);
    

    for(size_t k = 0; k < g_width_in_cubes; k++){
        for(size_t j = 0; j < g_width_in_cubes; j++){
            for(size_t i = 0; i < g_width_in_cubes; i++){
                //index the block
                const size_t b_i = block_idx(i, j , k);

		intermed_values_args_t* args;
		if((err = make_intermed_values_args(&args, i, j, k)) != 0){
		    return err;
		}

		struct starpu_task* intermed_values_task = starpu_task_create();
		intermed_values_task->name = "write_block";
		intermed_values_task->cl = &intermed_values_codelet;
		intermed_values_task->cl_arg = args;
		intermed_values_task->cl_arg_size = sizeof(intermed_values_args_t);
		intermed_values_task->cl_arg_free = 1;

		intermed_values_task->handles[0]  = vpz_handle;
		intermed_values_task->handles[1]  = vsv_handle;
		intermed_values_task->handles[2]  = epsilon_handle;
		intermed_values_task->handles[3]  = delta_handle;
		intermed_values_task->handles[4]  = phi_handle;
		intermed_values_task->handles[5]  = theta_handle;

		intermed_values_task->handles[6]  = hdl_ch1dxx[b_i];
		intermed_values_task->handles[7]  = hdl_ch1dyy[b_i];
		intermed_values_task->handles[8]  = hdl_ch1dzz[b_i];
		intermed_values_task->handles[9]  = hdl_ch1dxy[b_i];
		intermed_values_task->handles[10] = hdl_ch1dyz[b_i];
		intermed_values_task->handles[11] = hdl_ch1dxz[b_i];
		intermed_values_task->handles[12] = hdl_v2sz[b_i];
		intermed_values_task->handles[13] = hdl_v2pz[b_i];
		intermed_values_task->handles[14] = hdl_v2px[b_i];
		intermed_values_task->handles[15] = hdl_v2pn[b_i];

		if((err = starpu_task_submit(intermed_values_task)) != 0){
		    return err;
		};
            }
        }
    }

    starpu_task_wait_for_all();
    // unregister the handles
    starpu_data_unregister(vpz_handle);
    starpu_data_unregister(vsv_handle);
    starpu_data_unregister(epsilon_handle);
    starpu_data_unregister(delta_handle);
    starpu_data_unregister(phi_handle);
    starpu_data_unregister(theta_handle);
    
    return 0;
}

/*
#define FCUT        FP_LIT(40.0)
#define PICUBE      FP_LIT(31.00627668029982017537)
#define TWOSQRTPI   FP_LIT(3.54490770181103205458)
#define THREESQRTPI FP_LIT(5.31736155271654808184)
*/

// omissão do FP_LIT para obter os mesmos resultados do que o fletcher base
#define FCUT        40.0
#define PICUBE      31.00627668029982017537
#define TWOSQRTPI   3.54490770181103205458
#define THREESQRTPI 5.31736155271654808184

//#include <stdio.h>

FP medium_source_value(const FP dt, const int64_t it){
    const FP tf = TWOSQRTPI / FCUT;
    const FP fc = FCUT / THREESQRTPI;
    const FP fct = fc * (((FP) it) * dt - tf);
    const FP expo = PICUBE * fct * fct;
    const FP result = ((FP_LIT(1.0) - FP_LIT(2.0) * expo) * FP_EXP(-expo));

    //printf("source: %lf\n", result);
    return result;
}
