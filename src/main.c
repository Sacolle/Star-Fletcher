#include <starpu.h>
#include <pthread.h>
#include <stdint.h>

#include "kernel.h"
#include "macros.h"
#include "floatingpoint.h"
#include "argparse.h"
#include "medium.h"


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
//TODO: make test for these functions
void multi_free(void** buffers, size_t amount){
    for(size_t i = 0; i < amount; i++){
        printf("Erro em alocar os blocos em malloc!\n");
        free(buffers[i]);
    }
}

int multi_malloc(void** buffers, size_t amount, size_t size){
    size_t err_i = 0;
    for(size_t i = 0; i < amount; i++){
        if((buffers[i] = malloc(size)) == NULL){
            err_i = i;
            break;
        }
    }
    // remove os buffers até o i que terminou no caso de erro
    // pois se err_i = 2, o 3º elemento deu erro e deve-se desalocar os
    // buffers 0 e 1 que foram alocados antes desse.
    multi_free(buffers, err_i);
    //err_i pode ser 0 e indicar erro, nesse caso buffer[0] será nulo
    return err_i && buffers[0] == NULL;
}

void multi_starpu_free(void** buffers, size_t amount, size_t size){
    for(size_t i = 0; i < amount; i++){
        printf("Erro em alocar os blocos em malloc!\n");
        starpu_free_noflag(buffers[i], size);
    }
}

//TODO: validar que starpu_malloc retorna só 0 em sucesso
int multi_starpu_malloc(void** buffers, size_t amount, size_t size){
    size_t err_i = 0;
    int err = 0;
    for(size_t i = 0; i < amount; i++){
        if((err = starpu_malloc(buffers[i], size)) != 0){
            err_i = i;
            break;
        }
    }
    multi_starpu_free(buffers, err_i, size);
    return err;
}

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

    const size_t volume_in_bytes = CUBE(g_volume_width) * sizeof(FP);
    
    // allocate the buffers that will be used in computing the 
    // intermediary values
    FP *vpz, *vsv, *epsilon, *delta, *phi, *theta;
    FP* medium[6] = { vpz, vsv, epsilon, delta, phi, theta };
    if(multi_malloc((void**) medium, 6, volume_in_bytes) != 0){
        perror("Error in alocating all buffers.\n");
        return EXIT_FAILURE;
    }
    
    // inicialize the buffers above based on the type of medium
    medium_initialize(form, CUBE(g_volume_width), vpz, vsv, epsilon, delta, phi, theta);
    
    // set the absorption zone for vpz and vsv
    medium_random_velocity_boundary(BORDER_WIDTH, absorb_width, vpz, vsv);
    /*
        Alocate the below blocks in cubes 
    // inicialize the wave blocks with zero 
    float *pp=NULL;
    pp = (float *) malloc(sx*sy*sz*sizeof(float)); 
    float *pc=NULL;
    pc = (float *) malloc(sx*sy*sz*sizeof(float)); 
    float *qp=NULL;
    qp = (float *) malloc(sx*sy*sz*sizeof(float)); 
    float *qc=NULL;
    qc = (float *) malloc(sx*sy*sz*sizeof(float)); 
    */

    /*
    // inicialize the wave blocks with zero 
    float *pp=NULL;
    pp = (float *) malloc(sx*sy*sz*sizeof(float)); 
    float *pc=NULL;
    pc = (float *) malloc(sx*sy*sz*sizeof(float)); 
    float *qp=NULL;
    qp = (float *) malloc(sx*sy*sz*sizeof(float)); 
    float *qc=NULL;
    qc = (float *) malloc(sx*sy*sz*sizeof(float)); 
    
    // values that need to be pre computed
    float *ch1dxx = NULL; // isotropy simetry deep angle
    float *ch1dyy = NULL; // isotropy simetry deep angle
    float *ch1dzz = NULL; // isotropy simetry deep angle
    float *ch1dxy = NULL; // isotropy simetry deep angle
    float *ch1dyz = NULL; // isotropy simetry deep angle
    float *ch1dxz = NULL; // isotropy simetry deep angle
    float *v2px = NULL;   // coeficient of H2(p)
    float *v2pz = NULL;   // coeficient of H1(q)
    float *v2sz = NULL;   // coeficient of H1(p-q) and H2(p-q)
    float *v2pn = NULL;   // coeficient of H2(p)

    for (int i = 0; i < sx * sy * sz; i++)
    {
        float sinTheta = sin(theta[i]);
        float cosTheta = cos(theta[i]);
        float sin2Theta = sin(2.0 * theta[i]);
        float sinPhi = sin(phi[i]);
        float cosPhi = cos(phi[i]);
        float sin2Phi = sin(2.0 * phi[i]);
        ch1dxx[i] = sinTheta * sinTheta * cosPhi * cosPhi;
        ch1dyy[i] = sinTheta * sinTheta * sinPhi * sinPhi;
        ch1dzz[i] = cosTheta * cosTheta;
        ch1dxy[i] = sinTheta * sinTheta * sin2Phi;
        ch1dyz[i] = sin2Theta * sinPhi;
        ch1dxz[i] = sin2Theta * cosPhi;
    }

    // coeficients of H1 and H2 at PDEs
    for (int i = 0; i < sx * sy * sz; i++){
        v2sz[i] = vsv[i] * vsv[i];
        v2pz[i] = vpz[i] * vpz[i];
        v2px[i] = v2pz[i] * (1.0 + 2.0 * epsilon[i]);
        v2pn[i] = v2pz[i] * (1.0 + 2.0 * delta[i]);
    }
    */

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
    free(prev);
    free(curr);
	starpu_shutdown();
	return 0;
}