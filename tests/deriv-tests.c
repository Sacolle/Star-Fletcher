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

void setup_seed(void){
    srand(SEED);
}

double randomdd(){
    uint64_t r53 = ((uint64_t)(rand()) << 21) ^ (rand() >> 2);
    return (double)r53 / 9007199254740991.0;
}

Test(random, random_doubles_in_range){
    for(int i = 0; i < 10; i++){
        double rv = randomdd();
        //cr_log_info("Random is %lf", rv);
        cr_assert(le(dbl, rv, 1.01));
        cr_assert(ge(dbl, rv, -0.01));
    }
}

TestSuite(derivative, .init = setup_seed);


Test(derivative, first_degree_xlin) {
    static FP matrix[100];

    for(int i = 0; i < 100; i++){
        matrix[i] = randomdd();
    }

    for(int i = 4; i < 96; i++){
        const FP baseline = Der1((&matrix[0]), i, 1, 1.0);
        const FP my_impl = fst_deriv_dir(&matrix[0], NULL, NULL, i, i, 1, 1.0, 100);
        //cr_log_info("Baseline value is %lf and my impl is %lf", baseline, my_impl);

        cr_assert(epsilon_eq(baseline, my_impl, 0.001));
    }
}

Test(derivative, first_degree_break) {
    static FP matrix[100];

    for(int i = 0; i < 100; i++){
        matrix[i] = randomdd();
    }

    const FP baseline = Der1((&matrix[50]), 0, 1, 1.0);
    const FP my_impl = fst_deriv_dir(&matrix[50], &matrix[0], NULL, 0, 0, 1, 1.0, 50);
    cr_log_info("Baseline value is %lf and my impl is %lf", baseline, my_impl);

    cr_assert(epsilon_eq(baseline, my_impl, 0.001));
}

Test(misc, passing) {
    cr_assert(1);
}