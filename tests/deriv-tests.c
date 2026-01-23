#include <criterion/criterion.h>
#include <criterion/theories.h>
#include <criterion/new/assert.h>
#include <criterion/logging.h>

#include "macros.h"

#include "derivatives.h"

#include <stdio.h>

//derivadas definidas dentro do fletcher
#define Der1(p, i, s, dinv) (L1*(p[i+s]-p[i-s])+ L2*(p[i+2*s]-p[i-2*s]) + L3*(p[i+3*s]-p[i-3*s]) + L4*(p[i+4*s]-p[i-4*s]))*(dinv)
#define Der2(p, i, s, d2inv) ((K0*p[i]+ K1*(p[i+s]+p[i-s])+ K2*(p[i+2*s]+p[i-2*s]) + K3*(p[i+3*s]+p[i-3*s]) + K4*(p[i+4*s]+p[i-4*s]))*(d2inv))
#define DerCross(p, i, s11, s21, dinv) ((L11*(p[i+s21+s11]-p[i+s21-s11]-p[i-s21+s11]+p[i-s21-s11])+                                                                                    \
       L12*(p[i+s21+(2*s11)]-p[i+s21-(2*s11)]-p[i-s21+(2*s11)]+p[i-s21-(2*s11)]+p[i+(2*s21)+s11]-p[i+(2*s21)-s11]-p[i-(2*s21)+s11]+p[i-(2*s21)-s11])+                                  \
       L13*(p[i+s21+(3*s11)]-p[i+s21-(3*s11)]-p[i-s21+(3*s11)]+p[i-s21-(3*s11)]+p[i+(3*s21)+s11]-p[i+(3*s21)-s11]-p[i-(3*s21)+s11]+p[i-(3*s21)-s11])+                                  \
       L14*(p[i+s21+(4*s11)]-p[i+s21-(4*s11)]-p[i-s21+(4*s11)]+p[i-s21-(4*s11)]+p[i+(4*s21)+s11]-p[i+(4*s21)-s11]-p[i-(4*s21)+s11]+p[i-(4*s21)-s11])+                                  \
       L22*(p[i+(2*s21)+(2*s11)]-p[i+(2*s21)-(2*s11)]-p[i-(2*s21)+(2*s11)]+p[i-(2*s21)-(2*s11)])+                                                                                      \
       L23*(p[i+(2*s21)+(3*s11)]-p[i+(2*s21)-(3*s11)]-p[i-(2*s21)+(3*s11)]+p[i-(2*s21)-(3*s11)]+p[i+(3*s21)+(2*s11)]-p[i+(3*s21)-(2*s11)]-p[i-(3*s21)+(2*s11)]+p[i-(3*s21)-(2*s11)])+  \
       L24*(p[i+(2*s21)+(4*s11)]-p[i+(2*s21)-(4*s11)]-p[i-(2*s21)+(4*s11)]+p[i-(2*s21)-(4*s11)]+p[i+(4*s21)+(2*s11)]-p[i+(4*s21)-(2*s11)]-p[i-(4*s21)+(2*s11)]+p[i-(4*s21)-(2*s11)])+  \
       L33*(p[i+(3*s21)+(3*s11)]-p[i+(3*s21)-(3*s11)]-p[i-(3*s21)+(3*s11)]+p[i-(3*s21)-(3*s11)])+                                                                                      \
       L34*(p[i+(3*s21)+(4*s11)]-p[i+(3*s21)-(4*s11)]-p[i-(3*s21)+(4*s11)]+p[i-(3*s21)-(4*s11)]+p[i+(4*s21)+(3*s11)]-p[i+(4*s21)-(3*s11)]-p[i-(4*s21)+(3*s11)]+p[i-(4*s21)-(3*s11)])+  \
       L44*(p[i+(4*s21)+(4*s11)]-p[i+(4*s21)-(4*s11)]-p[i-(4*s21)+(4*s11)]+p[i-(4*s21)-(4*s11)]))*(dinv))

#define DerCrossPrint(block, i, s11, s21, dinv) \
	printf("Computed operation Macro:\n %.9f *  (\n        %.9f - %.9f - %.9f + %.9f\n    ) +\n    %.9f *  (\n        %.9f - %.9f - %.9f + %.9f + \n        %.9f - %.9f - %.9f + %.9f\n    ) +\n    %.9f *  (\n        %.9f - %.9f - %.9f + %.9f +\n        %.9f - %.9f - %.9f + %.9f\n    ) +\n    %.9f *  (\n        %.9f - %.9f - %.9f + %.9f +\n        %.9f - %.9f - %.9f + %.9f\n    ) +\n    %.9f *  (\n        %.9f - %.9f - %.9f + %.9f\n    ) +\n    %.9f *  (\n        %.9f - %.9f - %.9f + %.9f + \n        %.9f - %.9f - %.9f + %.9f\n	) +        \n    %.9f *  (\n        %.9f - %.9f - %.9f + %.9f + \n        %.9f - %.9f - %.9f + %.9f\n	) +      \n    %.9f *  (\n        %.9f - %.9f - %.9f + %.9f\n\n	) + \n    %.9f *  (\n        %.9f - %.9f - %.9f + %.9f + \n        %.9f - %.9f - %.9f + %.9f\n	) + \n    %.9f *  (\n        %.9f - %.9f - %.9f + %.9f\n    )) * %.9f\n", \
       L11, block[i+s21+s11], block[i+s21-s11], block[i-s21+s11], block[i-s21-s11],                                                                                    \
       L12, block[i+s21+(2*s11)], block[i+s21-(2*s11)], block[i-s21+(2*s11)], block[i-s21-(2*s11)], block[i+(2*s21)+s11], block[i+(2*s21)-s11], block[i-(2*s21)+s11], block[i-(2*s21)-s11],                                  \
       L13, block[i+s21+(3*s11)], block[i+s21-(3*s11)], block[i-s21+(3*s11)], block[i-s21-(3*s11)], block[i+(3*s21)+s11], block[i+(3*s21)-s11], block[i-(3*s21)+s11], block[i-(3*s21)-s11],                                  \
       L14, block[i+s21+(4*s11)], block[i+s21-(4*s11)], block[i-s21+(4*s11)], block[i-s21-(4*s11)], block[i+(4*s21)+s11], block[i+(4*s21)-s11], block[i-(4*s21)+s11], block[i-(4*s21)-s11],                                  \
       L22, block[i+(2*s21)+(2*s11)], block[i+(2*s21)-(2*s11)], block[i-(2*s21)+(2*s11)], block[i-(2*s21)-(2*s11)],                                                                                      \
       L23, block[i+(2*s21)+(3*s11)], block[i+(2*s21)-(3*s11)], block[i-(2*s21)+(3*s11)], block[i-(2*s21)-(3*s11)], block[i+(3*s21)+(2*s11)], block[i+(3*s21)-(2*s11)], block[i-(3*s21)+(2*s11)], block[i-(3*s21)-(2*s11)],  \
       L24, block[i+(2*s21)+(4*s11)], block[i+(2*s21)-(4*s11)], block[i-(2*s21)+(4*s11)], block[i-(2*s21)-(4*s11)], block[i+(4*s21)+(2*s11)], block[i+(4*s21)-(2*s11)], block[i-(4*s21)+(2*s11)], block[i-(4*s21)-(2*s11)],  \
       L33, block[i+(3*s21)+(3*s11)], block[i+(3*s21)-(3*s11)], block[i-(3*s21)+(3*s11)], block[i-(3*s21)-(3*s11)],                                                                                      \
       L34, block[i+(3*s21)+(4*s11)], block[i+(3*s21)-(4*s11)], block[i-(3*s21)+(4*s11)], block[i-(3*s21)-(4*s11)], block[i+(4*s21)+(3*s11)], block[i+(4*s21)-(3*s11)], block[i-(4*s21)+(3*s11)], block[i-(4*s21)-(3*s11)],  \
       L44, block[i+(4*s21)+(4*s11)], block[i+(4*s21)-(4*s11)], block[i-(4*s21)+(4*s11)], block[i-(4*s21)-(4*s11)], dinv)

#define SEED 42

#define EPSILON FLT_EPSILON
#define FP_CRIT flt

void setup_seed(void){
    srand(SEED);
}


Test(random, random_retries){
    const size_t size = 1000;
    FP first_list[size];
    FP second_list[size];

    srand(SEED);
    for(int i = 0; i < size; i++){
        first_list[i] = FP_RAND();
    }

    srand(SEED);
    for(int i = 0; i < size; i++){
        second_list[i] = FP_RAND();
    }

    for(int i = 0; i < size; i++){
        cr_assert(epsilon_eq(FP_CRIT, first_list[i], second_list[i], 0.0001));
    }
}

TestSuite(derivative, .init = setup_seed);


Test(derivative, second_degree_break_minus) {
    static FP matrix[100];

    for(int i = 0; i < 100; i++){
        matrix[i] = FP_RAND();
    }

    const FP baseline = Der2((&matrix[50]), 0, 1, 1.0f);
    const FP my_impl = snd_deriv_dir(&matrix[50], &matrix[0], NULL, 0, 0, 1, 1.0f, 50);

    cr_log_info("Baseline value is %lf and my impl is %lf", baseline, my_impl);

    cr_assert(epsilon_eq(FP_CRIT, baseline, my_impl, EPSILON));
}

Test(derivative, second_degree_break_plus) {
    static FP matrix[100];
    for(int i = 0; i < 100; i++){
        matrix[i] = FP_RAND();
    }
    const FP baseline = Der2((&matrix[0]), 49, 1, 1.0);
    const FP my_impl = snd_deriv_dir(&matrix[0], NULL, &matrix[50], 49, 49, 1, 1.0, 50);

    cr_log_info("Baseline value is %lf and my impl is %lf", baseline, my_impl);

    cr_assert(epsilon_eq(FP_CRIT, baseline, my_impl, EPSILON));
}

Test(derivative, second_degree_xlin) {
    static FP matrix[100];
    for(int i = 0; i < 100; i++){
        matrix[i] = FP_RAND();
    }

    for(int i = 4; i < 96; i++){
        const FP baseline = Der2((&matrix[0]), i, 1, 1.0);
        //const FP baseline = 1.0;
        const FP my_impl = snd_deriv_dir(&matrix[0], NULL, NULL,i, i, 1, 1.0f, 100);
        //cr_log_info("Baseline value is %lf and my impl is %lf", baseline, my_impl);

        cr_expect(epsilon_eq(FP_CRIT, baseline, my_impl, EPSILON));
    }
}

size_t g_volume_width;
size_t g_cube_width;
size_t g_width_in_cubes;

FP* g_volume_matrix;
FP** g_segment_matrix;

void build_matricies(){
    setup_seed();

    // use 3 because it simplifies
    g_width_in_cubes = 3;
    
    //can change to see the effect of diferent values
    g_cube_width = 16;
    g_volume_width = g_cube_width * g_width_in_cubes;

    if((g_volume_matrix = malloc(sizeof(FP) * CUBE(g_volume_width))) == NULL){
        cr_assert(false);
    }

    if((g_segment_matrix = malloc(sizeof(FP*) * CUBE(g_width_in_cubes))) == NULL){
        cr_assert(false);
    }
    for(size_t l = 0; l < CUBE(g_width_in_cubes); l++){
        if((g_segment_matrix[l] = malloc(sizeof(FP) * CUBE(g_cube_width))) == NULL){
            cr_assert(false);
        }
    }

    for(size_t k = 0; k < g_width_in_cubes; k++)
    for(size_t j = 0; j < g_width_in_cubes; j++)
    for(size_t i = 0; i < g_width_in_cubes; i++){
        for(size_t z = 0; z < g_cube_width; z++)
        for(size_t y = 0; y < g_cube_width; y++)
        for(size_t x = 0; x < g_cube_width; x++){
            const FP rand_val = FP_RAND();
            g_segment_matrix[block_idx(i, j, k)][cube_idx(x, y, z)] = rand_val;
            g_volume_matrix[block_cube_to_volume_idx(x, y, z, i, j, k)] = rand_val;
        }
    }
}

void teardown_values(){
    free(g_volume_matrix);
    for(size_t l = 0; l < CUBE(g_width_in_cubes); l++){
        free(g_segment_matrix[l]);
    }
    free(g_segment_matrix);
}

TestSuite(cross_derivative, .init = build_matricies, .fini = teardown_values);

Test(cross_derivative, same_random_values) {
    for(size_t k = 0; k < g_width_in_cubes; k++)
    for(size_t j = 0; j < g_width_in_cubes; j++)
    for(size_t i = 0; i < g_width_in_cubes; i++){
        FP* block = g_segment_matrix[block_idx(i, j, k)];
        for(size_t z = 0; z < g_cube_width; z++)
        for(size_t y = 0; y < g_cube_width; y++)
        for(size_t x = 0; x < g_cube_width; x++){
            cr_assert(epsilon_eq(FP_CRIT, 
                block[cube_idx(x,y,z)], 
                g_volume_matrix[block_cube_to_volume_idx(x, y, z, i, j, k)], 
                EPSILON)
            );
        }
    }
}

Test(cross_derivative, cross_derivative_computation) {
    FP* center = g_segment_matrix[block_idx(1, 1, 1)];

    for(size_t z = 0; z < g_cube_width; z++)
    for(size_t y = 0; y < g_cube_width; y++)
    for(size_t x = 0; x < g_cube_width; x++){
        const FP baseline_xy = DerCross(g_volume_matrix, block_cube_to_volume_idx(x, y, z, 1, 1, 1), 1, g_volume_width, 1.0);
        const FP baseline_yz = DerCross(g_volume_matrix, block_cube_to_volume_idx(x, y, z, 1, 1, 1), g_volume_width, SQUARE(g_volume_width), 1.0);
        const FP baseline_xz = DerCross(g_volume_matrix, block_cube_to_volume_idx(x, y, z, 1, 1, 1), 1, SQUARE(g_volume_width), 1.0);

        const FP my_impl_xy = cross_deriv_ddir(
            center, cube_idx(x, y, z), 
            x, g_segment_matrix[block_idx(0, 1, 1)], g_segment_matrix[block_idx(2, 1, 1)], 1, 
            y, g_segment_matrix[block_idx(1, 0, 1)], g_segment_matrix[block_idx(1, 2, 1)], g_cube_width, 
            g_segment_matrix[block_idx(2, 2, 1)],
            g_segment_matrix[block_idx(2, 0, 1)],
            g_segment_matrix[block_idx(0, 2, 1)],
            g_segment_matrix[block_idx(0, 0, 1)],
            g_cube_width, 1.0
        );

        const FP my_impl_yz = cross_deriv_ddir(
            center, cube_idx(x, y, z), 
            y, g_segment_matrix[block_idx(1, 0, 1)], g_segment_matrix[block_idx(1, 2, 1)], g_cube_width, 
            z, g_segment_matrix[block_idx(1, 1, 0)], g_segment_matrix[block_idx(1, 1, 2)], SQUARE(g_cube_width), 
            g_segment_matrix[block_idx(1, 2, 2)],
            g_segment_matrix[block_idx(1, 2, 0)],
            g_segment_matrix[block_idx(1, 0, 2)],
            g_segment_matrix[block_idx(1, 0, 0)],
            g_cube_width, 1.0
        );

        const FP my_impl_xz = cross_deriv_ddir(
            center, cube_idx(x, y, z), 
            x, g_segment_matrix[block_idx(0, 1, 1)], g_segment_matrix[block_idx(2, 1, 1)], 1, 
            z, g_segment_matrix[block_idx(1, 1, 0)], g_segment_matrix[block_idx(1, 1, 2)], SQUARE(g_cube_width), 
            g_segment_matrix[block_idx(2, 1, 2)],
            g_segment_matrix[block_idx(2, 1, 0)],
            g_segment_matrix[block_idx(0, 1, 2)],
            g_segment_matrix[block_idx(0, 1, 0)],
            g_cube_width, 1.0
        );

        cr_assert(epsilon_eq(FP_CRIT, baseline_xy, my_impl_xy, EPSILON), "(%ld, %ld, %ld)", x, y, z);
        cr_assert(epsilon_eq(FP_CRIT, baseline_yz, my_impl_yz, EPSILON), "(%ld, %ld, %ld)", x, y, z);
        cr_assert(epsilon_eq(FP_CRIT, baseline_xz, my_impl_xz, EPSILON), "(%ld, %ld, %ld)", x, y, z);
    }
}

Test(cross_derivative, cross_derivative_computation_xy_single_center) {
    FP* center = g_segment_matrix[block_idx(1, 1, 1)];

    size_t z = g_cube_width / 2;
    size_t y = g_cube_width / 2;
    size_t x = g_cube_width / 2;
    const FP baseline_xy = DerCross(g_volume_matrix, block_cube_to_volume_idx(x, y, z, 1, 1, 1), 1, g_volume_width, 1.0);
    //const FP baseline_yz = DerCross(g_volume_matrix, block_cube_to_volume_idx(x, y, z, 1, 1, 1), g_volume_width, SQUARE(g_volume_width), 1.0);
    //const FP baseline_xz = DerCross(g_volume_matrix, block_cube_to_volume_idx(x, y, z, 1, 1, 1), 1, SQUARE(g_volume_width), 1.0);

    const FP my_impl_xy = cross_deriv_ddir(
        center, cube_idx(x, y, z), 
        x, g_segment_matrix[block_idx(0, 1, 1)], g_segment_matrix[block_idx(2, 1, 1)], 1, 
        x, g_segment_matrix[block_idx(1, 0, 1)], g_segment_matrix[block_idx(1, 2, 1)], g_cube_width, 
        g_segment_matrix[block_idx(2, 2, 1)],
        g_segment_matrix[block_idx(2, 0, 1)],
        g_segment_matrix[block_idx(0, 2, 1)],
        g_segment_matrix[block_idx(0, 0, 1)],
        g_cube_width, 1.0
    );

    cr_assert(epsilon_eq(FP_CRIT, baseline_xy, my_impl_xy, EPSILON), "(%ld, %ld, %ld)", x, y, z);
}

Test(cross_derivative, cross_derivative_computation_xy_single_corner) {
    FP* center = g_segment_matrix[block_idx(1, 1, 1)];

    size_t z = 0;
    size_t y = 0;
    size_t x = 0;
    const FP baseline_xy = DerCross(g_volume_matrix, block_cube_to_volume_idx(x, y, z, 1, 1, 1), 1, g_volume_width, 1.0);
    //const FP baseline_yz = DerCross(g_volume_matrix, block_cube_to_volume_idx(x, y, z, 1, 1, 1), g_volume_width, SQUARE(g_volume_width), 1.0);
    //const FP baseline_xz = DerCross(g_volume_matrix, block_cube_to_volume_idx(x, y, z, 1, 1, 1), 1, SQUARE(g_volume_width), 1.0);

    const FP my_impl_xy = cross_deriv_ddir(
        center, cube_idx(x, y, z), 
        x, g_segment_matrix[block_idx(0, 1, 1)], g_segment_matrix[block_idx(2, 1, 1)], 1, 
        x, g_segment_matrix[block_idx(1, 0, 1)], g_segment_matrix[block_idx(1, 2, 1)], g_cube_width, 
        g_segment_matrix[block_idx(2, 2, 1)],
        g_segment_matrix[block_idx(2, 0, 1)],
        g_segment_matrix[block_idx(0, 2, 1)],
        g_segment_matrix[block_idx(0, 0, 1)],
        g_cube_width, 1.0
    );

    cr_assert(epsilon_eq(FP_CRIT, baseline_xy, my_impl_xy, EPSILON), "(%ld, %ld, %ld)", x, y, z);
}

Test(cross_derivative, cross_derivative_ootwo) {
    FP* center = g_segment_matrix[block_idx(1, 1, 1)];

    size_t z = 0;
    size_t y = 0;
    size_t x = 2;
    const FP baseline_yz = DerCross(g_volume_matrix, block_cube_to_volume_idx(x, y, z, 1, 1, 1), g_volume_width, SQUARE(g_volume_width), 1.0);

    // DerCrossPrint(g_volume_matrix, block_cube_to_volume_idx(x, y, z, 1, 1, 1), g_volume_width, SQUARE(g_volume_width), 1.0);

    const FP my_impl_yz = cross_deriv_ddir(
        center, cube_idx(x, y, z), 
        y, g_segment_matrix[block_idx(1, 0, 1)], g_segment_matrix[block_idx(1, 2, 1)], g_cube_width, 
        z, g_segment_matrix[block_idx(1, 1, 0)], g_segment_matrix[block_idx(1, 1, 2)], SQUARE(g_cube_width), 
        g_segment_matrix[block_idx(1, 2, 2)],
        g_segment_matrix[block_idx(1, 2, 0)],
        g_segment_matrix[block_idx(1, 0, 2)],
        g_segment_matrix[block_idx(1, 0, 0)],
        g_cube_width, 1.0
    );
    fflush(stdout);
    cr_assert(epsilon_eq(FP_CRIT, baseline_yz, my_impl_yz, EPSILON), "(%ld, %ld, %ld)", x, y, z);
}

Test(cross_derivative, cross_derivative_otwelveo_yz) {
    FP* center = g_segment_matrix[block_idx(1, 1, 1)];

    size_t z = 0;
    size_t y = 12;
    size_t x = 0;
    const FP baseline_yz = DerCross(g_volume_matrix, block_cube_to_volume_idx(x, y, z, 1, 1, 1), g_volume_width, SQUARE(g_volume_width), 1.0);

    //DerCrossPrint(g_volume_matrix, block_cube_to_volume_idx(x, y, z, 1, 1, 1), g_volume_width, SQUARE(g_volume_width), 1.0);

    const FP my_impl_yz = cross_deriv_ddir(
        center, cube_idx(x, y, z), 
        y, g_segment_matrix[block_idx(1, 0, 1)], g_segment_matrix[block_idx(1, 2, 1)], g_cube_width, 
        z, g_segment_matrix[block_idx(1, 1, 0)], g_segment_matrix[block_idx(1, 1, 2)], SQUARE(g_cube_width), 
        g_segment_matrix[block_idx(1, 2, 2)],
        g_segment_matrix[block_idx(1, 2, 0)],
        g_segment_matrix[block_idx(1, 0, 2)],
        g_segment_matrix[block_idx(1, 0, 0)],
        g_cube_width, 1.0
    );
    fflush(stdout);
    cr_assert(epsilon_eq(FP_CRIT, baseline_yz, my_impl_yz, EPSILON), "(%ld, %ld, %ld)", x, y, z);
}

Test(cross_derivative, cross_derivative_otwelveo) {
    FP* center = g_segment_matrix[block_idx(1, 1, 1)];

    size_t z = 0;
    size_t y = 12;
    size_t x = 0;
    const FP baseline_xz = DerCross(g_volume_matrix, block_cube_to_volume_idx(x, y, z, 1, 1, 1), 1, SQUARE(g_volume_width), 1.0);

    //DerCrossPrint(g_volume_matrix, block_cube_to_volume_idx(x, y, z, 1, 1, 1), 1, SQUARE(g_volume_width), 1.0);

    const FP my_impl_xz = cross_deriv_ddir(
        center, cube_idx(x, y, z), 
        x, g_segment_matrix[block_idx(0, 1, 1)], g_segment_matrix[block_idx(2, 1, 1)], 1, 
        z, g_segment_matrix[block_idx(1, 1, 0)], g_segment_matrix[block_idx(1, 1, 2)], SQUARE(g_cube_width), 
        g_segment_matrix[block_idx(2, 1, 2)],
        g_segment_matrix[block_idx(2, 1, 0)],
        g_segment_matrix[block_idx(0, 1, 2)],
        g_segment_matrix[block_idx(0, 1, 0)],
        g_cube_width, 1.0
    );
    fflush(stdout);
    cr_assert(epsilon_eq(FP_CRIT, baseline_xz, my_impl_xz, EPSILON), "(%ld, %ld, %ld)", x, y, z);
}
