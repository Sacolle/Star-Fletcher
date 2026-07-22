#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include<time.h>
#include <cuda_runtime.h>

#include "macros.h"
#include "floatingpoint.h"
#include "err.h"
#include "vector.h"


size_t g_volume_width = 0;
size_t g_width_in_cubes = 0;

size_t g_cube_width = 0;

const size_t BORDER_WIDTH = 4;

#define CUBE_SIZE (g_cube_width * g_cube_width * g_cube_width)
#define TOTAL_CUBES (g_width_in_cubes * g_width_in_cubes * g_width_in_cubes)

//#include <linux/time.h>
#define NS_PER_SECOND 1000000000ULL
#define SECONDS_PER_NS 1e-9

uint64_t get_timestamp_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ((uint64_t)ts.tv_sec * NS_PER_SECOND) + ts.tv_nsec;
}

double elapsed_seconds(const uint64_t t_end, const uint64_t t_start){
    return ((double)t_end - t_start) * SECONDS_PER_NS;
}


err_t g_err;
#define TRY(x,...) TRYTO(g_err = (x), program_status = EXIT_FAILURE; \
    printf("[error] Failed at line %d with error %ld: %s\n", __LINE__, g_err, err_name(g_err)); \
    printf("[err-msg] " __VA_ARGS__);, program_end)

#define DEBUG(...) printf("[debug] " __VA_ARGS__)

#ifndef RANDOM_SEED
#define RANDOM_SEED 0
#endif

#define CUDA_TRY(call) do{ \
   const cudaError_t err=call;\
   if (err != cudaSuccess){\
     fprintf(stderr, "CUDA ERROR: %s on %s:%d\n", cudaGetErrorString(err), __FILE__, __LINE__);\
     goto program_end; \
   }}while(0)


// extracted from mem.c and mem.h 
typedef vector(void*) mem_vec_t;

err_t mem_allocate(mem_vec_t* v, void** ptr, const size_t size){
  if((*ptr = (void*) malloc(size)) == NULL){
        return errno;
    }
    vector_push(*v, *ptr);
    return 0;
}

void mem_free(mem_vec_t* v){
    vector_free_all(*v, free);
}

cudaError_t cuda_mem_allocate(mem_vec_t* v, void** ptr, const size_t size){
  cudaError_t err;
  if((err = cudaMalloc(ptr, size)) != cudaSuccess){
        return err;
    }
    vector_push(*v, *ptr);
    return 0;
}

void cuda_mem_free(mem_vec_t* v){
    vector_free_all(*v, cudaFree);
}


err_t init_nested_buffer(FP** buff[], mem_vec_t* allocs, const size_t outter_count, const size_t inner_count){
  err_t err = 0;
  if((err = mem_allocate(allocs, (void**) buff, outter_count * sizeof(FP*))) != 0){
    return err;
  }
  FP** b = *buff;
  for (size_t i = 0; i < outter_count; i++) {
    if ((err = mem_allocate(allocs, (void **) &b[i],
                            inner_count * sizeof(FP))) != 0) {
      return err;
    }
    for (size_t j = 0; j < inner_count; j++) {
      b[i][j] = FP_RAND();
    }
  }
  return err;
}

cudaError_t cuda_copy_nested_buffer(FP** buff[], FP* ref[], mem_vec_t* allocs, mem_vec_t* cuda_allocs, const size_t outter_count, const size_t inner_count){
  if(mem_allocate(allocs, (void**) buff, outter_count * sizeof(FP*)) != 0){
    return cudaErrorMemoryAllocation;
  }

  cudaError_t err;
  FP** b = *buff;
  for (size_t i = 0; i < outter_count; i++) {
    if ((err = cuda_mem_allocate(cuda_allocs, (void **)(b + i),
                                 inner_count * sizeof(FP))) != cudaSuccess) {
      return err;
    }
    if((err = cudaMemcpy(b[i], ref[i], inner_count * sizeof(FP), cudaMemcpyHostToDevice)) != cudaSuccess){
      return err;
    }
  }
  return cudaSuccess;
}

extern void rtm_kernel_cuda(
    const size_t x_start, const size_t y_start, const size_t z_start,
    const size_t x_end, const size_t y_end, const size_t z_end,
    const FP dx, const FP dy, const FP dz, const FP dt, FP* descr[]
			    );

int main(int argc, char **argv){

    const uint64_t initialization_start_time = get_timestamp_ns();
    srand(RANDOM_SEED);

    //need to be toplevel for the try macro
    int program_status = EXIT_SUCCESS;
    mem_vec_t allocs = NULL;
    mem_vec_t cuda_allocs = NULL;
    uint32_t nx = 552, ny = 552, nz = 552, absorb_width = 8;
    FP dx, dy, dz, dt = 0.0001, tmax = 0.005;
    dx = dy = dz = FP_LIT(12.5);


    TRY(argc > 1 ? 0 : ME_COUNT_DONT_MATCH, "Passe a segmentação.");
    g_width_in_cubes = atoi(argv[1]);

    //fazendo dessa forma para ficar igual ao fletcher base
    g_volume_width = nx + 2 * absorb_width + 2 * BORDER_WIDTH;

    // the number of segments divides the total volume
    TRY(g_volume_width % g_width_in_cubes == 0 ? 0 : ME_COUNT_DONT_MATCH, 
        "A largura do volume + kernel size devem ser divisíveis pela largura do segmento.\n");

	g_cube_width = g_volume_width / g_width_in_cubes;
    
    TRY(g_cube_width > 7 ? 0 : ME_COUNT_DONT_MATCH, 
        "Tamanho interno para o cubo é muito pequeno."
        "As funções de derivada requerem um bloco cujo tamanho seja pelo menos 1 a menos que o tamanho do kernel, que é 9");
 
    const int64_t st = (int64_t) FP_CEIL(tmax / dt);

    // the point in the global volume that the source is inserted
    const size_t volume_propagation_idx = volume_idx(g_volume_width / 2, g_volume_width / 2, g_volume_width / 2);
    DEBUG("Index of propagation is %ld.\n", volume_propagation_idx);

    // INITIALIZE THE CUDA
    int deviceCount;
    CUDA_TRY(cudaGetDeviceCount(&deviceCount));
    const int device = 0;
    struct cudaDeviceProp deviceProp;
    CUDA_TRY(cudaGetDeviceProperties(&deviceProp, device));
    printf("CUDA source using device(%d) %s with compute capability %d.%d.\n",
           device, deviceProp.name, deviceProp.major, deviceProp.minor);
    CUDA_TRY(cudaSetDevice(device));
    

    FP **ch1dxx, **ch1dyy, **ch1dzz, **ch1dxy, **ch1dyz, **ch1dxz, **v2px, **v2pz, **v2sz, **v2pn;

    TRY(init_nested_buffer(&ch1dxx, &allocs, TOTAL_CUBES, CUBE_SIZE));
    TRY(init_nested_buffer(&ch1dyy, &allocs, TOTAL_CUBES, CUBE_SIZE));
    TRY(init_nested_buffer(&ch1dzz, &allocs, TOTAL_CUBES, CUBE_SIZE));
    TRY(init_nested_buffer(&ch1dxy, &allocs, TOTAL_CUBES, CUBE_SIZE));
    TRY(init_nested_buffer(&ch1dyz, &allocs, TOTAL_CUBES, CUBE_SIZE));
    TRY(init_nested_buffer(&ch1dxz, &allocs, TOTAL_CUBES, CUBE_SIZE));
    TRY(init_nested_buffer(&v2px, &allocs, TOTAL_CUBES, CUBE_SIZE));
    TRY(init_nested_buffer(&v2pz, &allocs, TOTAL_CUBES, CUBE_SIZE));
    TRY(init_nested_buffer(&v2sz, &allocs, TOTAL_CUBES, CUBE_SIZE));
    TRY(init_nested_buffer(&v2pn, &allocs, TOTAL_CUBES, CUBE_SIZE));

    FP **p_wave[3], **q_wave[3];

    TRY(init_nested_buffer(&p_wave[0], &allocs, CUBE(g_width_in_cubes + 1), CUBE_SIZE));
    TRY(init_nested_buffer(&p_wave[1], &allocs, CUBE(g_width_in_cubes + 1), CUBE_SIZE));
    TRY(init_nested_buffer(&p_wave[2], &allocs, CUBE(g_width_in_cubes + 1), CUBE_SIZE));
    TRY(init_nested_buffer(&q_wave[0], &allocs, CUBE(g_width_in_cubes + 1), CUBE_SIZE));
    TRY(init_nested_buffer(&q_wave[1], &allocs, CUBE(g_width_in_cubes + 1), CUBE_SIZE));
    TRY(init_nested_buffer(&q_wave[2], &allocs, CUBE(g_width_in_cubes + 1), CUBE_SIZE));


    
    // - Allocate all those mediums in the GPU

    FP **dev_ch1dxx, **dev_ch1dyy, **dev_ch1dzz, **dev_ch1dxy, **dev_ch1dyz, **dev_ch1dxz, **dev_v2px, **dev_v2pz, **dev_v2sz, **dev_v2pn;
    CUDA_TRY(cuda_copy_nested_buffer(&dev_ch1dxx, ch1dxx, &allocs, &cuda_allocs,  TOTAL_CUBES,  CUBE_SIZE));
    CUDA_TRY(cuda_copy_nested_buffer(&dev_ch1dyy, ch1dyy, &allocs, &cuda_allocs,  TOTAL_CUBES,  CUBE_SIZE));
    CUDA_TRY(cuda_copy_nested_buffer(&dev_ch1dzz, ch1dzz, &allocs, &cuda_allocs,  TOTAL_CUBES,  CUBE_SIZE));
    CUDA_TRY(cuda_copy_nested_buffer(&dev_ch1dxy, ch1dxy, &allocs, &cuda_allocs,  TOTAL_CUBES,  CUBE_SIZE));
    CUDA_TRY(cuda_copy_nested_buffer(&dev_ch1dyz, ch1dyz, &allocs, &cuda_allocs,  TOTAL_CUBES,  CUBE_SIZE));
    CUDA_TRY(cuda_copy_nested_buffer(&dev_ch1dxz, ch1dxz, &allocs, &cuda_allocs,  TOTAL_CUBES,  CUBE_SIZE));
    CUDA_TRY(cuda_copy_nested_buffer(&dev_v2px, v2px, &allocs, &cuda_allocs,  TOTAL_CUBES,  CUBE_SIZE));
    CUDA_TRY(cuda_copy_nested_buffer(&dev_v2pz, v2pz, &allocs, &cuda_allocs,  TOTAL_CUBES,  CUBE_SIZE));
    CUDA_TRY(cuda_copy_nested_buffer(&dev_v2sz, v2sz, &allocs, &cuda_allocs,  TOTAL_CUBES,  CUBE_SIZE));
    CUDA_TRY(cuda_copy_nested_buffer(&dev_v2pn, v2pn, &allocs, &cuda_allocs, TOTAL_CUBES, CUBE_SIZE));

    FP **dev_p_wave[3], **dev_q_wave[3];

    CUDA_TRY(cuda_copy_nested_buffer(&dev_p_wave[0], p_wave[0], &allocs, &cuda_allocs,  CUBE(g_width_in_cubes + 1),  CUBE_SIZE));
    CUDA_TRY(cuda_copy_nested_buffer(&dev_p_wave[1], p_wave[1], &allocs, &cuda_allocs,  CUBE(g_width_in_cubes + 1),  CUBE_SIZE));
    CUDA_TRY(cuda_copy_nested_buffer(&dev_p_wave[2], p_wave[2], &allocs, &cuda_allocs,  CUBE(g_width_in_cubes + 1),  CUBE_SIZE));
    CUDA_TRY(cuda_copy_nested_buffer(&dev_q_wave[0], q_wave[0], &allocs, &cuda_allocs,  CUBE(g_width_in_cubes + 1),  CUBE_SIZE));
    CUDA_TRY(cuda_copy_nested_buffer(&dev_q_wave[1], q_wave[1], &allocs, &cuda_allocs,  CUBE(g_width_in_cubes + 1),  CUBE_SIZE));
    CUDA_TRY(cuda_copy_nested_buffer(&dev_q_wave[2], q_wave[2], &allocs, &cuda_allocs,  CUBE(g_width_in_cubes + 1),  CUBE_SIZE));


    const uint64_t initialization_end_time = get_timestamp_ns();

    const double initialization_total_time = elapsed_seconds(initialization_end_time, initialization_start_time);

    printf("Initialization Elapsed time is: %lfs\n", initialization_total_time);

    const uint64_t start_time = get_timestamp_ns();

    for(int64_t t = 1; t <= st; t++){
      for (size_t k = 1; k < g_width_in_cubes + 1; k++) {
        for (size_t j = 1; j < g_width_in_cubes + 1; j++) {
          for (size_t i = 1; i < g_width_in_cubes + 1; i++) { // x

            const size_t idx = block_idx(i, j, k);

            const bool begin_z = k == 1;
            const bool stop_z = k == g_width_in_cubes;
            const bool begin_y = j == 1;
            const bool stop_y = j == g_width_in_cubes;
            const bool begin_x = i == 1;
            const bool stop_x = i == g_width_in_cubes;
            
            // if v == true, return the border size, else 0
            #define AS_BORDER(v) ((v) ? BORDER_WIDTH : 0)

	    const size_t x_start = AS_BORDER(begin_x);
	    const size_t y_start = AS_BORDER(begin_y);
	    const size_t z_start = AS_BORDER(begin_z);
	    const size_t x_end = AS_BORDER(stop_x);
	    const size_t y_end = AS_BORDER(stop_y);
	    const size_t z_end = AS_BORDER(stop_z);

            #undef AS_BORDER

            //pre computed values do not have a border and have to be adjusted as such
            const size_t precomp_idx = block_idx(i - 1, j - 1, k - 1);
	    FP* buffers[52];
	    
            buffers[0] = dev_ch1dxx[precomp_idx];
            buffers[1] = dev_ch1dyy[precomp_idx];
            buffers[2] = dev_ch1dzz[precomp_idx];
            buffers[3] = dev_ch1dxy[precomp_idx];
            buffers[4] = dev_ch1dyz[precomp_idx];
            buffers[5] = dev_ch1dxz[precomp_idx];
            buffers[6] = dev_v2px[precomp_idx];
            buffers[7] = dev_v2pz[precomp_idx];
            buffers[8] = dev_v2sz[precomp_idx];
            buffers[9] = dev_v2pn[precomp_idx];

            // p wave blocks
            buffers[10] = dev_p_wave[0][idx]; // write block

            buffers[11] = dev_p_wave[1][idx]; //central block when t - 1

            buffers[12] = dev_p_wave[1][block_idx(i + 0, j + 0, k - 1)];
            buffers[13] = dev_p_wave[1][block_idx(i + 0, j - 1, k - 1)];
            buffers[14] = dev_p_wave[1][block_idx(i - 1, j + 0, k - 1)];
            buffers[15] = dev_p_wave[1][block_idx(i + 1, j + 0, k - 1)];
            buffers[16] = dev_p_wave[1][block_idx(i + 0, j + 1, k - 1)];

            buffers[17] = dev_p_wave[1][block_idx(i - 1, j - 1, k + 0)];
            buffers[18] = dev_p_wave[1][block_idx(i + 0, j - 1, k + 0)];
            buffers[19] = dev_p_wave[1][block_idx(i + 1, j - 1, k + 0)];
            buffers[20] = dev_p_wave[1][block_idx(i - 1, j + 0, k + 0)];
            buffers[21] = dev_p_wave[1][block_idx(i + 1, j + 0, k + 0)];
            buffers[22] = dev_p_wave[1][block_idx(i - 1, j + 1, k + 0)];
            buffers[23] = dev_p_wave[1][block_idx(i + 0, j + 1, k + 0)];
            buffers[24] = dev_p_wave[1][block_idx(i + 1, j + 1, k + 0)];

            buffers[25] = dev_p_wave[1][block_idx(i + 0, j + 0, k + 1)];
            buffers[26] = dev_p_wave[1][block_idx(i + 0, j - 1, k + 1)];
            buffers[27] = dev_p_wave[1][block_idx(i - 1, j + 0, k + 1)];
            buffers[28] = dev_p_wave[1][block_idx(i + 1, j + 0, k + 1)];
            buffers[29] = dev_p_wave[1][block_idx(i + 0, j + 1, k + 1)];

            buffers[30] = dev_p_wave[2][idx]; //central block when t - 2

            // q wave blocks
            buffers[31] = dev_q_wave[0][idx]; // write block

            buffers[32] = dev_q_wave[1][idx]; //central block when t - 1

            buffers[33] = dev_q_wave[1][block_idx(i + 0, j + 0, k - 1)];
            buffers[34] = dev_q_wave[1][block_idx(i + 0, j - 1, k - 1)];
            buffers[35] = dev_q_wave[1][block_idx(i - 1, j + 0, k - 1)];
            buffers[36] = dev_q_wave[1][block_idx(i + 1, j + 0, k - 1)];
            buffers[37] = dev_q_wave[1][block_idx(i + 0, j + 1, k - 1)];

            buffers[38] = dev_q_wave[1][block_idx(i - 1, j - 1, k + 0)];
            buffers[39] = dev_q_wave[1][block_idx(i + 0, j - 1, k + 0)];
            buffers[40] = dev_q_wave[1][block_idx(i + 1, j - 1, k + 0)];
            buffers[41] = dev_q_wave[1][block_idx(i - 1, j + 0, k + 0)];
            buffers[42] = dev_q_wave[1][block_idx(i + 1, j + 0, k + 0)];
            buffers[43] = dev_q_wave[1][block_idx(i - 1, j + 1, k + 0)];
            buffers[44] = dev_q_wave[1][block_idx(i + 0, j + 1, k + 0)];
            buffers[45] = dev_q_wave[1][block_idx(i + 1, j + 1, k + 0)];

            buffers[46] = dev_q_wave[1][block_idx(i + 0, j + 0, k + 1)];
            buffers[47] = dev_q_wave[1][block_idx(i + 0, j - 1, k + 1)];
            buffers[48] = dev_q_wave[1][block_idx(i - 1, j + 0, k + 1)];
            buffers[49] = dev_q_wave[1][block_idx(i + 1, j + 0, k + 1)];
            buffers[50] = dev_q_wave[1][block_idx(i + 0, j + 1, k + 1)];

            buffers[51] = dev_q_wave[2][idx]; // central block when t - 2

            // TODO: call the function
            rtm_kernel_cuda(x_start, y_start, z_start, x_end, y_end, z_end, dx, dy, dz, dt, buffers);
	  }
	}
      }
        // rotate the iterations so that the currently computed values are the t - 1 values
        // reuse the space for the t - 2 for the new t values
        FP **tmp;
        tmp = dev_p_wave[0];
        dev_p_wave[0] = dev_p_wave[1];
        dev_p_wave[1] = dev_p_wave[2];
	dev_p_wave[2] = tmp;

        tmp = dev_q_wave[0];
        dev_q_wave[0] = dev_q_wave[1];
        dev_q_wave[1] = dev_q_wave[2];
        dev_q_wave[2] = tmp;

	printf("%ld iteration.\n", t);
    }
    
    const uint64_t end_time = get_timestamp_ns();
    const double total_elapsed_time = elapsed_seconds(end_time, start_time);

    printf("Computation Elapsed time is: %lfs\n", total_elapsed_time);

    const uint64_t samplesPropagate = CUBE(g_volume_width - 2 * BORDER_WIDTH);
    const uint64_t totalSamples = samplesPropagate * (uint64_t) st;

    #define MEGA 1.0e-6
    const double mega_samples = (MEGA * (double) totalSamples) / total_elapsed_time;
    printf("Msamples/s: %lf\n", mega_samples);


    program_end:
    mem_free(&allocs);
    cuda_mem_free(&cuda_allocs);
    return program_status;
}

