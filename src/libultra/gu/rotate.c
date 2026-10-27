#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include <math.h>

#include <PR/gu.h>
#else
#include "guint.h"
#endif

void guRotateF(float mf[4][4], float a, float x, float y, float z) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    float basis[4][4], transpose[4][4], rotation[4][4];
    long double angle;
    double cosineAngle;
    float sine, cosine;
    int i, j;

    if ((x == 0.0f || isnan(x)) && (y == 0.0f || isnan(y)) && (z == 0.0f || isnan(z))) {
        guMtxIdentF(mf);
        return;
    }
    guLookAtF(basis, x, y, z, 0, 0, 0, y, z, x);
    basis[0][3] = 0.0f;
    basis[1][3] = 0.0f;
    basis[2][3] = 0.0f;
    basis[3][0] = 0.0f;
    basis[3][1] = 0.0f;
    basis[3][2] = 0.0f;
    basis[3][3] = 1.0f;
    for (i = 0; i < 4; ++i) {
        for (j = 0; j < 4; ++j) {
            transpose[i][j] = basis[j][i];
        }
    }
    angle = (long double)a * 0.017453292519943295;
    cosineAngle = angle;
    sine = sinl(angle);
    cosine = cosl((long double)cosineAngle);
    guMtxIdentF(rotation);
    rotation[0][0] = cosine;
    rotation[0][1] = sine;
    rotation[1][0] = -sine;
    rotation[1][1] = cosine;
    guMtxCatF(basis, rotation, mf);
    guMtxCatF(mf, transpose, mf);
#else
    /* IDO stores the radians in a degree on each call before it reads them back. */
    /**
     * @ghidraAddress USA: 0x8037f0a0
     * @ghidraAddress EUR: 0x80380fe0
     * @ghidraAddress JPN: 0x8037fc50
     */
    static float g_fGuRotateDtor = 3.1415926 / 180.0;
    float sine, cosine, ab, bc, ca, t;

    guNormalize(&x, &y, &z);
    a *= g_fGuRotateDtor;
    sine = sinf(a);
    cosine = cosf(a);
    t = 1.0f - cosine;
    ab = x * y * t;
    bc = y * z * t;
    ca = z * x * t;
    guMtxIdentF(mf);

    t = x * x;
    mf[0][0] = t + cosine * (1.0f - t);
    mf[2][1] = bc - x * sine;
    mf[1][2] = bc + x * sine;

    t = y * y;
    mf[1][1] = t + cosine * (1.0f - t);
    mf[2][0] = ca + y * sine;
    mf[0][2] = ca - y * sine;

    t = z * z;
    mf[2][2] = t + cosine * (1.0f - t);
    mf[1][0] = ab - z * sine;
    mf[0][1] = ab + z * sine;
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
}

#if TGR_PLATFORM != TGR_PLATFORM_WIN
void guRotate(Mtx *m, float a, float x, float y, float z) {
    float mf[4][4];

    guRotateF(mf, a, x, y, z);
    guMtxF2L(mf, m);
}
#endif
