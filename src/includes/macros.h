#ifndef GUARD_MACROS
#define GUARD_MACROS

#include <stddef.h>

// linear idx https://stackoverflow.com/a/34363187
// calculates a + width * (b + c * width)
static inline size_t idx(size_t a, size_t b, size_t c, size_t width){
    return a + width * (b + c * width);
}

struct coords3d {
    size_t x;
    size_t y;
    size_t z;
};

// extract the components
// https://stackoverflow.com/a/11712864
static inline struct coords3d unindex(size_t index, size_t size){
    struct coords3d c;

    c.x = index / (size * size);
    c.y = (index / size) % size;
    c.z = index % size;

    return c;
}

// the linear index of the point(x, y, z) inside a segmented cube
// dependes on the global variable `g_cube_width`;
static inline size_t cube_idx(size_t x, size_t y, size_t z){
    extern size_t g_cube_width;
    return idx(x, y, z, g_cube_width);
}

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

// turn a (x, y, z) point in a (i, j, k) cube in a (vx, vy, vz) coordenate for the complete volume
// depends on `g_cube_width` and `g_volume_width`
static inline size_t block_cube_to_volume_idx(size_t x, size_t y, size_t z, size_t i, size_t j, size_t k){
    extern size_t g_cube_width;
    return volume_idx(x + i * g_cube_width, y + j * g_cube_width, z + k * g_cube_width);
}


// turn a global volume linear idx into a the index of the block that point correspondes
// depends on `g_cube_width`, `g_width_in_cubes` and `g_volume_width`
static inline size_t volume_to_block_idx(size_t idx){
    extern size_t g_volume_width;
    extern size_t g_cube_width;

    const struct coords3d c = unindex(idx, g_volume_width);
    const size_t vx = c.x;
    const size_t vy = c.y;
    const size_t vz = c.z;

    return block_idx(vx / g_cube_width, vy / g_cube_width, vz / g_cube_width);
}

// turn a global volume linear index into a the index inside its corresponding block 
// depends on `g_cube_width` and `g_volume_width`
static inline size_t volume_to_cube_idx(size_t idx){
    extern size_t g_volume_width;
    extern size_t g_cube_width;

    const struct coords3d c = unindex(idx, g_volume_width);
    const size_t vx = c.x;
    const size_t vy = c.y;
    const size_t vz = c.z;

    return cube_idx(vx % g_cube_width, vy % g_cube_width, vz % g_cube_width);
}

#endif