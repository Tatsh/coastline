#include <math.h>

#include <PR/gu.h>

void guRotateF(float mf[4][4], float a, float x, float y, float z) {
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
}
