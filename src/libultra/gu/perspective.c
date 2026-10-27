#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include <math.h>
#include <stdio.h>

#include <PR/gu.h>
#else
#include <ultra64.h>
#endif

#if TGR_PLATFORM == TGR_PLATFORM_WIN

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
#endif

void guPerspectiveF(
    float mf[4][4], u16 *perspNorm, float fovy, float aspect, float near, float far, float scale) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    long double halfHeight = tanl((long double)fovy * 0.008726646259971648) * near;
    float top = halfHeight;
    float right = halfHeight * aspect;

    guFrustumF(mf, -right, right, -top, top, near, far, scale);
    *perspNorm = 1;
#else
    float cot;
    int i, j;

    guMtxIdentF(mf);
    fovy *= 3.1415926 / 180.0;
    cot = cosf(fovy / 2) / sinf(fovy / 2);

    mf[0][0] = cot / aspect;
    mf[1][1] = cot;
    mf[2][2] = (near + far) / (near - far);
    mf[2][3] = -1.0f;
    mf[3][2] = (2 * near * far) / (near - far);
    mf[3][3] = 0.0f;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            mf[i][j] *= scale;
        }
    }

    if (perspNorm != NULL) {
        if (near + far <= 2.0) {
            *perspNorm = 0xFFFF;
        } else {
            *perspNorm = (u16)((2.0 * 65536.0) / (near + far));
            if (*perspNorm <= 0) {
                *perspNorm = 1;
            }
        }
    }
#endif
}

#if TGR_PLATFORM != TGR_PLATFORM_WIN
void guPerspective(
    Mtx *m, u16 *perspNorm, float fovy, float aspect, float near, float far, float scale) {
    float mf[4][4];

    guPerspectiveF(mf, perspNorm, fovy, aspect, near, far, scale);
    guMtxF2L(mf, m);
}
#endif
