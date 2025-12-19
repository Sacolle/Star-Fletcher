#include <starpu.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "kernel.h"
#include "macros.h"
#include "floatingpoint.h"
#include "argparse.h"
#include "medium.h"

#include "vector.h"


const size_t BORDER_WIDTH = 4;


// width for the volume of the whole sistem
size_t g_volume_width = 0;
// amount of segments in a dimension. 
// if g_volume_width = 100 and g_width_in_cubes = 10
// there are 10 segmentations by dimension, resulting in 1000 total cubes of 
// volume 1000
size_t g_width_in_cubes = 0;

// width for the segmented cube
size_t g_cube_width = 0;

#define MAXFILES 32
FILE* g_worker_files[MAXFILES];


#define CUBE_SIZE (g_cube_width * g_cube_width * g_cube_width)
#define TOTAL_CUBES (g_width_in_cubes * g_width_in_cubes * g_width_in_cubes)

const char* out_folder = "result";

void print_block(double* block){
    printf("[\n");
    for(size_t z = 0; z < g_cube_width; z++){
        printf("[\n");
        for(size_t y = 0; y < g_cube_width; y++){
            printf("\t[");
            for(size_t x = 0; x < g_cube_width; x++){
                printf("%.2lf", block[cube_idx(x, y, z)]);
                if(x != g_cube_width - 1){
                    printf(", ");
                }
            }
            if(y != g_cube_width - 1){ printf("],\n"); }else{ printf("]\n"); }
        }
        if(z != g_cube_width - 1){ printf("],\n"); }else{ printf("]\n"); }
    }
    printf("]");
}

void clear_block(FP* block, size_t width){
    for(size_t z = 0; z < width; z++){
        for(size_t y = 0; y < width; y++){
            for(size_t x = 0; x < width; x++){
                block[idx(x, y, z, width)] = 0.0;
            }
        }
    }
}

int allocate(vector(void*) v, void** ptr, const size_t size){
    if((*ptr = malloc(size)) == NULL){
        return 1;
    }
    vector_push(v, *ptr);
    return 0;
}

int allocate_starpu(vector(void*) v, void** ptr, const size_t size){
    if(starpu_malloc(ptr, size) != 0){
        return 1;
    }
    vector_push(v, *ptr);
    return 0;
}

// passes the iter[0] to iter[1], iter[1] to iter[2] and iter[2] to iter[0]
static inline void rotate_for_next_iter(starpu_data_handle_t* iter[3]){
    starpu_data_handle_t* tmp = iter[2]; 
    iter[2] = iter[1]; 
    iter[1] = iter[0]; 
    iter[0] = tmp; 
}


#define FSTBLK(blk) ((blk) == 1)
#define LSTBLK(blk) ((blk) == g_width_in_cubes)
#define START(blk, idx) (FSTBLK(blk) && (idx) < BORDER_WIDTH)
#define END(blk, idx) (LSTBLK(blk) && (idx) >= (g_cube_width - BORDER_WIDTH))
#define INEDGE(blk_idx, cidx) (START(blk_idx, cidx) || END(blk_idx, cidx))

#define EPSILON 0.0001
int has_clear_edge(FP* block, size_t i, size_t j, size_t k){
    for(size_t z = 0; z < g_cube_width; z++){
        for(size_t y = 0; y < g_cube_width; y++){
            for(size_t x = 0; x < g_cube_width; x++){
                const size_t idx = cube_idx(x, y, z);
                if(INEDGE(k, z) || INEDGE(j, y) || INEDGE(i, x)){
                    if(block[idx] < EPSILON || block[idx] > -EPSILON){

                    }else{
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}

void aggregate_block_buffers(FP* volume, FP* block, size_t i, size_t j, size_t k){
    for(size_t z = 0; z < g_cube_width; z++)
    for(size_t y = 0; y < g_cube_width; y++)
    for(size_t x = 0; x < g_cube_width; x++){
        //calculate the absolute index from the block index and the cords
        const size_t idx_vol = volume_idx(x + i * g_cube_width, y + j * g_cube_width, z + k * g_cube_width);
        const size_t idx_cube = cube_idx(x, y, z);
        volume[idx_vol] = block[idx_cube];
    }
}

typedef struct dump_block_args {
    size_t i;
    size_t j;
    size_t k;
    size_t t;
} dump_block_args_t;

int make_dump_block_args(dump_block_args_t** args, size_t i, size_t j, size_t k, size_t t){
    if((*args = (dump_block_args_t*) malloc(sizeof(dump_block_args_t))) == NULL){
        return 1;
    }
    **args = (dump_block_args_t) {i, j, k, t};
    return 0;
}


void dump_block_kernel(void *descr[], void *cl_args){
    const dump_block_args_t* args = (dump_block_args_t*) cl_args;
    const FP* block = (FP*) STARPU_BLOCK_GET_PTR(descr[0]);

    const size_t nx = STARPU_BLOCK_GET_NX(descr[0]);
    const size_t ny = STARPU_BLOCK_GET_NY(descr[0]);
    const size_t nz = STARPU_BLOCK_GET_NZ(descr[0]);

    const int worker_id = starpu_worker_get_id();
    // maybe validation here
    FILE* worker_file = g_worker_files[worker_id];

    fwrite(args, sizeof(dump_block_args_t), 1, worker_file);
    fwrite(block, sizeof(FP), nx * ny * nz, worker_file);
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


#ifdef RELEASE
// if x fails (!= 0), exit the program;
#define TRY(x,...) TRYTO(x, program_status = EXIT_FAILURE, program_end)
#else
// if x fails (!= 0), goto the end of main and log status;
#define TRY(x,...) TRYTO(x, program_status = EXIT_FAILURE; \
    printf("[error] Failed at line %d\n", __LINE__); \
    printf("[err-msg] " __VA_ARGS__);, program_end)
#endif


int main(int argc, char **argv){
    //need to be toplevel for the try macro
    int program_status = EXIT_SUCCESS;
    vector(void*) starpu_allocations = NULL;
    vector(void*) allocs = NULL;
    vector(void*) medium_allocs = NULL;

    enum Form form = 0;
    char* form_str = NULL;
    uint32_t nx, ny, nz, absorb_width;
    FP dx, dy, dz, dt, tmax;

    // NOTE: set a file for each worker so there's no race condition
    for(size_t wf = 0; wf < MAXFILES; wf++){
        g_worker_files[wf] = NULL;
    }

	int ret = starpu_init(NULL);
	STARPU_CHECK_RETURN_VALUE(ret, "starpu_init");

    int err = 0;
    TRY(err = read_args(argc, argv, 11, 
        ARG_str, &form_str, 
        ARG_u32, &nx, ARG_u32, &ny, ARG_u32, &nz, ARG_u32, &absorb_width, 
        FP_ARG, &dx, FP_ARG, &dy, FP_ARG, &dz, FP_ARG, &dt, FP_ARG, &tmax,
        ARG_u64, &g_width_in_cubes 
    ), "Error %s in parsing arg at %d\n.", get_parse_errors_name(err), get_parse_errors_local(err));

    TRY(str_to_medium(form_str, &form), "Failed at string to medium conversion");
    
    //fazendo dessa forma para ficar igual ao fletecher base
    g_volume_width = nx + 2 * absorb_width + 2 * BORDER_WIDTH;

    // the number of segments divides the total volume
    TRY(g_volume_width % g_width_in_cubes, 
        "A largura do volume + kernel size devem ser divisíveis pela largura do segmento.\n");

	g_cube_width = g_volume_width / g_width_in_cubes;
 
    const int64_t st = (int64_t) FP_CEIL(tmax / dt);

    // NOTE: code for initializing the files for each thread.
    char file_name[64];
    for(size_t worker_id = 0; worker_id < starpu_worker_get_count(); worker_id++){
        switch (starpu_worker_get_type(worker_id)){
        case STARPU_CPU_WORKER:
            TRY(snprintf(file_name, 63, "%s/%s-worker-out-%ld.bin", out_folder, form_str, worker_id) < 0 ? 1 : 0);
            if((g_worker_files[worker_id] = fopen(file_name, "wb")) == NULL){
                goto program_end;
            }
            break;
        default: assert(0 && "No non-CPU worker should be available at this moment.");
        }
    }

    // allocate the buffers that will be used in computing the 
    // intermediary values
    FP *vpz, *vsv, *epsilon, *delta, *phi, *theta;
    const size_t medium_size = sizeof(FP) * CUBE(g_volume_width);

    TRY(allocate(medium_allocs, (void**) &vpz, medium_size));
    TRY(allocate(medium_allocs, (void**) &vsv, medium_size));
    TRY(allocate(medium_allocs, (void**) &epsilon, medium_size));
    TRY(allocate(medium_allocs, (void**) &delta, medium_size));
    TRY(allocate(medium_allocs, (void**) &phi, medium_size));
    TRY(allocate(medium_allocs, (void**) &theta, medium_size));

    // inicialize the buffers above based on the type of medium
    medium_initialize(form, CUBE(g_volume_width), vpz, vsv, epsilon, delta, phi, theta);
    
    // set the absorption zone for vpz and vsv
    medium_random_velocity_boundary(BORDER_WIDTH, absorb_width, vpz, vsv);


    FP **ch1dxx, **ch1dyy, **ch1dzz, **ch1dxy, **ch1dyz, **ch1dxz, **v2px, **v2pz, **v2sz, **v2pn;
    #define ALLOCATE_PRECOMP_VALUES(v) \
        TRY(allocate(allocs, (void**) &v, TOTAL_CUBES * sizeof(FP*))); \
        for(size_t i = 0; i < TOTAL_CUBES; i++) \
            TRY(allocate_starpu(starpu_allocations, (void**)(v + i), CUBE_SIZE * sizeof(FP)));

    ALLOCATE_PRECOMP_VALUES(ch1dxx);
    ALLOCATE_PRECOMP_VALUES(ch1dyy);
    ALLOCATE_PRECOMP_VALUES(ch1dzz);
    ALLOCATE_PRECOMP_VALUES(ch1dxy);
    ALLOCATE_PRECOMP_VALUES(ch1dyz);
    ALLOCATE_PRECOMP_VALUES(ch1dxz);
    ALLOCATE_PRECOMP_VALUES(v2px);
    ALLOCATE_PRECOMP_VALUES(v2pz);
    ALLOCATE_PRECOMP_VALUES(v2sz);
    ALLOCATE_PRECOMP_VALUES(v2pn);


    medium_calc_intermediary_values(
        vpz, vsv, epsilon, delta, phi, theta,
        ch1dxx, ch1dyy, ch1dzz, ch1dxy, ch1dyz, ch1dxz, 
        v2px, v2pz, v2sz, v2pn
    );

    //at this point the values for the medium will not be used again
    vector_free_all(medium_allocs, free);

    // alocate the initial values for the waves pp, pc, qp, qc.
    // the null_block holds all zeros, which all blocks in pp and qp are
    // the propagation_block holds the point in which the propagation is set
    // therefore all data handles set are referêncing the null_block or the progration_block
    // which is only referênced once.
    FP *null_block, *propagation_block; 

    TRY(allocate_starpu(starpu_allocations, (void**) &null_block, CUBE_SIZE * sizeof(FP)));
    TRY(allocate_starpu(starpu_allocations, (void**) &propagation_block, CUBE_SIZE * sizeof(FP)));

    for(size_t b_i = 0; b_i < CUBE_SIZE; b_i++){
        null_block[b_i] = propagation_block[b_i] = FP_LIT(0.0);
    }
    // iSource in the cube
    const size_t perturbation_source_pos  = cube_idx(g_cube_width / 2, g_cube_width / 2, g_cube_width / 2);
    //insert in this block the propagration source
    const FP starting_source_value = medium_source_value(dt, 0);
    propagation_block[perturbation_source_pos] = starting_source_value;

    // a iteração do bloco t depende dos blocos t - 1 e t - 2.
    // aloca-se mais data_handles que necessário, compreendendo 0..g_width_in_cubes + 2
    // isso evita checks de bounds, pois os cubos internos tem uma borda que evita acessar fora deles no limite do volume
    // esses buffers extras exitem para inserir buffers válidos na ordem certa, mas eles nunca são acessados
    // na hora de fazer o `starpu_block_data_register` e `starpu_data_unregister_submit`, evita os blocos de borda
    // dessa forma, dentro do loop de execução de taregas i - 1 ou i + 1 são sempre índices válidos na lista de `data_handle_t`.
    starpu_data_handle_t* p_wave_iter[3];
    TRY(allocate(allocs, (void**) &p_wave_iter[0], CUBE(g_width_in_cubes + 2) * sizeof(starpu_data_handle_t)));
    TRY(allocate(allocs, (void**) &p_wave_iter[1], CUBE(g_width_in_cubes + 2) * sizeof(starpu_data_handle_t)));
    TRY(allocate(allocs, (void**) &p_wave_iter[2], CUBE(g_width_in_cubes + 2) * sizeof(starpu_data_handle_t)));

    starpu_data_handle_t* q_wave_iter[3];
    TRY(allocate(allocs, (void**) &q_wave_iter[0], CUBE(g_width_in_cubes + 2) * sizeof(starpu_data_handle_t)));
    TRY(allocate(allocs, (void**) &q_wave_iter[1], CUBE(g_width_in_cubes + 2) * sizeof(starpu_data_handle_t)));
    TRY(allocate(allocs, (void**) &q_wave_iter[2], CUBE(g_width_in_cubes + 2) * sizeof(starpu_data_handle_t)));

    starpu_data_handle_t *hdl_ch1dxx, *hdl_ch1dyy, *hdl_ch1dzz, 
        *hdl_ch1dxy, *hdl_ch1dyz, *hdl_ch1dxz, 
        *hdl_v2px, *hdl_v2pz, *hdl_v2sz, *hdl_v2pn;
    TRY(allocate(allocs, (void**) &hdl_ch1dxx, CUBE(g_width_in_cubes) * sizeof(starpu_data_handle_t)));
    TRY(allocate(allocs, (void**) &hdl_ch1dyy, CUBE(g_width_in_cubes) * sizeof(starpu_data_handle_t)));
    TRY(allocate(allocs, (void**) &hdl_ch1dzz, CUBE(g_width_in_cubes) * sizeof(starpu_data_handle_t)));
    TRY(allocate(allocs, (void**) &hdl_ch1dxy, CUBE(g_width_in_cubes) * sizeof(starpu_data_handle_t)));
    TRY(allocate(allocs, (void**) &hdl_ch1dyz, CUBE(g_width_in_cubes) * sizeof(starpu_data_handle_t)));
    TRY(allocate(allocs, (void**) &hdl_ch1dxz, CUBE(g_width_in_cubes) * sizeof(starpu_data_handle_t)));
    TRY(allocate(allocs, (void**) &hdl_v2px,   CUBE(g_width_in_cubes) * sizeof(starpu_data_handle_t)));
    TRY(allocate(allocs, (void**) &hdl_v2pz,   CUBE(g_width_in_cubes) * sizeof(starpu_data_handle_t)));
    TRY(allocate(allocs, (void**) &hdl_v2sz,   CUBE(g_width_in_cubes) * sizeof(starpu_data_handle_t)));
    TRY(allocate(allocs, (void**) &hdl_v2pn,   CUBE(g_width_in_cubes) * sizeof(starpu_data_handle_t)));


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

    const size_t perturbation_source_cube = block_idx(
        (g_width_in_cubes + 2) / 2, (g_width_in_cubes + 2) / 2, (g_width_in_cubes + 2) / 2);

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

    for(int64_t t = 1; t < st; t++){
        starpu_iteration_push(t);
        for(size_t k = 1; k < g_width_in_cubes + 1; k++)
        for(size_t j = 1; j < g_width_in_cubes + 1; j++)
        for(size_t i = 1; i < g_width_in_cubes + 1; i++){
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
            task->name = "wave propagation";
            task->cl = &rtm_codelet;

            const size_t start_z = k == 1 ? BORDER_WIDTH : 0;
            const size_t end_z = g_cube_width - (k == g_width_in_cubes ? BORDER_WIDTH : 0);
            const size_t start_y = j == 1 ? BORDER_WIDTH : 0;
            const size_t end_y = g_cube_width - (j == g_width_in_cubes ? BORDER_WIDTH : 0);
            const size_t start_x = i == 1 ? BORDER_WIDTH : 0;
            const size_t end_x = g_cube_width - (i == g_width_in_cubes ? BORDER_WIDTH : 0);
            
            struct rtm_args* rtm_args;
            TRY(make_rtm_args(&rtm_args, 
                start_x, end_x,
                start_y, end_y,
                start_z, end_z,
                dx, dy, dz, dt
            ));

            //sprintf(cl_args->name, "[%d, %d, %d, %ld]", i, j, k, t + 1);
            //task->name = cl_args->name;

            task->cl_arg = rtm_args;
            task->cl_arg_size = sizeof(struct rtm_args);
            task->cl_arg_free = 1; // free the args after use

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
        // task that insert the perturbation on the handle
        struct starpu_task* perturb_task = starpu_task_create();
        perturb_task->name = "wave insertion";

        const FP source_value = medium_source_value(dt, t);
        perturb_args_t* perturb_args;
        TRY(make_perturb_args(&perturb_args, perturbation_source_pos, source_value));

        perturb_task->cl = &insert_perturbation_codelet;
        perturb_task->cl_arg = perturb_args;
        perturb_task->cl_arg_size = sizeof(perturb_args_t);
        perturb_task->cl_arg_free = 1; // free the args after use

        perturb_task->handles[0] = p_wave_iter[0][perturbation_source_cube];
        perturb_task->handles[1] = q_wave_iter[0][perturbation_source_cube];

        TRY(starpu_task_submit(perturb_task));

        for(size_t k = 1; k < g_width_in_cubes + 1; k++)
        for(size_t j = 1; j < g_width_in_cubes + 1; j++)
        for(size_t i = 1; i < g_width_in_cubes + 1; i++){
            // call the write task
            dump_block_args_t* dump_args;
            TRY(make_dump_block_args(&dump_args, i - 1, j - 1, k - 1, t - 1));

            struct starpu_task* dump_block_task = starpu_task_create();
            dump_block_task->name = "write block";
            dump_block_task->cl = &dump_block_codelet;
            dump_block_task->cl_arg = dump_args;
            dump_block_task->cl_arg_size = sizeof(dump_block_args_t);
            dump_block_task->cl_arg_free = 1;

            dump_block_task->handles[0] = p_wave_iter[2][block_idx(i, j, k)];

            TRY(starpu_task_submit(dump_block_task));

            // only start to unregister when the automatically allocated buffers reaches the t - 2.
            //unregister all cubes from iteration t - 2
            //in the inner data_handles
            if(t >= 2){
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
    printf("Submitted all tasks\n");
    //at least after all iterations
    starpu_task_wait_for_all();

    //FP* result_block, *result_volume;
    //TRY(allocate(allocs,(void**) &result_block, sizeof(FP) * CUBE_SIZE));
    //TRY(allocate(allocs,(void**) &result_volume, sizeof(FP) * CUBE(g_volume_width)));

    //clear_block(result_block, g_cube_width);
    //clear_block(result_volume, g_volume_width);

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
    char filename[256];

    sprintf(filename, "%s/out-%s.rsf", out_folder, form_str);
    if((rsf_header = fopen(filename, "w+")) == NULL){
        goto program_end;
    }

    fprintf(rsf_header,"in=\"./%s@\"\n", filename);
    fprintf(rsf_header,"data_format=\"native_float\"\n");
    fprintf(rsf_header,"esize=%lu\n", sizeof(float)); 
    fprintf(rsf_header,"n1=%ld\n", g_volume_width /*sx*/);
    fprintf(rsf_header,"n2=%ld\n", g_volume_width /*sy*/);
    fprintf(rsf_header,"n3=%ld\n", g_volume_width /*sz*/);
    fprintf(rsf_header,"n4=%ld\n", st - 1); // TODO: validar quantas iterações são realmente salvas
    fprintf(rsf_header,"d1=%f\n",dx);
    fprintf(rsf_header,"d2=%f\n",dy);
    fprintf(rsf_header,"d3=%f\n",dz);
    fprintf(rsf_header,"d4=%f\n",dt);
    fprintf(rsf_header,"seg=%ld\n", g_width_in_cubes);

    fclose(rsf_header);


    program_end:

    for(size_t wf = 0; wf < MAXFILES; wf++){
        if(g_worker_files[wf] != NULL){
            fclose(g_worker_files[wf]);
        } 
    }

    vector_free_all(medium_allocs, free);
    vector_free_all(allocs, free);
    #define STARPU_FREE(x) starpu_free_noflag(x, CUBE_SIZE * sizeof(FP));
    vector_free_all(starpu_allocations, STARPU_FREE);

	starpu_shutdown();
	return program_status;
}

