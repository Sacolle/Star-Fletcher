#ifndef _MEM_GUARD_
#define _MEM_GUARD_

#include <stdbool.h>

#include "vector.h"
#include "err.h"

struct mem_stat;

typedef vector(struct mem_stat) mem_vec_t;

err_t mem_allocate(mem_vec_t *v, void** ptr, const size_t size);    
void mem_free(mem_vec_t *v);

err_t mem_allocate_local(mem_vec_t *v, void** ptr, const size_t size);    
void mem_free_local(mem_vec_t *v);

#endif
