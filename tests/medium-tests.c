#include <criterion/criterion.h>
#include <criterion/theories.h>
#include <criterion/new/assert.h>
#include <criterion/logging.h>

#include <stdint.h>
#include <float.h>

#include "medium.h"
#include "macros.h"

#define ind(ix,iy,iz) (((iz) * sy + (iy)) * sx + (ix))
// functions extracted from [fletcher-base](https://github.com/gabrielfrtg/fletcher-base/tree/main)
// this one sits in main, so i extratec it to a function
void fletcher_base_medium_initialize(const enum Form prob, size_t sx, size_t sy, size_t sz, 
    FP *restrict vpz, FP *restrict vsv, FP *restrict epsilon,
    FP *restrict delta, FP *restrict phi, FP *restrict theta
){
    size_t i;
    switch (prob){
    case ISO:
        for (i = 0; i < sx * sy * sz; i++)
        {
            vpz[i]     = FP_LIT(3000.0);
            epsilon[i] = FP_LIT(0.0);
            delta[i]   = FP_LIT(0.0);
            phi[i]     = FP_LIT(0.0);
            theta[i]   = FP_LIT(0.0);
            vsv[i]     = FP_LIT(0.0);
        }
        break;
    case VTI:
        for (i = 0; i < sx * sy * sz; i++)
        {
            vpz[i]     = FP_LIT(3000.0);
            epsilon[i] = FP_LIT(0.24);
            delta[i]   = FP_LIT(0.1);
            phi[i]     = FP_LIT(0.0);
            theta[i]   = FP_LIT(0.0);
            if (SIGMA > MAX_SIGMA)
            {
                vsv[i] = FP_LIT(0.0);
            }
            else
            {
                vsv[i] = vpz[i] * FP_SQRT(FP_ABS(epsilon[i] - delta[i]) / SIGMA);
            }
        }
        break;

    case TTI:
        for (i = 0; i < sx * sy * sz; i++)
        {
            vpz[i]     = FP_LIT(3000.0);
            epsilon[i] = FP_LIT(0.24);
            delta[i]   = FP_LIT(0.1);
            phi[i]     = FP_LIT(1.0); 
            theta[i] = FP_ATAN(1.0);
            if (SIGMA > MAX_SIGMA)
            {
                vsv[i] = FP_LIT(0.0);
            }
            else
            {
                vsv[i] = vpz[i] * FP_SQRT(FP_ABS(epsilon[i] - delta[i]) / SIGMA);
            }
        }
    }
}

void RandomVelocityBoundary(int sx, int sy, int sz,
                            int nx, int ny, int nz,
                            int bord, int absorb,
                            FP *vpz, FP *vsv)
{

    int i, ix, iy, iz;
    int distx, disty, distz, dist;
    int ivelx, ively, ivelz;
    FP bordDist;
    FP frac, rfac;
    int firstIn, bordLen;
    FP maxP, maxS;

    // maximum speed of P and S within bounds
    maxP = 0.0;
    maxS = 0.0;
    for (iz = bord + absorb; iz < nz + bord + absorb; iz++)
    {
        for (iy = bord + absorb; iy < ny + bord + absorb; iy++)
        {
            for (i = ind(bord + absorb, iy, iz); i < ind(nx + bord + absorb, iy, iz); i++)
            {
                maxP = FP_MAX(vpz[i], maxP);
                maxS = FP_MAX(vsv[i], maxS);
            }
        }
    }

    bordLen = bord + absorb - 1; // last index on low absortion zone
    firstIn = bordLen + 1;       // first index inside input grid
    frac = 1.0 / (FP)(absorb);

    for (iz = 0; iz < sz; iz++)
    {
        for (iy = 0; iy < sy; iy++)
        {
            for (ix = 0; ix < sx; ix++)
            {
                i = ind(ix, iy, iz);
                // do nothing inside input grid
                if ((iz >= firstIn && iz <= bordLen + nz) &&
                    (iy >= firstIn && iy <= bordLen + ny) &&
                    (ix >= firstIn && ix <= bordLen + nx))
                {
                    continue;
                }
                // random speed inside absortion zone
                else if ((iz >= bord && iz <= (bord + absorb + nz + absorb - 1)) &&
                         (iy >= bord && iy <= (bord + absorb + ny + absorb - 1)) &&
                         (ix >= bord && ix <= (bord + absorb + nx + absorb - 1)))
                {
                    if (iz > bordLen + nz)
                    {
                        distz = iz - bordLen - nz;
                        ivelz = bordLen + nz;
                    }
                    else if (iz < firstIn)
                    {
                        distz = firstIn - iz;
                        ivelz = firstIn;
                    }
                    else
                    {
                        distz = 0;
                        ivelz = iz;
                    }
                    if (iy > bordLen + ny)
                    {
                        disty = iy - bordLen - ny;
                        ively = bordLen + ny;
                    }
                    else if (iy < firstIn)
                    {
                        disty = firstIn - iy;
                        ively = firstIn;
                    }
                    else
                    {
                        disty = 0;
                        ively = iy;
                    }
                    if (ix > bordLen + nx)
                    {
                        distx = ix - bordLen - nx;
                        ivelx = bordLen + nx;
                    }
                    else if (ix < firstIn)
                    {
                        distx = firstIn - ix;
                        ivelx = firstIn;
                    }
                    else
                    {
                        distx = 0;
                        ivelx = ix;
                    }
                    dist = (disty > distz) ? disty : distz;
                    dist = (dist > distx) ? dist : distx;
                    bordDist = (FP)(dist)*frac;
                    rfac = (FP)rand() / (FP)RAND_MAX;

                    vpz[i] = vpz[ind(ivelx, ively, ivelz)] * (1.0 - bordDist) +
                             maxP * rfac * bordDist;
                    vsv[i] = vsv[ind(ivelx, ively, ivelz)] * (1.0 - bordDist) +
                             maxS * rfac * bordDist;

                    /*
                    vpz[i] = bordDist;
                    vsv[i] = bordDist;
                    */
                }
                // null speed at border
                else
                // PPL added {} surrounding vpz and vsv lines below
                {
                    vpz[i] = 0.0;
                    vsv[i] = 0.0;
                }
            }
        }
    }
}


#define SEED 42

void setup_seed(void)
{
    srand(SEED);
}

TestSuite(medium, .init = setup_seed);

//usado para os testes se adequarem a compilação
#if defined(FP_FLOAT)
    #define CRIT_FP flt
    #define EPSILON FLT_EPSILON
#elif defined(FP_LONG_DOUBLE)
    #define CRIT_FP ldbl
    #define EPSILON LDBL_EPSILON
#else
    #define CRIT_FP dbl
    #define EPSILON DBL_EPSILON
#endif


size_t g_volume_width;

Test(medium, proper_initialization){

    g_volume_width = 10;
    const int sx = 10;
    const int sy = 10;
    const int sz = 10;
    const int size = sx * sy * sz;
    FP vpz_base[size];
    FP vsv_base[size];
    FP epsilon_base[size];
    FP delta_base[size];
    FP phi_base[size];
    FP theta_base[size];

    FP vpz_myimpl[size];
    FP vsv_myimpl[size];
    FP epsilon_myimpl[size];
    FP delta_myimpl[size];
    FP phi_myimpl[size];
    FP theta_myimpl[size];


    for(int i = 0; i < 3; i++){
        const enum Form prob = i;
        fletcher_base_medium_initialize(prob, sx, sy, sz, 
            vpz_base, vsv_base, epsilon_base, delta_base, phi_base, theta_base);
        
        medium_initialize(prob, size, 
            vpz_myimpl, vsv_myimpl, epsilon_myimpl, delta_myimpl, phi_myimpl, theta_myimpl);

        for(int i = 0; i < size; i++){
            cr_assert(epsilon_eq(CRIT_FP, vpz_base[i], vpz_myimpl[i], EPSILON));
            cr_assert(epsilon_eq(CRIT_FP, vsv_base[i], vsv_myimpl[i], EPSILON));
            cr_assert(epsilon_eq(CRIT_FP, epsilon_base[i], epsilon_myimpl[i], EPSILON));
            cr_assert(epsilon_eq(CRIT_FP, delta_base[i], delta_myimpl[i], EPSILON));
            cr_assert(epsilon_eq(CRIT_FP, phi_base[i], phi_myimpl[i], EPSILON));
            cr_assert(epsilon_eq(CRIT_FP, theta_base[i], theta_myimpl[i], EPSILON));
        }
    }

    cr_log_info("Baseline value is %lf and my impl is %lf", vpz_base[157], vpz_myimpl[157]);
    
}

Test(medium, correct_absorb){

    const int bord = 4;
    const int absorb = 4;
    const int nx = 14;
    const int ny = 14;
    const int nz = 14;
    const int sx = nx + 2 * bord + 2 * absorb;
    const int sy = ny + 2 * bord + 2 * absorb;
    const int sz = nz + 2 * bord + 2 * absorb;

    g_volume_width = sx;
    const int size = sx * sy * sz;

    FP vpz_base[size];
    FP vsv_base[size];
    FP epsilon_base[size];
    FP delta_base[size];
    FP phi_base[size];
    FP theta_base[size];

    FP vpz_myimpl[size];
    FP vsv_myimpl[size];
    FP epsilon_myimpl[size];
    FP delta_myimpl[size];
    FP phi_myimpl[size];
    FP theta_myimpl[size];

    fletcher_base_medium_initialize(TTI, sx, sy, sz, 
        vpz_base, vsv_base, epsilon_base, delta_base, phi_base, theta_base);
    
    medium_initialize(TTI, size, 
        vpz_myimpl, vsv_myimpl, epsilon_myimpl, delta_myimpl, phi_myimpl, theta_myimpl);


    setup_seed();
    RandomVelocityBoundary(sx, sy, sz, nx, ny, nz, bord, absorb, vpz_base, vsv_base);

    setup_seed();
    medium_random_velocity_boundary(bord, absorb, vpz_myimpl, vsv_myimpl);


    for(int z = 0; z < sz; z++){
        for(int y = 0; y < sy; y++){
            for(int x = 0; x < sx; x++){
                const size_t i = volume_idx(x, y, z);
                cr_assert(epsilon_eq(CRIT_FP, vpz_base[i], vpz_myimpl[i], EPSILON), "vpz at (%d, %d, %d)", x, y, z);
                cr_assert(epsilon_eq(CRIT_FP, vsv_base[i], vsv_myimpl[i], EPSILON), "vsv at (%d, %d, %d)", x, y, z);
            }
        }
    }
}