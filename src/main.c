#include <starpu.h>
#include <pthread.h>
#include <stdint.h>

#include "kernel.h"
#include "macros.h"
#include "floatingpoint.h"
#include "argparse.h"
#include "medium.h"

#include "vector.h"


const uint64_t BORDER_WIDTH = 4;


// width for the volume of the whole sistem
size_t g_volume_width = 0;
// amount of segments in a dimension. 
// if g_volume_width = 100 and g_width_in_cubes = 10
// there are 10 segmentations by dimension, resulting in 1000 total cubes of 
// volume 1000
size_t g_width_in_cubes = 0;


// width for the segmented cube
size_t g_cube_width = 0;


#define CUBE_SIZE (g_cube_width * g_cube_width * g_cube_width)
#define TOTAL_CUBES (g_width_in_cubes * g_width_in_cubes * g_width_in_cubes)


void print_block(double* block){
    printf("[\n");
    for(size_t z = 0; z < g_cube_width; z++){
        printf("[\n");
        for(size_t y = 0; y < g_cube_width; y++){
            printf("\t[");
            for(size_t x = 0; x < g_cube_width; x++){
                printf("%.2f", block[CUBE_I(x, y, z)]);
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

//initialize block, the border consists of value 0.0
/*
void initialize_block(double* block, int i, int j, int k){
    for(size_t z = 0; z < g_cube_width; z++){
        for(size_t y = 0; y < g_cube_width; y++){
            for(size_t x = 0; x < g_cube_width; x++){
                //if it starts or ends in an edge, set to 0
                if( INEDGE(k, z) || INEDGE(j, y) || INEDGE(i, x)){
                    block[CUBE_I(x, y, z)] = 0.0;
                }else{
                    block[CUBE_I(x, y, z)] = 10.0 * ((double) rand() / RAND_MAX);
                }
            }
        }
    }
}*/

void clear_block(double* block){
    for(size_t z = 0; z < g_cube_width; z++){
        for(size_t y = 0; y < g_cube_width; y++){
            for(size_t x = 0; x < g_cube_width; x++){
                block[CUBE_I(x, y, z)] = 0.0;
            }
        }
    }
}

//TODO: 
void clear_pointers(starpu_data_handle_t* list){
    for(size_t i = 0; i < CUBE(g_width_in_cubes); i++){
        list[i] = NULL;
    }
}

#define EPSILON 0.0001
//TODO: mover para o sistema de testes
void assert_block_clear_edge(double* block, size_t i, size_t j, size_t k){
    for(size_t z = 0; z < g_cube_width; z++){
        for(size_t y = 0; y < g_cube_width; y++){
            for(size_t x = 0; x < g_cube_width; x++){
                const size_t idx = CUBE_I(x, y, z);
                if( INEDGE(k, z) || INEDGE(j, y) || INEDGE(i, x)){
                    assert(block[idx] < EPSILON);
                    assert(block[idx] > -EPSILON);
                }
            }
        }
    }
}


struct cl_args {
    char name[64];
    uint32_t i;
    uint32_t j;
    uint32_t k;
    uint32_t t;
};

struct cl_args* make_cl_args(size_t i, size_t j, size_t k, size_t t){
    struct cl_args* cl_args = (struct cl_args*) malloc(sizeof(struct cl_args));
    if(cl_args == NULL){
        return NULL;
    }
    cl_args->i = i;
    cl_args->j = j;
    cl_args->k = k;
    cl_args->t = t;

    return cl_args;
}

int allocate(vector(void*) v, void** ptr, size_t size){
    if((*ptr = malloc(size)) == NULL){
        return 1;
    }
    vector_push(v, *ptr);
    return 0;
}

int allocate_starpu(vector(void*) v, void** ptr, size_t size){
    if(starpu_malloc(ptr, size) != 0){
        return 1;
    }
    vector_push(v, *ptr);
    return 0;
}

int allocate_multi_starpu(vector(void*) v, void** buffers, size_t amount, size_t size){
    for(size_t i = 0; i < amount; i++){
        if(allocate_starpu(v, buffers + i, size) != 0) return 1;
    }
    return 0;
}

/*
//TODO: make test for these functions
void multi_free(void** buffers, size_t amount){
    for(size_t i = 0; i < amount; i++){
        free(buffers[i]);
    }
}

int multi_malloc(void** buffers, size_t amount, size_t size){
    for(size_t i = 0; i < amount; i++){
        if((buffers[i] = malloc(size)) == NULL){
            multi_free(buffers, i);
            return 1;
        }
    }
    return 0;
}

void multi_starpu_free(void** buffers, size_t amount, size_t size){
    for(size_t i = 0; i < amount; i++){
        printf("Erro em alocar os blocos em malloc!\n");
        starpu_free_noflag(buffers[i], size);
    }
}

//TODO: validar que starpu_malloc retorna só 0 em sucesso
int multi_starpu_malloc(void** buffers, size_t amount, size_t size){
    for(size_t i = 0; i < amount; i++){
        if(starpu_malloc(buffers[i], size) != 0){
            multi_starpu_free(buffers, i, size);
            return 1;
        }
    }
    return 0;
}

void two_layer_multi_free(void*** buffers, size_t amount_l1, size_t amount_l2, size_t size){
    for(size_t err_i; err_i < amount_l1; err_i++){
        multi_starpu_free(buffers[err_i], amount_l2, size);
    }
    multi_free((void**) buffers, amount_l1);
}

// essa função é horrenda, depois quando for usar uma estrutura de dados para gerir as alocações é melhor mudar
int two_layer_multi_malloc(void*** buffers, size_t amount_l1, size_t amount_l2, size_t size){
    size_t i = 0;
    int err = 0;
    for(i = 0; i < amount_l1; i++){
        if((buffers[i] = malloc(sizeof(void*) * amount_l2)) == NULL){
            err = 1;
            break;
        }
        if((err = multi_starpu_malloc(buffers[i], amount_l2, size)) != 0){
            break;
        }
    }

    if(err){
        two_layer_multi_free(buffers, amount_l1, amount_l2, size);
        return 1;
    }
    return 0;
}
*/

struct starpu_codelet avrg_filter_cl = {
    .cpu_funcs = { rtm_kernel },
    .nbuffers = 8,
    .modes = {
        STARPU_W, // write cell at CUBE_I(x, y, z) at tsizeof
        STARPU_R, // read cell at CUBE_I(x, y, z) at t - 1
        STARPU_R, // read cell at CUBE_I(x, y, z - 1) at t - 1
        STARPU_R, // read cell at CUBE_I(x, y, z + 1) at t - 1
        STARPU_R, // read cell at CUBE_I(x, y - 1, z) at t - 1
        STARPU_R, // read cell at CUBE_I(x, y + 1, z) at t - 1
        STARPU_R, // read cell at CUBE_I(x - 1, y, z) at t - 1
        STARPU_R  // read cell at CUBE_I(x + 1, y, z) at t - 1
    },
    .model = &starpu_perfmodel_nop,
};


int main(int argc, char **argv){
    enum Form form = 0;
    char* form_str = NULL;
    uint32_t nx, ny, nz, absorb_width;
    FP dx, dy, dz, dt, tmax;

    int err = 0;
    if((err = read_args(argc, argv, 11, 
        ARG_str, &form_str, 
        ARG_u32, &nx, ARG_u32, &ny, ARG_u32, &nz, ARG_u32, &absorb_width, 
        FP_ARG, &dx, FP_ARG, &dy, FP_ARG, &dz, FP_ARG, &dt, FP_ARG, &tmax,
        ARG_u64, &g_width_in_cubes 
    )) != 0){
        printf("Error %s in parsing arg at %d\n.", get_parse_errors_name(err), get_parse_errors_local(err));
        return EXIT_FAILURE;
    }

    if((form = str_to_medium(form_str)) < 0){
        printf("Error in parsing %s as a medium.\n", form_str);
        return EXIT_FAILURE;
    }
    //fazendo dessa forma para ficar igual ao fletecher base
    g_volume_width = nx + 2 * absorb_width + 2 * BORDER_WIDTH;

    // the number of segments divides the total volume
    assert((g_volume_width % g_width_in_cubes == 0) && 
        "A largura do volume + kernel size devem ser divisíveis pela largura do segmento.\n");

	g_cube_width = g_volume_width / g_width_in_cubes;
    assert(g_cube_width && g_width_in_cubes && g_volume_width);
 
    const int64_t st = (int64_t) FP_CEIL(tmax / dt);

    // iSource
    const size_t perturbation_source_cube = block_idx(g_width_in_cubes / 2, g_width_in_cubes / 2, g_width_in_cubes / 2);
    const size_t perturbation_source_pos  = cube_idx(g_cube_width / 2, g_cube_width / 2, g_cube_width / 2);

	int ret = starpu_init(NULL);
	STARPU_CHECK_RETURN_VALUE(ret, "starpu_init");

    vector(void*) starpu_allocations;
    vector(void*) allocs;
    vector(void*) medium_allocs;

    // if x fails (!= 0), exit the program;
    #define TRY(x) (x) != 0 ? (printf("[error] Failed at line %d: "# x , __LINE__); goto exit) : 0

    // allocate the buffers that will be used in computing the 
    // intermediary values
    FP *vpz, *vsv, *epsilon, *delta, *phi, *theta;
    const size_t medium_size = sizeof(FP) * CUBE(g_volume_width);

    TRY(allocate(medium_allocs, &vpz, medium_size));
    TRY(allocate(medium_allocs, &vsv, medium_size));
    TRY(allocate(medium_allocs, &epsilon, medium_size));
    TRY(allocate(medium_allocs, &delta, medium_size));
    TRY(allocate(medium_allocs, &phi, medium_size));
    TRY(allocate(medium_allocs, &theta, medium_size));

    // inicialize the buffers above based on the type of medium
    medium_initialize(form, CUBE(g_volume_width), vpz, vsv, epsilon, delta, phi, theta);
    
    // set the absorption zone for vpz and vsv
    medium_random_velocity_boundary(BORDER_WIDTH, absorb_width, vpz, vsv);


    FP **ch1dxx, **ch1dyy, **ch1dzz, **ch1dxy, **ch1dyz, **ch1dxz, **v2px, **v2pz, **v2sz, **v2pn;
    #define ALLOCATE_PRECOMP_VALUES(v) \
        TRY(allocate(allocs, (void**) &v, TOTAL_CUBES * sizeof(FP*))); \
        TRY(allocate_multi_starpu(starpu_allocations, v, TOTAL_CUBES, CUBE_SIZE * sizeof(FP)))

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

    TRY(allocate_starpu(starpu_allocations, &null_block, CUBE_SIZE * sizeof(FP)));
    TRY(allocate_starpu(starpu_allocations, &propagation_block, CUBE_SIZE * sizeof(FP)));

    for(size_t b_i = 0; b_i < CUBE_SIZE; b_i++){
        null_block[b_i] = propagation_block[b_i] = FP_LIT(0.0);
    }
    //TODO: insert the propagation source in the propagation block


    // a iteração do bloco t depende dos blocos t - 1 e t - 2.
    starpu_data_handle_t* iterations[3];
    TRY(allocate(allocs, &iterations[0], TOTAL_CUBES * sizeof(starpu_data_handle_t)));
    TRY(allocate(allocs, &iterations[1], TOTAL_CUBES * sizeof(starpu_data_handle_t)));
    TRY(allocate(allocs, &iterations[2], TOTAL_CUBES * sizeof(starpu_data_handle_t)));


    //starpu_data_handle_t* prev = (starpu_data_handle_t*) malloc(sizeof(starpu_data_handle_t) * CUBE(g_width_in_cubes));
/*
    for(int k = 0; k < g_width_in_cubes; k++){
        for(int j = 0; j < g_width_in_cubes; j++){
            for(int i = 0; i < g_width_in_cubes; i++){

                double* allocated_block = (double*) malloc(sizeof(double) * CUBE(g_cube_width));
                initialize_block(allocated_block, i, j, k);
                initial_values[BLOCK_I(i, j, k)] = allocated_block;

                // let starpu allocate the data by setting home_node = -1 
                // problem: the data is only alocated when writen with STARPU_W
                // wich will only happen in the codelet, (i think)
                // so the first iterations need to allocate the memory manually
                starpu_block_data_register(&prev[BLOCK_I(i,j,k)], STARPU_MAIN_RAM, (uintptr_t) allocated_block, 
                    //stride for y  stride for z
                    g_cube_width, SQUARE(g_cube_width),
                    // width         height        depth
                    g_cube_width, g_cube_width, g_cube_width, 
                    sizeof(double)
                );
                // assert that its properly registered
                assert(prev[BLOCK_I(i,j,k)] != NULL);
            }
        }
    }

    starpu_data_handle_t* curr = (starpu_data_handle_t*) malloc(sizeof(starpu_data_handle_t) * CUBE(g_width_in_cubes));
    clear_pointers(curr);

    for(int64_t t = 0; t < st; t++){
        starpu_iteration_push(t);
        for(int k = 0; k < g_width_in_cubes; k++){
            for(int j = 0; j < g_width_in_cubes; j++){
                for(int i = 0; i < g_width_in_cubes; i++){

                    const int idx = BLOCK_I(i, j, k);
                    //add to the curr buff
                    //let starpu allocate the data by setting home_node = -1 
                    starpu_block_data_register(&curr[idx], -1, 0,
                        g_cube_width, SQUARE(g_cube_width),
                        g_cube_width, g_cube_width, g_cube_width, 
                        sizeof(double)
                    );

                    // 
                    struct starpu_task* task = starpu_task_create();

                    task->cl = &avrg_filter_cl;
                    
                    struct cl_args* cl_args = make_cl_args(i, j, k, t + 1);
                    assert(cl_args != NULL);

                    sprintf(cl_args->name, "[%d, %d, %d, %ld]", i, j, k, t + 1);
                    task->name = cl_args->name;

                    task->cl_arg = cl_args;
                    task->cl_arg_size = sizeof(struct cl_args);
                    task->cl_arg_free = 1; // free the args after use


                    //select the handles
                    //need to stablish an order for selection, em 3d eg dificil
                    //          ^   ^
                    //          |  /
                    //          y z
                    //          |/
                    // -- x -- >
                    // ordem ao invés de rotacional vai ser via eixo,
                    // blocos do z (-1, +1), depois do y, depois do x
                    // no caso primeiro vai o bloco curr, 
                    // depois o central prev e dps o resto na ordem estabelecida

                    // buffers iniciais
                    task->handles[0] = curr[idx]; 
                    task->handles[1] = prev[idx];
                    // se for o primeiro bloco da direção k, 
                    // não usará este handle, mas tem que passar um buffer válido, então passa o central
                    // se não for o primeiro bloco, passa o bloco anterior a esse, que será acessado
                    task->handles[2] = FSTBLK(k) ? prev[idx] : prev[BLOCK_I(i, j, k - 1)];
                    // repete só que caso haja o último bloco, e se existir passa o seguite a esse
                    task->handles[3] = LSTBLK(k) ? prev[idx] : prev[BLOCK_I(i, j, k + 1)];
                    // primeiro e último para eixo y
                    task->handles[4] = FSTBLK(j) ? prev[idx] : prev[BLOCK_I(i, j - 1, k)];
                    task->handles[5] = LSTBLK(j) ? prev[idx] : prev[BLOCK_I(i, j + 1, k)];
                    // primeiro e último para eixo x
                    task->handles[6] = FSTBLK(i) ? prev[idx] : prev[BLOCK_I(i - 1, j, k)];
                    task->handles[7] = LSTBLK(i) ? prev[idx] : prev[BLOCK_I(i + 1, j, k)];

                    ret = starpu_task_submit(task);
                    //printf("iter: %d\n, %s", ++iter,t->name);
                    STARPU_CHECK_RETURN_VALUE(ret, "starpu_task_submit");
                }
            }
        }
        for(int ta = 0; ta < CUBE(g_width_in_cubes); ta++){
            starpu_data_unregister_submit(prev[ta]);
        }
        //do a swap
        starpu_data_handle_t* tmp = prev;
        prev = curr;
        curr = tmp;
        //write on a clear curr
        clear_pointers(curr);
        starpu_iteration_pop();
    }
    printf("Submitted all tasks\n");
    //at least after all iterations
    starpu_task_wait_for_all();

    //TODO: explicar
    starpu_data_handle_t* result = prev;

    //output the results in curr
    //desregistra os blocos e limpa as tasks
    double* result_block = (double*) malloc(sizeof(double) * CUBE(g_cube_width));
    clear_block(result_block);

    for(int k = 0; k < g_width_in_cubes; k++){
        for(int j = 0; j < g_width_in_cubes; j++){
            for(int i = 0; i < g_width_in_cubes; i++){
                starpu_data_handle_t handle = result[BLOCK_I(i,j,k)];

                // first need to acquire the data
                starpu_data_acquire(handle, STARPU_R);
                //double* result_block = initial_values[BLOCK_I(i,j,k)];
                starpu_ssize_t og_size = sizeof(double) * CUBE(g_cube_width);
                starpu_ssize_t size = og_size;
                int ret = starpu_data_pack(handle, (void**)&result_block, &size);
                assert(og_size == size);
                STARPU_CHECK_RETURN_VALUE(ret, "starpu_data_pack");

                //print_block(result_block);


                clear_block(result_block);
                //then release
                starpu_data_release(handle);
                starpu_data_unregister(handle);

                free(initial_values[BLOCK_I(i,j,k)]);
            }
        }
    }
    */
	starpu_shutdown();
	return 0;
}