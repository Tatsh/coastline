#include <PR/gu.h>

void guTranslateF(float mf[4][4], float x, float y, float z) {
    guMtxIdentF(mf);
    mf[3][0] = x;
    mf[3][1] = y;
    mf[3][2] = z;
}
