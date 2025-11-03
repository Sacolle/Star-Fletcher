#ifndef GUARD_MACROS
#define GUARD_MACROS

//extern uint64_t g_cube_width;
//extern uint64_t g_width_in_cubes;

// linear idx https://stackoverflow.com/a/34363187
#define CUBE_I(x, y, z) ((x) + ((y) * g_cube_width) + ((z) * g_cube_width * g_cube_width))

static inline size_t cube_idx(size_t x, size_t y, size_t z){
    extern uint64_t g_cube_width;
    return x + g_cube_width * (y + z * g_cube_width);
}


#define BLOCK_I(i, j, k) ((i) + ((j) * g_width_in_cubes) + ((k) * g_width_in_cubes * g_width_in_cubes))
#define SQUARE(x) ((x)*(x))
#define CUBE(x) ((x)*(x)*(x))

#define FSTBLK(blk) ((blk) == 0)
#define LSTBLK(blk) ((blk) == (g_width_in_cubes - 1))

//NOTE: need these?
#define START(blk, idx) (FSTBLK(blk) && (idx) < BORDER_SIZE)
#define END(blk, idx) (LSTBLK(blk) && (idx) >= (g_cube_width - BORDER_SIZE))
#define INEDGE(blk_idx, cidx) (START(blk_idx, cidx) || END(blk_idx, cidx))

#endif