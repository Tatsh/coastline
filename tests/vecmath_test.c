/* Host check of src/vecmath.c against the C library and known answers. */
#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "common/vecmath.h"

u32 g_dwRandomSeed;
static const char *lastMessage;
void SetDebugMessage(const char *message) {
    lastMessage = message;
}

static int near(f32 a, f32 b, f32 tolerance) {
    return fabsf(a - b) <= tolerance;
}

int main(void) {
    f32 m[4][4] = {{0, 1, 0, 0}, {-1, 0, 0, 0}, {0, 0, 2, 0}, {3, 4, 5, 1}};
    f32 inv[4][4], product[4][4];
    Vec3f a = {1, 2, 3}, b = {4, 5, 6}, out, axis = {0, 0, 1};
    Vec3f v0 = {0, 0, 0}, v1 = {1, 0, 0}, v2 = {0, 1, 0}, up = {0, 0, 1}, p = {0.2f, 0.2f, 0};
    Vec2f s1 = {0, 0}, s2 = {2, 2}, s3 = {0, 2}, s4 = {2, 0}, s5 = {3, 3}, s6 = {4, 4};
    Vec3f t1 = {0, 0, 9}, t2 = {2, 2, 9}, t3 = {0, 2, 9}, t4 = {2, 0, 9}, t5 = {3, 3, 9},
          t6 = {4, 4, 9};
    s32 i, j;
    f32 angle;

    assert(near(Powf(2.0f, 3.0f), 8.0f, 1e-5f));
    assert(near(Powf(1.2f, 2.5f), powf(1.2f, 2.5f), 1e-4f));
    for (angle = -3.0f; angle < 3.0f; angle += 0.37f) {
        assert(
            near(Atan2f(cosf(angle), sinf(angle)), angle < 0 ? angle + 6.2831853f : angle, 0.02f));
    }
    assert(Atan2f(0, 0) == 0.0f);

    Vec3fCross(&out, &a, &b);
    assert(out.x == -3 && out.y == 6 && out.z == -3);
    assert(Vec3fDot(&a, &b) == 32);
    Vec3fLerp(&out, &a, &b, 0.25f);
    assert(out.x == 3.25f);
    Vec3fMulAdd(&out, &a, &b, 2);
    assert(out.x == 9 && out.z == 15);
    out = (Vec3f){0, 0, 0};
    Vec3fNormalize(&out);
    assert(out.z == 1);

    out = (Vec3f){1, 0, 0};
    Vec3fRotateAroundAxis(&out, 1.5707963f, &axis);
    /* The game turns clockwise as seen from the tip of the axis. */
    assert(near(out.x, 0, 1e-5f) && near(out.y, -1, 1e-5f));

    assert(MtxInvert4x4(inv, m) == 1);
    Mtx4x4Multiply(product, m, inv);
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            assert(near(product[i][j], i == j, 1e-5f));
        }
    }
    {
        f32 singular[4][4] = {{0}};
        lastMessage = NULL;
        assert(MtxInvert4x4(inv, singular) == 0 && lastMessage != NULL);
    }

    assert(PointInTriangle3D(&p, &v0, &v1, &v2, &up));
    p.x = 0.9f;
    p.y = 0.9f;
    assert(!PointInTriangle3D(&p, &v0, &v1, &v2, &up));

    assert(TestLineSegmentIntersection(&s1, &s2, &s3, &s4) == 1);
    assert(TestLineSegmentIntersection(&s1, &s2, &s5, &s6) == 0);
    assert(SegmentCrossesLine2D(&t1, &t2, &t3, &t4) == 1);
    assert(SegmentCrossesLine2D(&t1, &t2, &t5, &t6) == 2);

    SetRngSeed(1);
    assert(AdvanceRng() == 16807 && AdvanceRng() == ((16807u * 16807u) & 0x7FFFFFF));
    puts("vecmath ok");
    return 0;
}
