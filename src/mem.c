#include "mem.h"

#include <starpu.h>
#include <stdlib.h>
#include <errno.h>


err_t mem_allocate(vector(void*) v, void** ptr, const size_t size){
    if((*ptr = malloc(size)) == NULL){
        return errno;
    }
    vector_push(v, *ptr);
    return 0;
}

err_t mem_allocate_starpu(vector(void*) v, void** ptr, const size_t size){
    err_t err;
    if((err = starpu_malloc(ptr, size)) != 0){
        return err;
    }
    vector_push(v, *ptr);
    return 0;
}