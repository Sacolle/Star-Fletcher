#ifndef _VECTOR_GUARD_
#define _VECTOR_GUARD_

//taken from https://crocidb.com/post/simple-vector-implementation-in-c/

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>

/// Header data for a light-weight implementation of typed vectors.
struct allocations_header
{
    long long capacity;
    long long count;
};

void layec_vector_maybe_expand(void** vector_ref, long long element_size, long long required_count);

#define vector(T) T*
#define vector_get_header(V) (((struct allocations_header*)(V)) - 1)
#define vector_count(V) ((V) ? vector_get_header(V)->count : 0)
#define vector_push(V, E) do { layec_vector_maybe_expand((void**)&(V), (long long)sizeof *(V), vector_count(V) + 1); (V)[vector_count(V)] = E; vector_get_header(V)->count++; } while (0)
#define vector_pop(V) do { if (vector_get_header(V)->count) vector_get_header(V)->count--; } while (0)
#define vector_free(V) do { if (V) { memset(V, 0, (unsigned long long)vector_count(V) * (sizeof *(V))); free(vector_get_header(V)); (V) = NULL; } } while (0)
#define vector_free_all(V, F) do { if (V) { for (long long vector_index = 0; vector_index < vector_count(V); vector_index++) F((V)[vector_index]); memset(V, 0, (unsigned long long)vector_count(V) * (sizeof *(V))); free(vector_get_header(V)); (V) = NULL; } } while (0)



#endif