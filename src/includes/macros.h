#ifndef GUARD_MACROS
#define GUARD_MACROS

// linear idx https://stackoverflow.com/a/34363187
#define CUBE_I(x, y, z) ((x) + ((y) * g_cube_width) + ((z) * g_cube_width * g_cube_width))
#define BLOCK_I(i, j, k) ((i) + ((j) * g_width_in_cubes) + ((k) * g_width_in_cubes * g_width_in_cubes))
#define SQUARE(x) ((x)*(x))
#define CUBE(x) ((x)*(x)*(x))

#define FSTBLK(blk) ((blk) == 0)
#define LSTBLK(blk) ((blk) == (g_width_in_cubes - 1))

#define START(blk, idx) (FSTBLK(blk) && (idx) < KERNEL_SIZE)
#define END(blk, idx) (LSTBLK(blk) && (idx) >= (g_cube_width - KERNEL_SIZE))
#define INEDGE(blk_idx, cidx) (START(blk_idx, cidx) || END(blk_idx, cidx))

#endif