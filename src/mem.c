#include "mem.h"

#include <starpu.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>



struct mem_stat {
	void* ptr;
	uint32_t size;
	bool is_pinned;
	bool is_starpus;
};


err_t mem_allocate(mem_vec_t v, void** ptr, const size_t size, const bool pin, const bool is_starpus){
    
    err_t err;
    if(is_starpus){
        if((err = starpu_malloc(ptr, size)) != 0){
            return err;
        }
    }else{
        if((*ptr = malloc(size)) == NULL){
	    return errno;
	}
	if(pin){
	    if((err = starpu_memory_pin(*ptr, size)) != 0){
	   	return err; 
	    }
	}
    }

    struct mem_stat stat = {
        .ptr = *ptr, 
        .size = size, 
        .is_pinned = pin, 
        .is_starpus = is_starpus
    };

    vector_push(v, stat);
    return 0;
}

err_t mem_free(mem_vec_t v){
    #define FREE(x) do{ \
	if(x.is_pinned && !x.is_starpus) { starpu_memory_unpin(x.ptr, x.size); } \
	if(x.is_starpus) { starpu_free_noflag(x.ptr, x.size); }else \
	{ free(x.ptr); }}while(0);

    vector_free_all(v, FREE);
    #undef FREE
}

