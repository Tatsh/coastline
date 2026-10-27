#include <PR/gu.h>

#if TGR_PLATFORM != TGR_PLATFORM_WIN
void guMtxXFMF(float mf[4][4], float x, float y, float z, float *ox, float *oy, float *oz) {
    *ox = mf[0][0] * x + mf[1][0] * y + mf[2][0] * z + mf[3][0];
    *oy = mf[0][1] * x + mf[1][1] * y + mf[2][1] * z + mf[3][1];
    *oz = mf[0][2] * x + mf[1][2] * y + mf[2][2] * z + mf[3][2];
}
#endif

void guMtxCatF(float mf[4][4], float nf[4][4], float res[4][4]) {
    int i, j;
#if TGR_PLATFORM != TGR_PLATFORM_WIN
    int k;
#endif
    float temp[4][4];

#if TGR_PLATFORM == TGR_PLATFORM_WIN
    int aliases;
    float (*dest)[4];

    if (!mf || !nf) {
        return;
    }
    aliases = res == mf || res == nf;
    dest = aliases ? temp : res;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            long double a = (long double)mf[i][0] * nf[0][j];
            long double b = (long double)mf[i][1] * nf[1][j];
            long double c = (long double)mf[i][2] * nf[2][j];
            long double d = (long double)mf[i][3] * nf[3][j];
            /* The two x87 paths add the products in different orders. */
            dest[i][j] = (float)(aliases ? c + (a + (d + b)) : b + (a + (c + d)));
        }
    }
    if (!aliases) {
        return;
    }
#else
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            temp[i][j] = 0.0f;
            for (k = 0; k < 4; k++) {
                temp[i][j] += mf[i][k] * nf[k][j];
            }
        }
    }
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
    /* The product goes through a copy so that the result may alias an operand. */
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            res[i][j] = temp[i][j];
        }
    }
}
