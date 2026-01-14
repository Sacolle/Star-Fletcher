#include <criterion/criterion.h>
#include <criterion/theories.h>
#include <criterion/new/assert.h>
#include <criterion/logging.h>

#include "derivatives.h"

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

Test(derivative, cross_deriv_xy) {
    #define SIZE 9
    static FP matrix[SIZE * SIZE];

    for(int j = 0; j < SIZE; j++){
        for(int i = 0; i < SIZE; i++){
            matrix[j * SIZE + i] = FP_RAND();
        }
    }
    const size_t base_idx = 4 * SIZE + 4;
    const FP baseline = DerCross((&matrix[0]), base_idx, 1, SIZE, 1.0);
    const FP my_impl = cross_deriv_ddir(
        &matrix[0], base_idx, 
        4, NULL, NULL, 1, 
        4, NULL, NULL, SIZE, 
        NULL, NULL, NULL, NULL, 
        SIZE, 1.0
    );

    cr_log_info("Baseline value is %lf and my impl is %lf", baseline, my_impl);

    cr_assert(epsilon_eq(FP_CRIT, baseline, my_impl, EPSILON));
}

Test(derivative, cross_deriv_yz) {
    #define SIZE 9
    static FP matrix[SIZE * SIZE * SIZE];

    for(int k = 0; k < SIZE; k++){
        for(int j = 0; j < SIZE; j++){
            for(int i = 0; i < SIZE; i++){
                matrix[k * SIZE * SIZE + j * SIZE + i] = FP_RAND();
            }
        }
    }
    const size_t x = 4, y = 4, z = 4;
    const size_t base_idx = z * SIZE * SIZE + y * SIZE + x;

    const FP baseline = DerCross((&matrix[0]), base_idx, SIZE, SIZE * SIZE, 1.0);
    const FP my_impl = cross_deriv_ddir(
        &matrix[0], base_idx, 
        y, NULL, NULL, SIZE, 
        z, NULL, NULL, SIZE * SIZE, 
        NULL, NULL, NULL, NULL, 
        SIZE, 1.0
    );

    cr_log_info("Baseline value is %lf and my impl is %lf", baseline, my_impl);

    cr_assert(epsilon_eq(FP_CRIT, baseline, my_impl, EPSILON));
}

Test(derivative, cross_deriv_xz) {
    #define SIZE 9
    static FP matrix[SIZE * SIZE * SIZE];

    for(int k = 0; k < SIZE; k++){
        for(int j = 0; j < SIZE; j++){
            for(int i = 0; i < SIZE; i++){
                matrix[k * SIZE * SIZE + j * SIZE + i] = FP_RAND();
            }
        }
    }
    const size_t x = 4, y = 4, z = 4;
    const size_t base_idx = z * SIZE * SIZE + y * SIZE + x;

    const FP baseline = DerCross((&matrix[0]), base_idx, 1, SIZE * SIZE, 1.0);
    const FP my_impl = cross_deriv_ddir(
        &matrix[0], base_idx, 
        x, NULL, NULL, 1, 
        z, NULL, NULL, SIZE * SIZE, 
        NULL, NULL, NULL, NULL, 
        SIZE, 1.0
    );

    cr_log_info("Baseline value is %lf and my impl is %lf", baseline, my_impl);

    cr_assert(epsilon_eq(FP_CRIT, baseline, my_impl, EPSILON));
}

Test(derivative, cross_deriv_zx) {
    #define SIZE 9
    static FP matrix[SIZE * SIZE * SIZE];

    for(int k = 0; k < SIZE; k++){
        for(int j = 0; j < SIZE; j++){
            for(int i = 0; i < SIZE; i++){
                matrix[k * SIZE * SIZE + j * SIZE + i] = FP_RAND();
            }
        }
    }
    const size_t x = 4, y = 4, z = 4;
    const size_t base_idx = z * SIZE * SIZE + y * SIZE + x;

    const FP baseline = DerCross((&matrix[0]), base_idx, SIZE * SIZE, 1, 1.0);
    const FP my_impl = cross_deriv_ddir(
        &matrix[0], base_idx, 
        z, NULL, NULL, SIZE * SIZE, 
        x, NULL, NULL, 1, 
        NULL, NULL, NULL, NULL, 
        SIZE, 1.0
    );

    cr_log_info("Baseline value is %lf and my impl is %lf", baseline, my_impl);

    cr_assert(epsilon_eq(FP_CRIT, baseline, my_impl, EPSILON));
}

Test(derivative, same_random_values) {
    #define SIZE 9
    #define SEG 3
    #define BIGSIZE (SEG * SIZE)
    #define IDX(i, j, k, size) (i) + ((j) * size) + ((k) * size * size)
    #define CB(x) ((x) * (x) * (x))
    static FP mbig[CB(BIGSIZE)];
    static FP mseg[CB(SEG)][CB(SIZE)];

    for(int dz = 0; dz < SEG; dz++){
        for(int dy = 0; dy < SEG; dy++){
            for(int dx = 0; dx < SEG; dx++){
                for(int k = 0; k < SIZE; k++){
                    for(int j = 0; j < SIZE; j++){
                        for(int i = 0; i < SIZE; i++){
                            const FP rand_val = FP_RAND();
                            mseg[IDX(dx, dy, dz, SEG)][IDX(i, j, k, SIZE)] = rand_val;
                            mbig[IDX(i + dx * SIZE, j + dy * SIZE, k + dz * SIZE, BIGSIZE)] = rand_val;
                        }
                    }
                }
            }
        }
    }
    for(int dz = 0; dz < SEG; dz++){
        for(int dy = 0; dy < SEG; dy++){
            for(int dx = 0; dx < SEG; dx++){
                FP* block = mseg[IDX(dx, dy, dz, SEG)];
                for(int k = 0; k < SIZE; k++){
                    for(int j = 0; j < SIZE; j++){
                        for(int i = 0; i < SIZE; i++){
                            
                            cr_assert(epsilon_eq(FP_CRIT, 
                                block[IDX(i, j, k, SIZE)], 
                                mbig[IDX(i + dx * SIZE, j + dy * SIZE, k + dz * SIZE, BIGSIZE)], 
                                EPSILON)
                            );
                        }
                    }
                }
            }
        }
    }
}

Test(derivative, match_with_stride) {
    #define SIZE 9
    #define SEG 3
    #define BIGSIZE (SEG * SIZE)
    #define IDX(i, j, k, size) (i) + ((j) * size) + ((k) * size * size)
    #define CB(x) ((x) * (x) * (x))
    static FP mbig[CB(BIGSIZE)];
    static FP mseg[CB(SEG)][CB(SIZE)];

    for(int dz = 0; dz < SEG; dz++){
        for(int dy = 0; dy < SEG; dy++){
            for(int dx = 0; dx < SEG; dx++){
                for(int k = 0; k < SIZE; k++){
                    for(int j = 0; j < SIZE; j++){
                        for(int i = 0; i < SIZE; i++){
                            const FP rand_val = FP_RAND();
                            mseg[IDX(dx, dy, dz, SEG)][IDX(i, j, k, SIZE)] = rand_val;
                            mbig[IDX(i + dx * SIZE, j + dy * SIZE, k + dz * SIZE, BIGSIZE)] = rand_val;
                        }
                    }
                }
            }
        }
    }
    for(int dz = 0; dz < SEG; dz++){
        for(int dy = 0; dy < SEG; dy++){
            for(int dx = 0; dx < SEG; dx++){
                FP* block = mseg[IDX(dx, dy, dz, SEG)];
                size_t sk = IDX(dx * SIZE, dy * SIZE, dz * SIZE, BIGSIZE);
                for(int k = 0; k < SIZE; k++){
                    size_t sj = sk;
                    for(int j = 0; j < SIZE; j++){
                        size_t si = sj;
                        for(int i = 0; i < SIZE; i++){
                            cr_assert(epsilon_eq(FP_CRIT, 
                                block[IDX(i, j, k, SIZE)], 
                                mbig[si], 
                                EPSILON)
                            );
                            si += 1;
                        }
                        sj += BIGSIZE;
                    }
                    sk += BIGSIZE * BIGSIZE;
                }
            }
        }
    }
}

Test(derivative, corner) {
    #define SIZE 9
    #define SEG 3
    #define BIGSIZE (SEG * SIZE)
    #define IDX(i, j, k, size) (i) + ((j) * size) + ((k) * size * size)
    #define CB(x) ((x) * (x) * (x))
    static FP mbig[CB(BIGSIZE)];
    static FP mseg[CB(SEG)][CB(SIZE)];

    for(int dz = 0; dz < SEG; dz++){
        for(int dy = 0; dy < SEG; dy++){
            for(int dx = 0; dx < SEG; dx++){
                for(int k = 0; k < SIZE; k++){
                    for(int j = 0; j < SIZE; j++){
                        for(int i = 0; i < SIZE; i++){
                            const FP rand_val = FP_RAND();
                            mseg[IDX(dx, dy, dz, SEG)][IDX(i, j, k, SIZE)] = rand_val;
                            mbig[IDX(i + dx * SIZE, j + dy * SIZE, k + dz * SIZE, BIGSIZE)] = rand_val;
                        }
                    }
                }
            }
        }
    }
    FP* x_minus = mseg[IDX(0,1,1,SEG)];
    FP* x_plus = mseg[IDX(2,1,1,SEG)];

    FP* y_minus = mseg[IDX(1,0,1,SEG)];
    FP* y_plus = mseg[IDX(1,2,1,SEG)];

    FP* xy_minus_minus = mseg[IDX(0,0,1,SEG)];
    FP* xy_plus_minus  = mseg[IDX(2,0,1,SEG)];

    FP* xy_minus_plus = mseg[IDX(0,2,1,SEG)];
    FP* xy_plus_plus  = mseg[IDX(2,2,1,SEG)];


    const FP baseline = DerCross((&mbig[0]), IDX(SIZE, SIZE, SIZE, BIGSIZE), 1, BIGSIZE, 1.0);

    const FP my_impl = cross_deriv_ddir(
        mseg[IDX(1, 1, 1, SEG)], 0, 
        0, x_minus, x_plus, 1, 
        0, y_minus, y_plus, SIZE, 
        xy_plus_plus, 
        xy_plus_minus, 
        xy_minus_plus, 
        xy_minus_minus, 
        SIZE, 1.0
    );

    cr_assert(epsilon_eq(FP_CRIT, baseline, my_impl, EPSILON));

}

Test(derivative, cross_deriv_with_borders) {
    #define SIZE 9
    #define SEG 3
    #define BIGSIZE (SEG * SIZE)
    #define IDX(i, j, k, size) (i) + ((j) * size) + ((k) * size * size)
    #define CB(x) ((x) * (x) * (x))
    static FP mbig[CB(BIGSIZE)];
    static FP mseg[CB(SEG)][CB(SIZE)];

    for(int dz = 0; dz < SEG; dz++){
        for(int dy = 0; dy < SEG; dy++){
            for(int dx = 0; dx < SEG; dx++){
                for(int k = 0; k < SIZE; k++){
                    for(int j = 0; j < SIZE; j++){
                        for(int i = 0; i < SIZE; i++){
                            const FP rand_val = FP_RAND();
                            mseg[IDX(dx, dy, dz, SEG)][IDX(i, j, k, SIZE)] = rand_val;
                            mbig[IDX(i + dx * SIZE, j + dy * SIZE, k + dz * SIZE, BIGSIZE)] = rand_val;
                        }
                    }
                }
            }
        }
    }
    //start of the central cube
    const size_t x = SIZE, y =  SIZE, z = SIZE;
    const size_t base_idx = IDX(x,y,z, BIGSIZE);

    FP* x_minus = mseg[IDX(0,1,1,SEG)];
    FP* x_plus = mseg[IDX(2,1,1,SEG)];

    FP* y_minus = mseg[IDX(1,0,1,SEG)];
    FP* y_plus = mseg[IDX(1,2,1,SEG)];

    FP* xy_minus_minus = mseg[IDX(0,0,1,SEG)];
    FP* xy_plus_minus  = mseg[IDX(2,0,1,SEG)];

    FP* xy_minus_plus = mseg[IDX(0,2,1,SEG)];
    FP* xy_plus_plus  = mseg[IDX(2,2,1,SEG)];


    for(int k = 0; k < SIZE; k++){
        for(int j = 0; j < SIZE; j++){
            for(int i = 0; i < SIZE; i++){
                const FP baseline = DerCross((&mbig[0]), IDX(i + SIZE, j + SIZE, k + SIZE, BIGSIZE), 1, BIGSIZE, 1.0);

                const FP my_impl = cross_deriv_ddir(
                    mseg[IDX(1, 1, 1, SEG)], IDX(i,j,k, SIZE), 
                    i, x_minus, x_plus, 1, 
                    j, y_minus, y_plus, SIZE, 
                    xy_plus_plus, 
                    xy_plus_minus, 
                    xy_minus_plus, 
                    xy_minus_minus, 
                    SIZE, 1.0
                );

                cr_assert(epsilon_eq(FP_CRIT, baseline, my_impl, EPSILON));

            }
        }
    }
}