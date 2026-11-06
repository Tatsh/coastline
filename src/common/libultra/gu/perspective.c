#include <math.h>
#include <stdio.h>

#include <PR/gu.h>

void guFrustumF(float mf[4][4],
                float left,
                float right,
                float bottom,
                float top,
                float near,
                float far,
                float scale) {
    float width, height, depth, twiceNear;
    int i, j;

    (void)scale;
    if (left == right || bottom == top || near == far || isnan(left) || isnan(right) ||
        isnan(bottom) || isnan(top) || isnan(near) || isnan(far)) {
        printf("Error: guFrustumF(): unable to compute matrix\n");
        return;
    }
    width = (long double)right - left;
    height = (long double)top - bottom;
    depth = (long double)far - near;
    twiceNear = (long double)near + near;
    for (i = 0; i < 4; ++i) {
        for (j = 0; j < 4; ++j) {
            mf[i][j] = 0.0f;
        }
    }
    mf[0][0] = ((long double)near + near) / width;
    mf[2][0] = ((long double)right + left) / width;
    mf[1][1] = (long double)twiceNear / height;
    mf[2][1] = ((long double)top + bottom) / height;
    mf[2][2] = -(((long double)far + near) / depth);
    mf[2][3] = -1.0f;
    mf[3][2] = -(((long double)far * near + (long double)far * near) / depth);
}

void guPerspectiveF(
    float mf[4][4], u16 *perspNorm, float fovy, float aspect, float near, float far, float scale) {
    long double halfHeight = tanl((long double)fovy * 0.008726646259971648) * near;
    float top = halfHeight;
    float right = halfHeight * aspect;

    guFrustumF(mf, -right, right, -top, top, near, far, scale);
    *perspNorm = 1;
}
