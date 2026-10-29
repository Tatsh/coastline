#if TGR_BOSS_RALLY
#include <PR/gu.h>

#include "common/vecmath.h"
#else
#include <ultra64.h>
#endif

void guLookAtF(float mf[4][4],
               float xEye,
               float yEye,
               float zEye,
               float xAt,
               float yAt,
               float zAt,
               float xUp,
               float yUp,
               float zUp) {
#if TGR_BOSS_RALLY
    Vec3d look = {(long double)xEye - xAt, (long double)yEye - yAt, (long double)zEye - zAt};
    Vec3d up = {xUp, yUp, zUp};
    Vec3d right;
    long double dot;

    Vec3dNormalize(&look);
    dot = Vec3dDot(&up, &look);
    up.x = (long double)up.x - dot * look.x;
    up.y = (long double)up.y - dot * look.y;
    up.z = (long double)up.z - dot * look.z;
    Vec3dNormalize(&up);
    Vec3dCross(&up, &look, &right);

    mf[0][0] = right.x;
    mf[1][0] = right.y;
    mf[2][0] = right.z;
    mf[3][0] =
        -((long double)xEye * right.x) - (long double)yEye * right.y - (long double)zEye * right.z;
    mf[0][1] = up.x;
    mf[1][1] = up.y;
    mf[2][1] = up.z;
    mf[3][1] = -((long double)xEye * up.x) - (long double)yEye * up.y - (long double)zEye * up.z;
    mf[0][2] = look.x;
    mf[1][2] = look.y;
    mf[2][2] = look.z;
    mf[3][2] =
        -((long double)xEye * look.x) - (long double)yEye * look.y - (long double)zEye * look.z;
    mf[0][3] = 0.0f;
    mf[1][3] = 0.0f;
    mf[2][3] = 0.0f;
    mf[3][3] = 1.0f;
#else
    float len, xLook, yLook, zLook, xRight, yRight, zRight;

    guMtxIdentF(mf);

    xLook = xAt - xEye;
    yLook = yAt - yEye;
    zLook = zAt - zEye;
    /* The view looks down negative Z. */
    len = -1.0 / sqrtf(xLook * xLook + yLook * yLook + zLook * zLook);
    xLook *= len;
    yLook *= len;
    zLook *= len;

    xRight = yUp * zLook - zUp * yLook;
    yRight = zUp * xLook - xUp * zLook;
    zRight = xUp * yLook - yUp * xLook;
    len = 1.0 / sqrtf(xRight * xRight + yRight * yRight + zRight * zRight);
    xRight *= len;
    yRight *= len;
    zRight *= len;

    xUp = yLook * zRight - zLook * yRight;
    yUp = zLook * xRight - xLook * zRight;
    zUp = xLook * yRight - yLook * xRight;
    len = 1.0 / sqrtf(xUp * xUp + yUp * yUp + zUp * zUp);
    xUp *= len;
    yUp *= len;
    zUp *= len;

    mf[0][0] = xRight;
    mf[1][0] = yRight;
    mf[2][0] = zRight;
    mf[3][0] = -(xEye * xRight + yEye * yRight + zEye * zRight);

    mf[0][1] = xUp;
    mf[1][1] = yUp;
    mf[2][1] = zUp;
    mf[3][1] = -(xEye * xUp + yEye * yUp + zEye * zUp);

    mf[0][2] = xLook;
    mf[1][2] = yLook;
    mf[2][2] = zLook;
    mf[3][2] = -(xEye * xLook + yEye * yLook + zEye * zLook);

    mf[0][3] = 0.0f;
    mf[1][3] = 0.0f;
    mf[2][3] = 0.0f;
    mf[3][3] = 1.0f;
#endif /* TGR_BOSS_RALLY */
}

#if !TGR_BOSS_RALLY
void guLookAt(Mtx *m,
              float xEye,
              float yEye,
              float zEye,
              float xAt,
              float yAt,
              float zAt,
              float xUp,
              float yUp,
              float zUp) {
    float mf[4][4];

    guLookAtF(mf, xEye, yEye, zEye, xAt, yAt, zAt, xUp, yUp, zUp);
    guMtxF2L(mf, m);
}
#endif /* !TGR_BOSS_RALLY */
