#include "common/vecmath.h"

void FinishFrameMatrix(f32 m[4][4]) {
    m[2][3] = 0.0f;
    m[1][3] = 0.0f;
    m[0][3] = 0.0f;
    m[3][3] = 1.0f;
}
