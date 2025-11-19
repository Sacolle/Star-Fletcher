#ifndef GUARD_MACROS
#define GUARD_MACROS

#include <stddef.h>

// linear idx https://stackoverflow.com/a/34363187
//#define CUBE_I(x, y, z) ((x) + ((y) * g_cube_width) + ((z) * g_cube_width * g_cube_width))

// the linear index of the point(x, y, z) inside a segmented cube
// dependes on the global variable `g_cube_width`;
static inline size_t cube_idx(size_t x, size_t y, size_t z){
    extern size_t g_cube_width;
    return x + g_cube_width * (y + z * g_cube_width);
}

//#define BLOCK_I(i, j, k) ((i) + ((j) * g_width_in_cubes) + ((k) * g_width_in_cubes * g_width_in_cubes))

// the linear index of the block(i,j,k) in the total volume
// dependes on the global variable `g_width_in_cubes`;
static inline size_t block_idx(size_t i, size_t j, size_t k){
    extern size_t g_width_in_cubes;
    return i + g_width_in_cubes * (j + k * g_width_in_cubes);
}

// the linear index for a point (vx, vy, vz) in the whole volume
// dependes on the global variable `g_volume_width`;
static inline size_t volume_idx(size_t vx, size_t vy, size_t vz){
    extern size_t g_volume_width;
    return vx + g_volume_width * (vy + vz * g_volume_width);
}

#define SQUARE(x) ((x)*(x))
#define CUBE(x) ((x)*(x)*(x))

#define FSTBLK(blk) ((blk) == 0)
#define LSTBLK(blk) ((blk) == (g_width_in_cubes - 1))

//NOTE: need these?
#define START(blk, idx) (FSTBLK(blk) && (idx) < BORDER_WIDTH)
#define END(blk, idx) (LSTBLK(blk) && (idx) >= (g_cube_width - BORDER_WIDTH))
#define INEDGE(blk_idx, cidx) (START(blk_idx, cidx) || END(blk_idx, cidx))

//if exp != 0, run body and goto tag
#define TRYTO(exp, body, tag) if((exp) != 0) { body; goto tag; }

#endif