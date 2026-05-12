#include <starpu.h>
#include <stdint.h>
#include <stdbool.h>

#include <stdio.h>


#include "kernel.h"
#include "macros.h"
#include "floatingpoint.h"
#include "argparse.h"
#include "medium.h"
#include "mem.h"
#include "io.h"
#include "err.h"
#include "vector.h"

#define DEFAULT_OUTPUT_FOLDER "results"
#define DEFAULT_OUTPUT_NAME "form"

// 1, 3, 7
#define WAVE_PROPAGATION_CENTER  0
#define WAVE_PROPAGATION_SURFACE 1
#define WAVE_PROPAGATION_BORDER  2
#define WAVE_PROPAGATION_CORNER  3

// width for the volume of the whole sistem
size_t g_volume_width = 0;
// amount of segments in a dimension. 
// if g_volume_width = 100 and g_width_in_cubes = 10
// there are 10 segmentations by dimension, resulting in 1000 total cubes of 
// volume 1000
size_t g_width_in_cubes = 0;

// width for the segmented cube
size_t g_cube_width = 0;

const size_t BORDER_WIDTH = 4;

#define CUBE_SIZE (g_cube_width * g_cube_width * g_cube_width)
#define TOTAL_CUBES (g_width_in_cubes * g_width_in_cubes * g_width_in_cubes)

FP g_dt_output = FP_LIT(0.01);

// passes the iter[0] to iter[1], iter[1] to iter[2] and iter[2] to iter[0]
static inline void rotate_for_next_iter(starpu_data_handle_t* iter[3]){
    starpu_data_handle_t* tmp = iter[2]; 
    iter[2] = iter[1]; 
    iter[1] = iter[0]; 
    iter[0] = tmp; 
}


typedef struct dump_block_args {
    size_t i;
    size_t j;
    size_t k;
    size_t t;
} dump_block_args_t;

int make_dump_block_args(dump_block_args_t** args, size_t i, size_t j, size_t k, size_t t){
    //NOTE: isso quebra o meu script de reconstrução?
    if((*args = (dump_block_args_t*) malloc(sizeof(dump_block_args_t))) == NULL){
        return errno;
    }
    **args = (dump_block_args_t) {i, j, k, t};
    return 0;
}


void dump_block_kernel(void *descr[], void *cl_args){
    const dump_block_args_t* args = (dump_block_args_t*) cl_args;
    FP* block = (FP*) STARPU_BLOCK_GET_PTR(descr[0]);

    const size_t nx = STARPU_BLOCK_GET_NX(descr[0]);
    const size_t ny = STARPU_BLOCK_GET_NY(descr[0]);
    const size_t nz = STARPU_BLOCK_GET_NZ(descr[0]);

    io_state_write_file(args->i, args->j, args->k, args->t, nx, ny, nz, block);
}

#include <linux/time.h>
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


struct starpu_codelet dump_block_codelet = {
    .cpu_funcs = { dump_block_kernel },
    .nbuffers = 1,
    .modes = { STARPU_R }  // p wave
};

struct starpu_codelet insert_perturbation_codelet = {
    .cpu_funcs = { perturbation_kernel },
    .nbuffers = 2,
    .modes = { 
        STARPU_RW, // p wave
        STARPU_RW  // q wave
    }
};

struct starpu_codelet rtm_codelet = {
    .cpu_funcs = { rtm_kernel },
    .nbuffers = 52,
    .modes = {
        // precomputed values
        STARPU_R, // r at (i, j, k) of ch1dxx
        STARPU_R, // r at (i, j, k) of ch1dyy
        STARPU_R, // r at (i, j, k) of ch1dzz
        STARPU_R, // r at (i, j, k) of ch1dxy
        STARPU_R, // r at (i, j, k) of ch1dyz
        STARPU_R, // r at (i, j, k) of ch1dxz
        STARPU_R, // r at (i, j, k) of v2px
        STARPU_R, // r at (i, j, k) of v2pz
        STARPU_R, // r at (i, j, k) of v2sz
        STARPU_R, // r at (i, j, k) of v2pn

        STARPU_W, // w at (i, j, k) of p_wave_iter[0]

        STARPU_R, // r at (i, j, k) of p_wave_iter[1]
        // layer when k - 1
        // o x o
        // x x x 
        // o x o
        STARPU_R, // r at (i + 0, j + 0, k - 1) of p_wave_iter[1]
        STARPU_R, // r at (i + 0, j - 1, k - 1) of p_wave_iter[1]
        STARPU_R, // r at (i - 1, j + 0, k - 1) of p_wave_iter[1]
        STARPU_R, // r at (i + 1, j + 0, k - 1) of p_wave_iter[1]
        STARPU_R, // r at (i + 0, j + 1, k - 1) of p_wave_iter[1]

        // layer when k
        // x x x
        // x o x 
        // x x x
        STARPU_R, // r at (i - 1, j - 1, k + 0) of p_wave_iter[1]
        STARPU_R, // r at (i + 0, j - 1, k + 0) of p_wave_iter[1]
        STARPU_R, // r at (i + 1, j - 1, k + 0) of p_wave_iter[1]
        STARPU_R, // r at (i - 1, j + 0, k + 0) of p_wave_iter[1]
        STARPU_R, // r at (i + 1, j + 0, k + 0) of p_wave_iter[1]
        STARPU_R, // r at (i - 1, j + 1, k + 0) of p_wave_iter[1]
        STARPU_R, // r at (i + 0, j + 1, k + 0) of p_wave_iter[1]
        STARPU_R, // r at (i + 1, j + 1, k + 0) of p_wave_iter[1]

        // layer when k + 1
        // o x o
        // x x x 
        // o x o
        STARPU_R, // r at (i + 0, j + 0, k + 1) of p_wave_iter[1]
        STARPU_R, // r at (i + 0, j - 1, k + 1) of p_wave_iter[1]
        STARPU_R, // r at (i - 1, j + 0, k + 1) of p_wave_iter[1]
        STARPU_R, // r at (i + 1, j + 0, k + 1) of p_wave_iter[1]
        STARPU_R, // r at (i + 0, j + 1, k + 1) of p_wave_iter[1]

        STARPU_R,  // r at (i, j, k) of p_wave_iter[2]

        STARPU_W, // w at (i, j, k) of q_wave_iter[0]

        STARPU_R, // r at (i, j, k) of q_wave_iter[1]
        // layer when k - 1
        // o x o
        // x x x 
        // o x o
        STARPU_R, // r at (i + 0, j + 0, k - 1) of q_wave_iter[1]
        STARPU_R, // r at (i + 0, j - 1, k - 1) of q_wave_iter[1]
        STARPU_R, // r at (i - 1, j + 0, k - 1) of q_wave_iter[1]
        STARPU_R, // r at (i + 1, j + 0, k - 1) of q_wave_iter[1]
        STARPU_R, // r at (i + 0, j + 1, k - 1) of q_wave_iter[1]

        // layer when k
        // x x x
        // x o x 
        // x x x
        STARPU_R, // r at (i - 1, j - 1, k + 0) of q_wave_iter[1]
        STARPU_R, // r at (i + 0, j - 1, k + 0) of q_wave_iter[1]
        STARPU_R, // r at (i + 1, j - 1, k + 0) of q_wave_iter[1]
        STARPU_R, // r at (i - 1, j + 0, k + 0) of q_wave_iter[1]
        STARPU_R, // r at (i + 1, j + 0, k + 0) of q_wave_iter[1]
        STARPU_R, // r at (i - 1, j + 1, k + 0) of q_wave_iter[1]
        STARPU_R, // r at (i + 0, j + 1, k + 0) of q_wave_iter[1]
        STARPU_R, // r at (i + 1, j + 1, k + 0) of q_wave_iter[1]

        // layer when k + 1
        // o x o
        // x x x 
        // o x o
        STARPU_R, // r at (i + 0, j + 0, k + 1) of q_wave_iter[1]
        STARPU_R, // r at (i + 0, j - 1, k + 1) of q_wave_iter[1]
        STARPU_R, // r at (i - 1, j + 0, k + 1) of q_wave_iter[1]
        STARPU_R, // r at (i + 1, j + 0, k + 1) of q_wave_iter[1]
        STARPU_R, // r at (i + 0, j + 1, k + 1) of q_wave_iter[1]

        STARPU_R  // r at (i, j, k) of q_wave_iter[2]
    },
    .model = &starpu_perfmodel_nop,
};

err_t write_wave(int64_t* n_out, starpu_data_handle_t* wave_iter){

    err_t err = 0;
    // salva o primeiro bloco (nulo)
    for(size_t k = 1; k < g_width_in_cubes + 1; k++)
    for(size_t j = 1; j < g_width_in_cubes + 1; j++)
    for(size_t i = 1; i < g_width_in_cubes + 1; i++){

        // call the write task
        dump_block_args_t* dump_args;
        if((err = make_dump_block_args(&dump_args, i - 1, j - 1, k - 1, *n_out)) != 0){
            return err;
        }

        struct starpu_task* dump_block_task = starpu_task_create();
        dump_block_task->name = "write_block";
        dump_block_task->cl = &dump_block_codelet;
        dump_block_task->cl_arg = dump_args;
        dump_block_task->cl_arg_size = sizeof(dump_block_args_t);
        dump_block_task->cl_arg_free = 1;
        //NOTE: mudado para p_wave_iter[0] para se adequar melhor com o trace do fletcher base
        //      no caso não importa pois write(t = 0) é só zeros no original
        dump_block_task->handles[0] = wave_iter[block_idx(i, j, k)];

        if((err = starpu_task_submit(dump_block_task)) != 0){
            return err;
        };
    }
    (*n_out)++;
    return 0;
}


#ifdef RELEASE
// if x fails (!= 0), exit the program;
#define TRY(x,...) TRYTO(x, program_status = EXIT_FAILURE, program_end)
#define DEBUG(...) 
#else
// if x fails (!= 0), goto the end of main and log status;
err_t g_err;
#define TRY(x,...) TRYTO(g_err = (x), program_status = EXIT_FAILURE; \
    printf("[error] Failed at line %d with error %ld: %s\n", __LINE__, g_err, err_name(g_err)); \
    printf("[err-msg] " __VA_ARGS__);, program_end)

#define DEBUG(...) printf("[debug] " __VA_ARGS__)
#endif

#ifndef RANDOM_SEED
#define RANDOM_SEED 0
#endif

int main(int argc, char **argv){

    srand(RANDOM_SEED);

    //need to be toplevel for the try macro
    int program_status = EXIT_SUCCESS;
    vector(void*) starpu_allocations = NULL;
    vector(void*) allocs = NULL;
    vector(void*) medium_allocs = NULL;


	int ret = starpu_init(NULL);
	STARPU_CHECK_RETURN_VALUE(ret, "starpu_init");

    char* output_folder = DEFAULT_OUTPUT_FOLDER;
    get_envvar(&output_folder, "OUTPUT_FOLDER");

    char* output_filename = DEFAULT_OUTPUT_NAME;
    get_envvar(&output_filename, "OUTPUT_FILE");

    int64_t enable_io = 1;
    get_envvar(&enable_io, "ENABLE_IO");
    

    enum Form form = 0;
    char* form_str = NULL;
    uint32_t nx, ny, nz, absorb_width;
    FP dx, dy, dz, dt, tmax;

    TRY(read_args(&argc, argv, 12, 
        ARG_str, &form_str, 
        ARG_u32, &nx, ARG_u32, &ny, ARG_u32, &nz, ARG_u32, &absorb_width, 
        FP_ARG, &dx, FP_ARG, &dy, FP_ARG, &dz, FP_ARG, &dt, FP_ARG, &tmax,
        ARG_u64, &g_width_in_cubes, FP_ARG, &g_dt_output
    ), "Error in parsing arg at %d\n.", argc);

    TRY(str_to_medium(form_str, &form), "Failed at string to medium conversion");
    
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
    // amount of iterations the program will save the block
    const size_t total_saved_moments = (size_t) FP_CEIL(tmax / g_dt_output) + 1;

    // the point in the global volume that the source is inserted
    const size_t volume_propagation_idx = volume_idx(g_volume_width / 2, g_volume_width / 2, g_volume_width / 2);
    DEBUG("Index of propagation is %ld.\n", volume_propagation_idx);


    /* 
    const size_t max_count_of_page_size = 16;
    size_t available_page_sizes[16];
    size_t page_size_results = 0;
    
    TRY(io_available_huge_page_sizes(max_count_of_page_size, &page_size_results, &available_page_sizes[0]));
    TRY((page_size_results > 0 || page_size_results <= max_count_of_page_size) ? 0 : ME_COUNT_DONT_MATCH);

    size_t max_page_size = 0;
    for(size_t i = 0; i < page_size_results; i++){
        printf("%d: page size %ld\n", i, available_page_sizes[i]);
        //max_page_size = max_page_size < available_page_sizes[i] ? available_page_sizes[i] : max_page_size;
    }
    max_page_size = available_page_sizes[0];
    */

    char bin_filename[512] = {'\0'};
    sprintf(bin_filename, "%s/out-%s.rsf@", output_folder, output_filename);
    printf("bin filename %s\n", bin_filename);

    //init the IO here
    TRY(io_state_init(bin_filename, 4096, total_saved_moments, g_volume_width * g_volume_width * g_volume_width));

    // mem_allocate the buffers that will be used in computing the 
    // intermediary values
    FP *vpz, *vsv, *epsilon, *delta, *phi, *theta;
    const size_t medium_size = sizeof(FP) * CUBE(g_volume_width);

    TRY(mem_allocate(medium_allocs, (void**) &vpz, medium_size));
    TRY(mem_allocate(medium_allocs, (void**) &vsv, medium_size));
    TRY(mem_allocate(medium_allocs, (void**) &epsilon, medium_size));
    TRY(mem_allocate(medium_allocs, (void**) &delta, medium_size));
    TRY(mem_allocate(medium_allocs, (void**) &phi, medium_size));
    TRY(mem_allocate(medium_allocs, (void**) &theta, medium_size));

    // inicialize the buffers above based on the type of medium
    medium_initialize(form, CUBE(g_volume_width), vpz, vsv, epsilon, delta, phi, theta);
    
    // set the absorption zone for vpz and vsv
    medium_random_velocity_boundary(BORDER_WIDTH, absorb_width, vpz, vsv);

    // run the stability condition for the size
    const FP stability_condition = medium_stability_condition(dx, dy, dz, vpz, epsilon, CUBE(g_volume_width));
    DEBUG("The stability condition (proper value for dt) for this problem is %lf.\n", stability_condition);

    FP **ch1dxx, **ch1dyy, **ch1dzz, **ch1dxy, **ch1dyz, **ch1dxz, **v2px, **v2pz, **v2sz, **v2pn;
    #define ALLOCATE_NESTED_BUFFER(v) \
        TRY(mem_allocate(allocs, (void**) &v, TOTAL_CUBES * sizeof(FP*))); \
        for(size_t i = 0; i < TOTAL_CUBES; i++) \
            TRY(mem_allocate_starpu(starpu_allocations, (void**)(v + i), CUBE_SIZE * sizeof(FP)));

    ALLOCATE_NESTED_BUFFER(ch1dxx);
    ALLOCATE_NESTED_BUFFER(ch1dyy);
    ALLOCATE_NESTED_BUFFER(ch1dzz);
    ALLOCATE_NESTED_BUFFER(ch1dxy);
    ALLOCATE_NESTED_BUFFER(ch1dyz);
    ALLOCATE_NESTED_BUFFER(ch1dxz);
    ALLOCATE_NESTED_BUFFER(v2px);
    ALLOCATE_NESTED_BUFFER(v2pz);
    ALLOCATE_NESTED_BUFFER(v2sz);
    ALLOCATE_NESTED_BUFFER(v2pn);


    medium_calc_intermediary_values(
        vpz, vsv, epsilon, delta, phi, theta,
        ch1dxx, ch1dyy, ch1dzz, ch1dxy, ch1dyz, ch1dxz, 
        v2px, v2pz, v2sz, v2pn
    );

    //at this point the values for the medium will not be used again
    vector_free_all(medium_allocs, free);

    // a iteração do bloco t depende dos blocos t - 1 e t - 2.
    // aloca-se mais data_handles que necessário, compreendendo 0..g_width_in_cubes + 2
    // isso evita checks de bounds, pois os cubos internos tem uma borda que evita acessar fora deles no limite do volume
    // esses buffers extras exitem para inserir buffers válidos na ordem certa, mas eles nunca são acessados
    // na hora de fazer o `starpu_block_data_register` e `starpu_data_unregister_submit`, evita os blocos de borda
    // dessa forma, dentro do loop de execução de taregas i - 1 ou i + 1 são sempre índices válidos na lista de `data_handle_t`.
    starpu_data_handle_t* p_wave_iter[3];
    TRY(mem_allocate(allocs, (void**) &p_wave_iter[0], CUBE(g_width_in_cubes + 2) * sizeof(starpu_data_handle_t)));
    TRY(mem_allocate(allocs, (void**) &p_wave_iter[1], CUBE(g_width_in_cubes + 2) * sizeof(starpu_data_handle_t)));
    TRY(mem_allocate(allocs, (void**) &p_wave_iter[2], CUBE(g_width_in_cubes + 2) * sizeof(starpu_data_handle_t)));

    starpu_data_handle_t* q_wave_iter[3];
    TRY(mem_allocate(allocs, (void**) &q_wave_iter[0], CUBE(g_width_in_cubes + 2) * sizeof(starpu_data_handle_t)));
    TRY(mem_allocate(allocs, (void**) &q_wave_iter[1], CUBE(g_width_in_cubes + 2) * sizeof(starpu_data_handle_t)));
    TRY(mem_allocate(allocs, (void**) &q_wave_iter[2], CUBE(g_width_in_cubes + 2) * sizeof(starpu_data_handle_t)));

    starpu_data_handle_t *hdl_ch1dxx, *hdl_ch1dyy, *hdl_ch1dzz, 
        *hdl_ch1dxy, *hdl_ch1dyz, *hdl_ch1dxz, 
        *hdl_v2px, *hdl_v2pz, *hdl_v2sz, *hdl_v2pn;
    TRY(mem_allocate(allocs, (void**) &hdl_ch1dxx, CUBE(g_width_in_cubes) * sizeof(starpu_data_handle_t)));
    TRY(mem_allocate(allocs, (void**) &hdl_ch1dyy, CUBE(g_width_in_cubes) * sizeof(starpu_data_handle_t)));
    TRY(mem_allocate(allocs, (void**) &hdl_ch1dzz, CUBE(g_width_in_cubes) * sizeof(starpu_data_handle_t)));
    TRY(mem_allocate(allocs, (void**) &hdl_ch1dxy, CUBE(g_width_in_cubes) * sizeof(starpu_data_handle_t)));
    TRY(mem_allocate(allocs, (void**) &hdl_ch1dyz, CUBE(g_width_in_cubes) * sizeof(starpu_data_handle_t)));
    TRY(mem_allocate(allocs, (void**) &hdl_ch1dxz, CUBE(g_width_in_cubes) * sizeof(starpu_data_handle_t)));
    TRY(mem_allocate(allocs, (void**) &hdl_v2px,   CUBE(g_width_in_cubes) * sizeof(starpu_data_handle_t)));
    TRY(mem_allocate(allocs, (void**) &hdl_v2pz,   CUBE(g_width_in_cubes) * sizeof(starpu_data_handle_t)));
    TRY(mem_allocate(allocs, (void**) &hdl_v2sz,   CUBE(g_width_in_cubes) * sizeof(starpu_data_handle_t)));
    TRY(mem_allocate(allocs, (void**) &hdl_v2pn,   CUBE(g_width_in_cubes) * sizeof(starpu_data_handle_t)));


    #define BLOCK_REGISTER(handle, ptr) starpu_block_data_register((handle), STARPU_MAIN_RAM, (uintptr_t) (ptr), \
        g_cube_width, SQUARE(g_cube_width), g_cube_width, g_cube_width, g_cube_width, sizeof(FP))

    for(size_t idx = 0; idx < TOTAL_CUBES; idx++){
        BLOCK_REGISTER(hdl_ch1dxx + idx, ch1dxx[idx]);
        BLOCK_REGISTER(hdl_ch1dyy + idx, ch1dyy[idx]);
        BLOCK_REGISTER(hdl_ch1dzz + idx, ch1dzz[idx]);
        BLOCK_REGISTER(hdl_ch1dxy + idx, ch1dxy[idx]);
        BLOCK_REGISTER(hdl_ch1dyz + idx, ch1dyz[idx]);
        BLOCK_REGISTER(hdl_ch1dxz + idx, ch1dxz[idx]);
        BLOCK_REGISTER(hdl_v2px + idx, v2px[idx]);
        BLOCK_REGISTER(hdl_v2pz + idx, v2pz[idx]);
        BLOCK_REGISTER(hdl_v2sz + idx, v2sz[idx]);
        BLOCK_REGISTER(hdl_v2pn + idx, v2pn[idx]);
    }
    
    // alocate the initial values for the waves pp, pc, qp, qc.
    // the null_block holds all zeros, which all blocks in pp and qp are
    // the propagation_block holds the point in which the propagation is set
    // therefore all data handles set are referêncing the null_block or the progration_block
    // which is only referênced once.
    FP *null_block, *propagation_block; 

    TRY(mem_allocate_starpu(starpu_allocations, (void**) &null_block, CUBE_SIZE * sizeof(FP)));
    TRY(mem_allocate_starpu(starpu_allocations, (void**) &propagation_block, CUBE_SIZE * sizeof(FP)));

    for(size_t b_i = 0; b_i < CUBE_SIZE; b_i++){
        null_block[b_i] = propagation_block[b_i] = FP_LIT(0.0);
    }
    // iSource in the cube
    const size_t perturbation_source_pos = volume_to_cube_idx(volume_propagation_idx);
    //insert in this block the propagration source
    //const FP starting_source_value = medium_source_value(dt, 0);

    //printf("source value: %.9f\n", starting_source_value);
    //propagation_block[perturbation_source_pos] = starting_source_value;

    // because it added a padding to every side of the block collection, 
    // we need to move the perturbation_source cube by one in every direction
    //block_idx((g_width_in_cubes + 2) / 2, (g_width_in_cubes + 2) / 2, (g_width_in_cubes + 2) / 2);
    const size_t perturbation_source_cube = volume_to_block_idx(volume_propagation_idx) + block_idx(1, 1, 1);

    for(size_t idx = 0; idx < CUBE(g_width_in_cubes + 2); idx++){
        BLOCK_REGISTER(p_wave_iter[0] + idx, null_block);
        BLOCK_REGISTER(q_wave_iter[0] + idx, null_block);
        // if we are initializing at the idx of the block that will be perturbed 
        // used the block with the perturbation source
        if(idx == perturbation_source_cube){
            BLOCK_REGISTER(p_wave_iter[1] + idx, propagation_block);
            BLOCK_REGISTER(q_wave_iter[1] + idx, propagation_block);
        }else{
            BLOCK_REGISTER(p_wave_iter[1] + idx, null_block);
            BLOCK_REGISTER(q_wave_iter[1] + idx, null_block);
        }
        BLOCK_REGISTER(p_wave_iter[2] + idx, null_block);
        BLOCK_REGISTER(q_wave_iter[2] + idx, null_block);
    }
    // TODO: rename n_out para algo que faça mais sentido

    int64_t n_out = 0;
    // salva o primeiro bloco (nulo)
    if(enable_io){
        TRY(write_wave(&n_out, p_wave_iter[0]));
    }

    const uint64_t start_time = get_timestamp_ns();

    for(int64_t t = 1; t <= st; t++){
        // printf("t: %d\n", t);
        starpu_iteration_push(t);

        //mover para aqui a inserção da fonte

        // task that insert the perturbation on the handle
        struct starpu_task* perturb_task = starpu_task_create();
        perturb_task->name = "wave_insertion";

        const FP source_value = medium_source_value(dt, t - 1);
        perturb_args_t* perturb_args;
        TRY(make_perturb_args(&perturb_args, perturbation_source_pos, source_value, t - 1));

        perturb_task->cl = &insert_perturbation_codelet;
        perturb_task->cl_arg = perturb_args;
        perturb_task->cl_arg_size = sizeof(perturb_args_t);
        perturb_task->cl_arg_free = 1; // free the args after use

        // insert in the $A_{t-1}$ wave that will be used for computing the A_t
        perturb_task->handles[0] = p_wave_iter[1][perturbation_source_cube];
        perturb_task->handles[1] = q_wave_iter[1][perturbation_source_cube];

        TRY(starpu_task_submit(perturb_task));

        for(size_t k = 1; k < g_width_in_cubes + 1; k++) // z
        for(size_t j = 1; j < g_width_in_cubes + 1; j++) // y
        for(size_t i = 1; i < g_width_in_cubes + 1; i++){// x
            const size_t idx = block_idx(i, j, k);

            //add to the curr buff
            //let starpu allocate the data by setting home_node = -1 
            starpu_block_data_register(&p_wave_iter[0][idx], -1, 0,
                g_cube_width, SQUARE(g_cube_width),
                g_cube_width, g_cube_width, g_cube_width, sizeof(FP)
            );

            starpu_block_data_register(&q_wave_iter[0][idx], -1, 0,
                g_cube_width, SQUARE(g_cube_width),
                g_cube_width, g_cube_width, g_cube_width, sizeof(FP)
            );

            struct starpu_task* task = starpu_task_create();
            task->name = "wave_propagation";
            task->cl = &rtm_codelet;

            const bool begin_z = k == 1;
            const bool stop_z = k == g_width_in_cubes;
            const bool begin_y = j == 1;
            const bool stop_y = j == g_width_in_cubes;
            const bool begin_x = i == 1;
            const bool stop_x = i == g_width_in_cubes;
            
            // if v == true, return the border size, else 0
            #define AS_BORDER(v) ((v) ? BORDER_WIDTH : 0)

            struct rtm_args* rtm_args;
            TRY(make_rtm_args(&rtm_args, 
                AS_BORDER(begin_x), g_cube_width - AS_BORDER(stop_x),
                AS_BORDER(begin_y), g_cube_width - AS_BORDER(stop_y),
                AS_BORDER(begin_z), g_cube_width - AS_BORDER(stop_z),
                dx, dy, dz, dt
            ));

            task->cl_arg = rtm_args;
            task->cl_arg_size = sizeof(struct rtm_args);
            task->cl_arg_free = 1; // free the args after use

            #undef AS_BORDER

            task->use_tag = 1;
            task->tag_id = 
                begin_z || stop_z ? 1 : 0 + 
                begin_y || stop_y ? 1 : 0 + 
                begin_x || stop_x ? 1 : 0;
            

            //sprintf(cl_args->name, "[%d, %d, %d, %ld]", i, j, k, t + 1);
            //task->name = cl_args->name;


            //select the handles
            //          ^   ^
            //          |  /
            //          y z
            //          |/
            // -- x -- >
            // ordem ao invés de rotacional vai ser via eixo,
            // blocos do z (-1, +1), depois do y, depois do x
            // dessa forma, o primeiro bloco é o (-1, -1, -1), 
            // depois o (-1, -1, 0), (-1, -1, 1), (-1, 0, -1) ...
            // essa lista inclui as diagonais que devem ser omitidas, *rsf_body = NULL

            //pre computed values do not have a border and have to be adjusted as such
            const size_t precomp_idx = block_idx(i - 1, j - 1, k - 1);
            task->handles[0] = hdl_ch1dxx[precomp_idx];
            task->handles[1] = hdl_ch1dyy[precomp_idx];
            task->handles[2] = hdl_ch1dzz[precomp_idx];
            task->handles[3] = hdl_ch1dxy[precomp_idx];
            task->handles[4] = hdl_ch1dyz[precomp_idx];
            task->handles[5] = hdl_ch1dxz[precomp_idx];
            task->handles[6] = hdl_v2px[precomp_idx];
            task->handles[7] = hdl_v2pz[precomp_idx];
            task->handles[8] = hdl_v2sz[precomp_idx];
            task->handles[9] = hdl_v2pn[precomp_idx];

            // p wave blocks
            task->handles[10] = p_wave_iter[0][idx]; // write block

            task->handles[11] = p_wave_iter[1][idx]; //central block when t - 1

            task->handles[12] = p_wave_iter[1][block_idx(i + 0, j + 0, k - 1)];
            task->handles[13] = p_wave_iter[1][block_idx(i + 0, j - 1, k - 1)];
            task->handles[14] = p_wave_iter[1][block_idx(i - 1, j + 0, k - 1)];
            task->handles[15] = p_wave_iter[1][block_idx(i + 1, j + 0, k - 1)];
            task->handles[16] = p_wave_iter[1][block_idx(i + 0, j + 1, k - 1)];

            task->handles[17] = p_wave_iter[1][block_idx(i - 1, j - 1, k + 0)];
            task->handles[18] = p_wave_iter[1][block_idx(i + 0, j - 1, k + 0)];
            task->handles[19] = p_wave_iter[1][block_idx(i + 1, j - 1, k + 0)];
            task->handles[20] = p_wave_iter[1][block_idx(i - 1, j + 0, k + 0)];
            task->handles[21] = p_wave_iter[1][block_idx(i + 1, j + 0, k + 0)];
            task->handles[22] = p_wave_iter[1][block_idx(i - 1, j + 1, k + 0)];
            task->handles[23] = p_wave_iter[1][block_idx(i + 0, j + 1, k + 0)];
            task->handles[24] = p_wave_iter[1][block_idx(i + 1, j + 1, k + 0)];

            task->handles[25] = p_wave_iter[1][block_idx(i + 0, j + 0, k + 1)];
            task->handles[26] = p_wave_iter[1][block_idx(i + 0, j - 1, k + 1)];
            task->handles[27] = p_wave_iter[1][block_idx(i - 1, j + 0, k + 1)];
            task->handles[28] = p_wave_iter[1][block_idx(i + 1, j + 0, k + 1)];
            task->handles[29] = p_wave_iter[1][block_idx(i + 0, j + 1, k + 1)];

            task->handles[30] = p_wave_iter[2][idx]; //central block when t - 2

            // q wave blocks
            task->handles[31] = q_wave_iter[0][idx]; // write block

            task->handles[32] = q_wave_iter[1][idx]; //central block when t - 1

            task->handles[33] = q_wave_iter[1][block_idx(i + 0, j + 0, k - 1)];
            task->handles[34] = q_wave_iter[1][block_idx(i + 0, j - 1, k - 1)];
            task->handles[35] = q_wave_iter[1][block_idx(i - 1, j + 0, k - 1)];
            task->handles[36] = q_wave_iter[1][block_idx(i + 1, j + 0, k - 1)];
            task->handles[37] = q_wave_iter[1][block_idx(i + 0, j + 1, k - 1)];

            task->handles[38] = q_wave_iter[1][block_idx(i - 1, j - 1, k + 0)];
            task->handles[39] = q_wave_iter[1][block_idx(i + 0, j - 1, k + 0)];
            task->handles[40] = q_wave_iter[1][block_idx(i + 1, j - 1, k + 0)];
            task->handles[41] = q_wave_iter[1][block_idx(i - 1, j + 0, k + 0)];
            task->handles[42] = q_wave_iter[1][block_idx(i + 1, j + 0, k + 0)];
            task->handles[43] = q_wave_iter[1][block_idx(i - 1, j + 1, k + 0)];
            task->handles[44] = q_wave_iter[1][block_idx(i + 0, j + 1, k + 0)];
            task->handles[45] = q_wave_iter[1][block_idx(i + 1, j + 1, k + 0)];

            task->handles[46] = q_wave_iter[1][block_idx(i + 0, j + 0, k + 1)];
            task->handles[47] = q_wave_iter[1][block_idx(i + 0, j - 1, k + 1)];
            task->handles[48] = q_wave_iter[1][block_idx(i - 1, j + 0, k + 1)];
            task->handles[49] = q_wave_iter[1][block_idx(i + 1, j + 0, k + 1)];
            task->handles[50] = q_wave_iter[1][block_idx(i + 0, j + 1, k + 1)];

            task->handles[51] = q_wave_iter[2][idx]; //central block when t - 2

            TRY(starpu_task_submit(task));
        }

        //starpu_task_wait_for_all(); // Force syncronize

        //starpu_task_wait_for_all();
        // only output the block when simulation time overtakes the min time to generate output
        const FP simulation_time = t * dt;
        const FP output_time = n_out * g_dt_output;
        if(simulation_time >= output_time){ // hand made fst iter to dump
            if(enable_io){
                TRY(write_wave(&n_out, p_wave_iter[0]));
            }
        }

        if(t >= 2){
            for(size_t k = 1; k < g_width_in_cubes + 1; k++)
            for(size_t j = 1; j < g_width_in_cubes + 1; j++)
            for(size_t i = 1; i < g_width_in_cubes + 1; i++){
                starpu_data_unregister_submit(p_wave_iter[2][block_idx(i, j, k)]);
                starpu_data_unregister_submit(q_wave_iter[2][block_idx(i, j, k)]);
            }
        }
        // rotate the iterations so that the currently computed values are the t - 1 values
        // reuse the space for the t - 2 for the new t values
        rotate_for_next_iter(p_wave_iter);
        rotate_for_next_iter(q_wave_iter);

        starpu_iteration_pop();
    }
    DEBUG("Submitted all tasks\n");
    //at least after all iterations
    starpu_task_wait_for_all();

    const uint64_t end_time = get_timestamp_ns();

    const double total_elapsed_time = elapsed_seconds(end_time, start_time);

    printf("Computation Elapsed time is: %lfs\n", total_elapsed_time);

    const uint64_t samplesPropagate = CUBE(g_volume_width - 2 * BORDER_WIDTH);
    const uint64_t totalSamples = samplesPropagate * (uint64_t) st;

    #define MEGA 1.0e-6
    const double mega_samples = (MEGA * (double) totalSamples) / total_elapsed_time;

    printf("Msamples/s: %lf\n", mega_samples);


    // cleanup all remaning handles (p_wave_iter[2] and iteraions[1])
    for(size_t k = 1; k < g_width_in_cubes + 1; k++){
        for(size_t j = 1; j < g_width_in_cubes + 1; j++){
            for(size_t i = 1; i < g_width_in_cubes + 1; i++){
                starpu_data_handle_t ph1 = p_wave_iter[1][block_idx(i, j, k)];
                starpu_data_handle_t ph2 = p_wave_iter[2][block_idx(i, j, k)];

                starpu_data_handle_t qh1 = q_wave_iter[1][block_idx(i, j, k)];
                starpu_data_handle_t qh2 = q_wave_iter[2][block_idx(i, j, k)];
                /*
                // first need to acquire the data
                TRY(starpu_data_acquire(ph1, STARPU_R));

                starpu_ssize_t size = sizeof(double) * CUBE_SIZE;
                TRY(starpu_data_pack(ph1, (void**)&result_block, &size));

                //print_block(result_block);
                aggregate_block_buffers(result_volume, result_block, i - 1, j - 1, k - 1);
                clear_block(result_block, g_cube_width);

                //NOTE: função aqui para testar se a borda não está sendo violada
                TRY(has_clear_edge(result_block, i, j, k), "block %ld, %ld, %ld fails clear block test", i, j, k);

                //then release
                starpu_data_release(ph1);
                */
                starpu_data_unregister(ph1);
                starpu_data_unregister(ph2);
                starpu_data_unregister(qh1);
                starpu_data_unregister(qh2);
            }
        }
    }

    //Write the header file
    FILE *rsf_header = NULL;
    char filename[512] = {'\0'};
    sprintf(filename, "%s/out-%s.rsf", output_folder, output_filename);

    TRY((rsf_header = fopen(filename, "w+")) == NULL);

    fprintf(rsf_header, "in=\"%s\"\n", io_state_get_filename());
    fprintf(rsf_header, "data_format=\"native_float\"\n");
    fprintf(rsf_header, "esize=%lu\n", sizeof(float)); 
    fprintf(rsf_header, "n1=%ld\n", g_volume_width /*sx*/);
    fprintf(rsf_header, "n2=%ld\n", g_volume_width /*sy*/);
    fprintf(rsf_header, "n3=%ld\n", g_volume_width /*sz*/);
    fprintf(rsf_header, "n4=%ld\n", n_out); // TODO: validar n_out 
    fprintf(rsf_header, "d1=%f\n", dx);
    fprintf(rsf_header, "d2=%f\n", dy);
    fprintf(rsf_header, "d3=%f\n", dz);
    fprintf(rsf_header, "d4=%f\n", dt);
    fprintf(rsf_header, "seg=%ld\n", g_width_in_cubes);

    fclose(rsf_header);


    program_end:

    vector_free_all(medium_allocs, free);
    vector_free_all(allocs, free);
    #define STARPU_FREE(x) starpu_free_noflag(x, CUBE_SIZE * sizeof(FP));
    vector_free_all(starpu_allocations, STARPU_FREE);

	starpu_shutdown();
    assert(io_state_finish() == 0);
	return program_status;
}

