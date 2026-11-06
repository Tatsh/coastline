#include <PR/gu.h>

void guMtxCatF(float mf[4][4], float nf[4][4], float res[4][4]) {
    int i, j;
    float temp[4][4];
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
    /* The product goes through a copy so that the result may alias an operand. */
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            res[i][j] = temp[i][j];
        }
    }
}
