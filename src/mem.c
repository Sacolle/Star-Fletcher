#include "mem.h"

#include <starpu.h>
#include <stdlib.h>

// idk se esse é o melhor jeito,
// nesse instante acho que só falta uma 
// estrutura singleton para juntar todos os globais do programa
extern uint32_t g_mem_aligment;

int mem_allocate(vector(void*) v, void** ptr, const size_t size){
    // https://man7.org/linux/man-pages/man3/posix_memalign.3.html
    if(posix_memalign(ptr, g_mem_aligment, size)){
        return 1;
    }
    vector_push(v, *ptr);
    return 0;
}

int mem_allocate_starpu(vector(void*) v, void** ptr, const size_t size){
    if(starpu_malloc(ptr, size) != 0){
        return 1;
    }
    vector_push(v, *ptr);
    return 0;
}