/* Test of sinf, cosf and the fixed-point matrix conversion against the host libm, with sinf.c
 * compiled under -Dsinf=gu_sinf and cosf.c under -Dcosf=gu_cosf. */
#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "libultra/gu/guint.h"
const s16 sintable[0x400];
const double g_adSinfP[5] = {1.0,
                             -0.166666595504277565,
                             0.00833306624608215474,
                             -0.000198096029019379492,
                             2.60578063796803717e-06};
const double g_adCosfP[5] = {1.0,
                             -0.166666595504277565,
                             0.00833306624608215474,
                             -0.000198096029019379492,
                             2.60578063796803717e-06};
const double g_dSinfRpi = 0.318309886183790691, g_dCosfRpi = 0.318309886183790691;
const double g_dSinfPio2Hi = 3.14159262180328369, g_dCosfPio2Hi = 3.14159262180328369;
const double g_dSinfPio2Lo = 3.17865095470563921e-08, g_dCosfPio2Lo = 3.17865095470563921e-08;
const float g_fSinfZero = 0, g_fCosfZero = 0, __libm_qnan_f = NAN;
float gu_sinf(float), gu_cosf(float);
int main(void) {
    for (float x = -100.0f; x < 100.0f; x += 0.01f) {
        assert(fabs(gu_sinf(x) - sin(x)) < 1e-5);
        assert(fabs(gu_cosf(x) - cos(x)) < 1e-5);
    }
    float mf[4][4], back[4][4];
    Mtx m;
    for (int i = 0; i < 16; i++) {
        mf[i / 4][i % 4] = (i - 7) * 1.37f;
    }
    guMtxF2L(mf, &m);
    guMtxL2F(back, &m);
    for (int i = 0; i < 16; i++) {
        assert(fabsf(back[i / 4][i % 4] - mf[i / 4][i % 4]) < 1e-4f);
    }
    /* guRotateF() scales the angle by the radians in a degree, 0x3C8EFA35. About the z axis the
     * sine and the cosine land unchanged in the matrix, so they must equal those of that angle. */
    union {
        unsigned u;
        float f;
    } dtor = {0x3C8EFA35u};
    float angle = 90.0f * dtor.f;
    guRotateF(mf, 90.0f, 0.0f, 0.0f, 2.0f);
    assert(mf[0][1] == sinf(angle) && mf[1][0] == -sinf(angle) && mf[0][0] == cosf(angle));
    assert(fabsf(mf[0][1] - 1.0f) < 1e-5f && fabsf(mf[1][0] + 1.0f) < 1e-5f);
    assert(fabsf(mf[0][0]) < 1e-5f && fabsf(mf[2][2] - 1.0f) < 1e-5f);
    puts("gu check passed");
    return 0;
}
