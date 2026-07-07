#include "mem.h"

#include <starpu.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>



struct mem_stat {
	void* ptr;
	size_t size;
};


err_t mem_allocate(mem_vec_t v, void** ptr, const size_t size){

    err_t err;
    if((err = starpu_malloc(ptr, size)) != 0){
        return err;
    }

    struct mem_stat stat = {
        .ptr = *ptr, 
        .size = size
    };

    vector_push(v, stat);
    return 0;
}

void mem_free(mem_vec_t v){
    #define FREE(x) starpu_free_noflag(x.ptr, x.size);
    vector_free_all(v, FREE);
    #undef FREE
}

