#ifndef _MEM_GUARD_
#define _MEM_GUARD_

#include "vector.h"
#include "err.h"

err_t mem_allocate(vector(void*) v, void** ptr, const size_t size);
err_t mem_allocate_starpu(vector(void*) v, void** ptr, const size_t size);

#endif