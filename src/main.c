#include <starpu.h>
#include <pthread.h>
#include <stdint.h>

#include "kernel.h"

#include "macros.h"

const int nbufs = STARPU_NMAXBUFS;

const int KERNEL_SIZE = 4;
// number of iterations
static int g_iterations;
// width for the volume of the whole sistem
int g_volume_width;
// amount of segments in a dimension. 
// if g_volume_width = 100 and g_width_in_cubes = 10
// there are 10 segmentations by dimension, resulting in 1000 total cubes of 
// volume 1000
int g_width_in_cubes;
// width for the segmented cube
int g_cube_width;


void print_block(double* block){
    printf("[\n");
    for(int z = 0; z < g_cube_width; z++){
        printf("[\n");
        for(int y = 0; y < g_cube_width; y++){
            printf("\t[");
            for(int x = 0; x < g_cube_width; x++){
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
void initialize_block(double* block, int i, int j, int k){
    for(int z = 0; z < g_cube_width; z++){
        for(int y = 0; y < g_cube_width; y++){
            for(int x = 0; x < g_cube_width; x++){
                //if it starts or ends in an edge, set to 0
                if( INEDGE(k, z) || INEDGE(j, y) || INEDGE(i, x)){
                    block[CUBE_I(x, y, z)] = 0.0;
                }else{
                    block[CUBE_I(x, y, z)] = 10.0 * ((double) rand() / RAND_MAX);
                }
            }
        }
    }
}

void clear_block(double* block){
    for(int z = 0; z < g_cube_width; z++){
        for(int y = 0; y < g_cube_width; y++){
            for(int x = 0; x < g_cube_width; x++){
                block[CUBE_I(x, y, z)] = 0.0;
            }
        }
    }
}

void clear_pointers(starpu_data_handle_t* list){
    for(int i = 0; i < CUBE(g_width_in_cubes); i++){
        list[i] = NULL;
    }
}

#define EPSILON 0.0001

void assert_block_clear_edge(double* block, int i, int j, int k){
    for(int z = 0; z < g_cube_width; z++){
        for(int y = 0; y < g_cube_width; y++){
            for(int x = 0; x < g_cube_width; x++){
                const int idx = CUBE_I(x, y, z);
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

struct cl_args* make_cl_args(uint32_t i, uint32_t j, uint32_t k, uint32_t t){
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

int main(int argc, char **argv){
    //get global parâmeters
	assert(argc >= 4);

	g_iterations = atoi(argv[1]);
    assert(g_iterations > 0);

	g_volume_width = atoi(argv[2]);
	g_width_in_cubes = atoi(argv[3]);
    // the number of segments divides the total volume
    assert((g_volume_width % g_width_in_cubes == 0) && 
        "A largura do volume + kernel size devem ser divisíveis pela largura do segmento.\n");

	g_cube_width = g_volume_width / g_width_in_cubes;

  
    struct starpu_codelet avrg_filter_cl = {
        .cpu_funcs = { rtm_kernel },
        .nbuffers = 8,
        .modes = {
            STARPU_W, // write cell at CUBE_I(x, y, z) at t
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
	int ret = starpu_init(NULL);
	STARPU_CHECK_RETURN_VALUE(ret, "starpu_init");


    // é necessário inicializar duas matrizes iniciais, pois para computar o bloco(x,y,z,t)
    // também é necessário o bloco(x,y,z,t-1)
    // agora ao invés de um blocão, os blocos seram disjuntos
    starpu_data_handle_t* prev = (starpu_data_handle_t*) malloc(sizeof(starpu_data_handle_t) * CUBE(g_width_in_cubes));
    clear_pointers(prev);

    // need this because the starpu automatic alocation happens on demand
    // an if can be used to free this at the end of the code or after the first iteration
    // rn it is left as a TODO: free this sometime
    double* initial_values[CUBE(g_width_in_cubes)];

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

    for(int t = 0; t < g_iterations; t++){
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

                    sprintf(cl_args->name, "[%d, %d, %d, %d]", i, j, k, t + 1);
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
        //TODO: call starpu_unregister_submit aqui para todos os valores em prev
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
    free(prev);
    free(curr);
	starpu_shutdown();
	return 0;
}