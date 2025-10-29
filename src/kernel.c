#include <stdio.h>

extern int KERNEL_SIZE;
extern int g_volume_width;
extern int g_width_in_cubes;
extern int g_cube_width;

void rtm_kernel(void *descr[], void *cl_args){
    // do stuff here

}

/*
void average_filter(void *descr[], void *cl_args){

    struct cl_args* args = (struct cl_args*) cl_args;
    const uint32_t i = args->i;
    const uint32_t j = args->j;
    const uint32_t k = args->k;

    double *const block_w = (double*) STARPU_BLOCK_GET_PTR( descr[0] );
    double *const block_r = (double*) STARPU_BLOCK_GET_PTR( descr[1] );

    double *const z_minus = (double*) STARPU_BLOCK_GET_PTR( descr[2] );
    double *const z_plus = (double*) STARPU_BLOCK_GET_PTR( descr[3] );

    double *const y_minus = (double*) STARPU_BLOCK_GET_PTR( descr[4] );
    double *const y_plus = (double*) STARPU_BLOCK_GET_PTR( descr[5] );

    double *const x_minus = (double*) STARPU_BLOCK_GET_PTR( descr[6] );
    double *const x_plus = (double*) STARPU_BLOCK_GET_PTR( descr[7] );

    for(int z = 0; z < g_cube_width; z++){
        for(int y = 0; y < g_cube_width; y++){
            for(int x = 0; x < g_cube_width; x++){
                // need to set the border to 0, because the cube might contain tash data
                if( INEDGE(k, z) || INEDGE(j, y) || INEDGE(i, x)){
                    block_w[CUBE_I(x,y,z)] = 0.0;
                }else {
                    double res = 0.0;
                    for(int k = -KERNEL_SIZE; k <= KERNEL_SIZE; k++){ 
                        const int cell_idx = z + k; 
                        
                        if(cell_idx < 0){ 
                            res += z_minus[CUBE_I(x, y, g_cube_width + cell_idx)]; 
                        }else if(cell_idx >= g_cube_width){ 
                            res += z_plus[CUBE_I(x, y, cell_idx - g_cube_width)]; 
                        }else{ 
                            res += block_r[CUBE_I(x, y, cell_idx)]; 
                        } 
                    }

                    for(int k = -KERNEL_SIZE; k <= KERNEL_SIZE; k++){ 
                        const int cell_idx = y + k; 
                        if(cell_idx < 0){ 
                            res += y_minus[CUBE_I(x, g_cube_width + cell_idx, z)]; 
                        }else if(cell_idx >= g_cube_width){ 
                            res += y_plus[CUBE_I(x, cell_idx - g_cube_width, z)]; 
                        }else{ 
                            res += block_r[CUBE_I(x, cell_idx, z)]; 
                        } 
                    }

                    for(int k = -KERNEL_SIZE; k <= KERNEL_SIZE; k++){ 
                        const int cell_idx = x + k; 
                        if(cell_idx < 0){ 
                            res += x_minus[CUBE_I(g_cube_width + cell_idx, y, z)]; 
                        }else if(cell_idx >= g_cube_width){ 
                            res += x_plus[CUBE_I(cell_idx - g_cube_width, y, z)]; 
                        }else{ 
                            res += block_r[CUBE_I(cell_idx, y, z)]; 
                        } 
                    }
                        
                    block_w[CUBE_I(x, y, z)] = (res - (2 * block_r[CUBE_I(x, y, z)])) / 25.0;
                }
            }
        }
    }
    //assert_block_clear_edge(block_w, i, j, k);
}

*/