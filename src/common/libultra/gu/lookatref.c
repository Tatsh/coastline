#include <PR/gu.h>

#include "common/packedvalue.h"

void guLookAtReflectF(float mf[4][4],
                      LookAt *l,
                      float xEye,
                      float yEye,
                      float zEye,
                      float xAt,
                      float yAt,
                      float zAt,
                      float xUp,
                      float yUp,
                      float zUp) {
    guLookAtF(mf, xEye, yEye, zEye, xAt, yAt, zAt, xUp, yUp, zUp);
    l->l[0].l.dir[0] = ConvertDoubleToSignedByte(mf[0][0]);
    l->l[0].l.dir[1] = ConvertDoubleToSignedByte(mf[1][0]);
    l->l[0].l.dir[2] = ConvertDoubleToSignedByte(mf[2][0]);
    l->l[1].l.dir[0] = ConvertDoubleToSignedByte(mf[0][1]);
    l->l[1].l.dir[1] = ConvertDoubleToSignedByte(mf[1][1]);
    l->l[1].l.dir[2] = ConvertDoubleToSignedByte(mf[2][1]);
}
