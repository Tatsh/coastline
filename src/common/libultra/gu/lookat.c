#include <PR/gu.h>

#include "common/vecmath.h"

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
}
