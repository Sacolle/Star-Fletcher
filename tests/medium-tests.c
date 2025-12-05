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


#define FCUT        40.0
#define PICUBE      31.00627668029982017537
#define TWOSQRTPI    3.54490770181103205458
#define THREESQRTPI  5.31736155271654808184

FP Source(FP dt, int it){
  FP tf, fc, fct, expo;
  tf=TWOSQRTPI/FCUT;
  fc=FCUT/THREESQRTPI;
  fct=fc*(((FP)it)*dt-tf);
  expo=PICUBE*fct*fct;
  return ((FP_LIT(1.0)-FP_LIT(2.0)*expo)*FP_EXP(-expo));
}

void intermediary_values(
    int sx, int sy , int sz, 
    FP *restrict vpz, FP *restrict vsv, FP *restrict epsilon,
    FP *restrict delta, FP *restrict phi, FP *restrict theta,

    FP* ch1dxx, FP* ch1dyy, FP* ch1dzz, 
    FP* ch1dxy, FP* ch1dyz, FP* ch1dxz, 
    FP* v2px, FP* v2pz, FP* v2sz, FP* v2pn
){

    // coeficients of derivatives at H1 operator
    for (int i=0; i<sx*sy*sz; i++) {
        FP sinTheta=FP_SIN(theta[i]);
        FP cosTheta=FP_COS(theta[i]);
        FP sin2Theta=FP_SIN(FP_LIT(2.0)*theta[i]);
        FP sinPhi=FP_SIN(phi[i]);
        FP cosPhi=FP_COS(phi[i]);
        FP sin2Phi=FP_SIN(FP_LIT(2.0)*phi[i]);
        ch1dxx[i]=sinTheta*sinTheta * cosPhi*cosPhi;
        ch1dyy[i]=sinTheta*sinTheta * sinPhi*sinPhi;
        ch1dzz[i]=cosTheta*cosTheta;
        ch1dxy[i]=sinTheta*sinTheta * sin2Phi;
        ch1dyz[i]=sin2Theta         * sinPhi;
        ch1dxz[i]=sin2Theta         * cosPhi;
    }

    // coeficients of H1 and H2 at PDEs

    for (int i=0; i<sx*sy*sz; i++){
        v2sz[i]=vsv[i]*vsv[i];
        v2pz[i]=vpz[i]*vpz[i];
        v2px[i]=v2pz[i]*(FP_LIT(1.0)+FP_LIT(2.0)*epsilon[i]);
        v2pn[i]=v2pz[i]*(FP_LIT(1.0)+FP_LIT(2.0)*delta[i]);
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
size_t g_cube_width;
size_t g_width_in_cubes;

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


Test(medium, correct_intermediary_values){
    const size_t seg = 3;
    const size_t c_size = 10;
    g_width_in_cubes = seg;
    g_cube_width = c_size;

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

    const size_t total_seg = seg * seg * seg;
    const size_t total_c_size = c_size * c_size * c_size;
    
    FP* ch1dxx_base = (FP*) malloc(sizeof(FP) * size);
    cr_assert(ne(ptr,ch1dxx_base, NULL));

    FP* ch1dyy_base = (FP*) malloc(sizeof(FP) * size);
    cr_assert(ne(ptr,ch1dyy_base, NULL));

    FP* ch1dzz_base = (FP*) malloc(sizeof(FP) * size);
    cr_assert(ne(ptr,ch1dzz_base, NULL));

    FP* ch1dxy_base = (FP*) malloc(sizeof(FP) * size);
    cr_assert(ne(ptr,ch1dxy_base, NULL));

    FP* ch1dyz_base = (FP*) malloc(sizeof(FP) * size);
    cr_assert(ne(ptr,ch1dyz_base, NULL));

    FP* ch1dxz_base = (FP*) malloc(sizeof(FP) * size);
    cr_assert(ne(ptr,ch1dxz_base, NULL));

    FP* v2px_base = (FP*) malloc(sizeof(FP) * size);
    cr_assert(ne(ptr,v2px_base, NULL));

    FP* v2pz_base = (FP*) malloc(sizeof(FP) * size);
    cr_assert(ne(ptr,v2pz_base, NULL));

    FP* v2sz_base = (FP*) malloc(sizeof(FP) * size);
    cr_assert(ne(ptr,v2sz_base, NULL));

    FP* v2pn_base = (FP*) malloc(sizeof(FP) * size);
    cr_assert(ne(ptr,v2pn_base, NULL));



    FP* ch1dxx_myimpl[total_seg];
    FP* ch1dyy_myimpl[total_seg];
    FP* ch1dzz_myimpl[total_seg];
    FP* ch1dxy_myimpl[total_seg];
    FP* ch1dyz_myimpl[total_seg];
    FP* ch1dxz_myimpl[total_seg];
    FP* v2px_myimpl[total_seg];
    FP* v2pz_myimpl[total_seg];
    FP* v2sz_myimpl[total_seg];
    FP* v2pn_myimpl[total_seg];

    FP** buffs[10] = {
        ch1dxx_myimpl,
        ch1dyy_myimpl,
        ch1dzz_myimpl,
        ch1dxy_myimpl,
        ch1dyz_myimpl,
        ch1dxz_myimpl,
        v2px_myimpl,
        v2pz_myimpl,
        v2sz_myimpl,
        v2pn_myimpl
    };

    for(int i = 0; i < 10; i++){
        for(int seg = 0; seg < total_seg; seg++){
            if((buffs[i][seg] = (FP*) malloc(sizeof(FP) * total_c_size)) == NULL){
                cr_assert(false);
            }
        }
    }

    intermediary_values(sx, sy, sz, 
        vpz_base, vsv_base, epsilon_base, delta_base, phi_base, theta_base,
        ch1dxx_base, ch1dyy_base, ch1dzz_base, ch1dxy_base, ch1dyz_base, ch1dxz_base, 
        v2px_base, v2pz_base, v2sz_base, v2pn_base
    );

    medium_calc_intermediary_values(
        vpz_myimpl, vsv_myimpl, epsilon_myimpl, delta_myimpl, phi_myimpl, theta_myimpl,
        (FP**) ch1dxx_myimpl, (FP**) ch1dyy_myimpl, (FP**) ch1dzz_myimpl,
        (FP**) ch1dxy_myimpl, (FP**) ch1dyz_myimpl, (FP**) ch1dxz_myimpl, 
        (FP**) v2px_myimpl, (FP**) v2pz_myimpl, (FP**) v2sz_myimpl, (FP**) v2pn_myimpl
    );

    for(size_t k = 0; k < g_width_in_cubes; k++){
        for(size_t j = 0; j < g_width_in_cubes; j++){
            for(size_t i = 0; i < g_width_in_cubes; i++){
                //index the block
                const size_t b_i = block_idx(i, j , k);

                for(size_t z = 0; z < g_cube_width; z++){
                    for(size_t y = 0; y < g_cube_width; y++){
                        for(size_t x = 0; x < g_cube_width; x++){
                            const size_t c_i = cube_idx(x, y, z);
                            const size_t vol_i = volume_idx(x + i * g_cube_width, y + j * g_cube_width, z + k * g_cube_width);

                            cr_expect(epsilon_eq(CRIT_FP, ch1dxx_base[vol_i], ch1dxx_myimpl[b_i][c_i], EPSILON), 
                                "vpz at b(%d, %d, %d), c(%d, %d, %d)",i, j, k, x, y, z);

                            cr_expect(epsilon_eq(CRIT_FP, ch1dyy_base[vol_i], ch1dyy_myimpl[b_i][c_i], EPSILON), 
                                "vpz at b(%d, %d, %d), c(%d, %d, %d)",i, j, k, x, y, z);

                            cr_expect(epsilon_eq(CRIT_FP, ch1dzz_base[vol_i], ch1dzz_myimpl[b_i][c_i], EPSILON), 
                                "vpz at b(%d, %d, %d), c(%d, %d, %d)",i, j, k, x, y, z);

                            cr_expect(epsilon_eq(CRIT_FP, ch1dxy_base[vol_i], ch1dxy_myimpl[b_i][c_i], EPSILON), 
                                "vpz at b(%d, %d, %d), c(%d, %d, %d)",i, j, k, x, y, z);

                            cr_expect(epsilon_eq(CRIT_FP, ch1dyz_base[vol_i], ch1dyz_myimpl[b_i][c_i], EPSILON), 
                                "vpz at b(%d, %d, %d), c(%d, %d, %d)",i, j, k, x, y, z);

                            cr_expect(epsilon_eq(CRIT_FP, ch1dxz_base[vol_i], ch1dxz_myimpl[b_i][c_i], EPSILON), 
                                "vpz at b(%d, %d, %d), c(%d, %d, %d)",i, j, k, x, y, z);

                            cr_expect(epsilon_eq(CRIT_FP, v2px_base[vol_i], v2px_myimpl[b_i][c_i], EPSILON), 
                                "vpz at b(%d, %d, %d), c(%d, %d, %d)",i, j, k, x, y, z);

                            cr_expect(epsilon_eq(CRIT_FP, v2pz_base[vol_i], v2pz_myimpl[b_i][c_i], EPSILON), 
                                "vpz at b(%d, %d, %d), c(%d, %d, %d)",i, j, k, x, y, z);

                            cr_expect(epsilon_eq(CRIT_FP, v2sz_base[vol_i], v2sz_myimpl[b_i][c_i], EPSILON), 
                                "vpz at b(%d, %d, %d), c(%d, %d, %d)",i, j, k, x, y, z);

                            cr_expect(epsilon_eq(CRIT_FP, v2pn_base[vol_i], v2pn_myimpl[b_i][c_i], EPSILON), 
                                "vpz at b(%d, %d, %d), c(%d, %d, %d)",i, j, k, x, y, z);
                        }
                    }
                }
            }
        }
    }

    for(int i = 0; i < 10; i++){
        for(int seg = 0; seg < total_seg; seg++){
            free(buffs[i][seg]);
        }
    }
    free(ch1dxx_base);
    free(ch1dyy_base);
    free(ch1dzz_base);
    free(ch1dxy_base);
    free(ch1dyz_base);
    free(ch1dxz_base);
    free(v2px_base);
    free(v2pz_base);
    free(v2sz_base);
    free(v2pn_base);
}

Test(medium, kernel_proper_computation){
    const size_t seg = 3;
    const size_t c_size = 10;
    g_width_in_cubes = seg;
    g_cube_width = c_size;

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

    FP vpz[size];
    FP vsv[size];
    FP epsilon[size];
    FP delta[size];
    FP phi[size];
    FP theta[size];

    medium_initialize(TTI, size, vpz, vsv, epsilon, delta, phi, theta);

    medium_random_velocity_boundary(bord, absorb, vpz, vsv);

    const size_t total_seg = seg * seg * seg;
    const size_t total_c_size = c_size * c_size * c_size;

    FP* ch1dxx[total_seg];
    FP* ch1dyy[total_seg];
    FP* ch1dzz[total_seg];
    FP* ch1dxy[total_seg];
    FP* ch1dyz[total_seg];
    FP* ch1dxz[total_seg];
    FP* v2px[total_seg];
    FP* v2pz[total_seg];
    FP* v2sz[total_seg];
    FP* v2pn[total_seg];

    FP** buffs[10] = { ch1dxx, ch1dyy, ch1dzz, ch1dxy, ch1dyz, ch1dxz, v2px, v2pz, v2sz, v2pn };

    for(int i = 0; i < 10; i++){
        for(int seg = 0; seg < total_seg; seg++){
            if((buffs[i][seg] = (FP*) malloc(sizeof(FP) * total_c_size)) == NULL){
                cr_assert(false);
            }
        }
    }

    medium_calc_intermediary_values(
        vpz, vsv, epsilon, delta, phi, theta,
        (FP**) ch1dxx, (FP**) ch1dyy, (FP**) ch1dzz,
        (FP**) ch1dxy, (FP**) ch1dyz, (FP**) ch1dxz, 
        (FP**) v2px, (FP**) v2pz, (FP**) v2sz, (FP**) v2pn
    );

    for(size_t k = 0; k < g_width_in_cubes; k++){
        for(size_t j = 0; j < g_width_in_cubes; j++){
            for(size_t i = 0; i < g_width_in_cubes; i++){
                //index the block
                const size_t b_i = block_idx(i, j, k);

            }
        }
    }

    for(int i = 0; i < 10; i++){
        for(int seg = 0; seg < total_seg; seg++){
            free(buffs[i][seg]);
        }
    }
}


Test(medium, source_value){
    const float dt = 1.0;
    for(int i = 0; i < 100; i++){
        const FP proper = Source(dt, i + 1);
        const FP mine = medium_source_value(dt, i + 1);
        cr_expect(epsilon_eq(CRIT_FP, proper, mine, EPSILON), "i: %d", i);
    }
}
