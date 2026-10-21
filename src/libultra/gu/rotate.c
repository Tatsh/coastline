#include "guint.h"

void guRotateF(float mf[4][4], float a, float x, float y, float z) {
    /* IDO stores the radians in a degree on each call before it reads them back. */
    /**
     * @ghidraAddress USA: 0x8037f0a0
     * @ghidraAddress JPN: 0x8037fc50
     * @ghidraAddress EUR: 0x80380fe0
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
}

void guRotate(Mtx *m, float a, float x, float y, float z) {
    float mf[4][4];

    guRotateF(mf, a, x, y, z);
    guMtxF2L(mf, m);
}
