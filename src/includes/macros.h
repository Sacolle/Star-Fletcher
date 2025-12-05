#ifndef GUARD_MACROS
#define GUARD_MACROS

#include <stddef.h>

// calculates a + width * (b + c * width)
static inline size_t idx(size_t a, size_t b, size_t c, size_t width){
    return a + width * (b + c * width);
}
// linear idx https://stackoverflow.com/a/34363187
//#define CUBE_I(x, y, z) ((x) + ((y) * g_cube_width) + ((z) * g_cube_width * g_cube_width))

// the linear index of the point(x, y, z) inside a segmented cube
// dependes on the global variable `g_cube_width`;
static inline size_t cube_idx(size_t x, size_t y, size_t z){
    extern size_t g_cube_width;
    return idx(x, y, z, g_cube_width);
}

//#define BLOCK_I(i, j, k) ((i) + ((j) * g_width_in_cubes) + ((k) * g_width_in_cubes * g_width_in_cubes))

// the linear index of the block(i,j,k) in the total volume
// dependes on the global variable `g_width_in_cubes`;
static inline size_t block_idx(size_t i, size_t j, size_t k){
    extern size_t g_width_in_cubes;
    return idx(i, j, k, g_width_in_cubes);
}

// the linear index for a point (vx, vy, vz) in the whole volume
// dependes on the global variable `g_volume_width`;
static inline size_t volume_idx(size_t vx, size_t vy, size_t vz){
    extern size_t g_volume_width;
    return idx(vx, vy, vz, g_volume_width);
}

#define SQUARE(x) ((x)*(x))
#define CUBE(x) ((x)*(x)*(x))

//if exp != 0, run body and goto tag
#define TRYTO(exp, body, tag) if((exp) != 0) { body; goto tag; }

#endif