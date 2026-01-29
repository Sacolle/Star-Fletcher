#ifndef _MEM_GUARD_
#define _MEM_GUARD_

#include "vector.h"

int mem_allocate(vector(void*) v, void** ptr, const size_t size);


int mem_allocate_starpu(vector(void*) v, void** ptr, const size_t size);

#endif