#ifndef _VECTOR_GUARD_
#define _VECTOR_GUARD_

//taken from https://crocidb.com/post/simple-vector-implementation-in-c/

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>

/// Header data for a light-weight implementation of typed vectors.
struct layec_vector_header
{
    long long capacity;
    long long count;
};

void layec_vector_maybe_expand(void** vector_ref, long long element_size, long long required_count);

#define vector(T) T*
#define vector_get_header(V) (((struct layec_vector_header*)(V)) - 1)
#define vector_count(V) ((V) ? vector_get_header(V)->count : 0)
#define vector_push(V, E) do { layec_vector_maybe_expand((void**)&(V), (long long)sizeof *(V), vector_count(V) + 1); (V)[vector_count(V)] = E; vector_get_header(V)->count++; } while (0)
#define vector_pop(V) do { if (vector_get_header(V)->count) vector_get_header(V)->count--; } while (0)
#define vector_free(V) do { if (V) { memset(V, 0, (unsigned long long)vector_count(V) * (sizeof *(V))); free(vector_get_header(V)); (V) = NULL; } } while (0)
#define vector_free_all(V, F) do { if (V) { for (long long vector_index = 0; vector_index < vector_count(V); vector_index++) F((V)[vector_index]); memset(V, 0, (unsigned long long)vector_count(V) * (sizeof *(V))); free(vector_get_header(V)); (V) = NULL; } } while (0)

void layec_vector_maybe_expand(void** vector_ref, long long element_size, long long required_count)
{
    if (required_count <= 0) return;
    
    struct layec_vector_header* header = vector_get_header(*vector_ref);
    if (!*vector_ref)
    {
        long long initial_capacity = 32;
        void* new_data = malloc((sizeof *header) + (unsigned long long)(initial_capacity * element_size));
        header = (struct layec_vector_header*) new_data;

        header->capacity = initial_capacity;
        header->count = 0;

    }
    else if (required_count > header->capacity)
    {
        while (required_count > header->capacity)
            header->capacity *= 2;
        header = (struct layec_vector_header*) realloc(header, (sizeof *header) + (unsigned long long)(header->capacity * element_size));
    }
    
    *vector_ref = (void*)(header + 1);
}


#endif