#ifndef _MEM_GUARD_
#define _MEM_GUARD_

#include <stdbool.h>

#include "vector.h"
#include "err.h"

struct mem_stat;

typedef vector(struct mem_stat) mem_vec_t;

#define PIN_ME true
#define NOT_PIN_ME false
#define STARPU_ALLOCATION true
#define CPU_ALLOCATION false

err_t mem_allocate(mem_vec_t v, void** ptr, const size_t size, const bool pin, const bool is_starpus);    
err_t mem_free(mem_vec_t v);

#endif
