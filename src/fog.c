#include "render.h"
#include "weather.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
f32 g_afViewProjMatrix[4][4];
s32 g_dwFogFactorOffset;
s32 g_dwFogFactorScale;
#endif

f32 ComputeFogFactorAtPoint(Vec3f *point) {
    f32 depth;
    f32 clip[4];
    f32 fog;

    if (!g_dwWeatherActive) {
        return 0.0f;
    }
    Vec3fTransformByMatrix(clip, point, g_afViewProjMatrix);
    depth = clip[2];
    depth /= clip[3];
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    fog = ((long double)depth * (f32)g_dwFogFactorScale + (f32)g_dwFogFactorOffset) *
          (f32)(1.0 / 255.0);
    if (!(fog >= 0.0f)) {
        return 0.0f;
    }
#else
    /* The cast is redundant, and the original compiles with the depth as the first factor. */
    fog = ((f32)depth * g_dwFogFactorScale + g_dwFogFactorOffset) * (1.0 / 255.0);
    if (fog < 0.0f) {
        return 0.0f;
    }
#endif
    if (1.0f < fog) {
        return 1.0f;
    }
    return fog;
}
