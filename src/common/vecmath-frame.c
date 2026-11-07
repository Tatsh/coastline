#include "common/vecmath.h"

#if TGR_BOSS_RALLY && defined(_MSC_VER) && !defined(_WIN64)
void __fastcall FinishFrameMatrix(f32 m[4][4]) {
#else
void FinishFrameMatrix(f32 m[4][4]) {
#endif
    m[2][3] = 0.0f;
    m[1][3] = 0.0f;
    m[0][3] = 0.0f;
    m[3][3] = 1.0f;
}
