#include "mem.h"

#include <starpu.h>

int mem_allocate(vector(void*) v, void** ptr, const size_t size){
    if((*ptr = malloc(size)) == NULL){
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