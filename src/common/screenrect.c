#include "common/render.h"
#include "common/trackrender.h"

#if !TGR_BOSS_RALLY
void ProjectWorldToScreenRect(const Vec3f *point, s32 size, s16 low[2], s16 high[2]) {
    f32 clip[4];
    f32 upper[4];
    s32 width;
    s32 height;
    f32 inverse;
    f32 radius;
    s32 centerX;
    s32 centerY;
    s32 halfWidth;
    s32 halfHeight;

    width = g_aViewportRects[g_dwCurrentSplitScreenIndex].width;
    height = g_aViewportRects[g_dwCurrentSplitScreenIndex].height;
    centerX = g_aViewportRects[g_dwCurrentSplitScreenIndex].x + (width >> 1);
    centerY = g_aViewportRects[g_dwCurrentSplitScreenIndex].y + (height >> 1);
    halfWidth = width >> 1;
    halfHeight = height >> 1;
    Vec3fTransformByMatrix(clip, point, g_afViewProjMatrix);
    if (!(clip[3] <= 0.001f && -0.001f <= clip[3])) {
        inverse = 1.0f / clip[3];
        if (g_dwTrackMirrored - g_dwViewportMirrored) {
            clip[0] *= -inverse;
        } else {
            clip[0] *= inverse;
        }
        clip[1] *= inverse;
        radius = size * inverse;
        upper[0] = clip[0] + radius;
        clip[0] = clip[0] - radius;
        upper[1] = clip[1] + radius;
        clip[1] = clip[1] - radius;
        low[0] = (s32)(clip[0] * halfWidth) + centerX;
        low[1] = centerY - (s32)(clip[1] * halfHeight);
        high[0] = (s32)(upper[0] * halfWidth) + centerX;
        high[1] = centerY - (s32)(upper[1] * halfHeight);
    }
}
#else
void ProjectWorldToScreenRect(
    const ViewportRect *viewports, const Vec3f *point, s32 size, s16 low[2], s16 high[2]) {
    const ViewportRect *view = &viewports[g_dwCurrentSplitScreenIndex];
    f32 clip[4];
    f32 upper[2];
    long double inverse, radius, projectedX, projectedY, lowerX, lowerY;
    f32 halfWidth, halfHeight;
    s32 halfWidthPixels = view->width >> 1;
    s32 halfHeightPixels = view->height >> 1;
    s32 centerX = (s32)((u32)view->x + (u32)halfWidthPixels);
    s32 centerY = (s32)((u32)view->y + (u32)halfHeightPixels);
    halfWidth = (f32)halfWidthPixels;
    halfHeight = (f32)halfHeightPixels;

    Vec3fTransformByMatrix(clip, point, g_afViewProjMatrix);
    if (clip[3] <= 0.001f && clip[3] >= -0.001f) {
        return;
    }
    inverse = 1.0L / clip[3];
    projectedX = inverse * clip[0];
    if (g_dwTrackMirrored != g_dwViewportMirrored) {
        projectedX = -projectedX;
    }
    clip[0] = projectedX;
    projectedY = inverse * clip[1];
    radius = (long double)size * inverse;
    /* Lower bounds retain extended precision; upper bounds round before scaling. */
    upper[0] = (long double)clip[0] + radius;
    lowerX = (long double)clip[0] - radius;
    upper[1] = projectedY + radius;
    lowerY = projectedY - radius;
    low[0] = (s16)((u32)(s64)(lowerX * halfWidth) + (u32)centerX);
    low[1] = (s16)((u32)centerY - (u32)(s64)(lowerY * halfHeight));
    high[0] = (s16)((u32)(s64)((long double)upper[0] * halfWidth) + (u32)centerX);
    high[1] = (s16)((u32)centerY - (u32)(s64)((long double)upper[1] * halfHeight));
}
#endif /* TGR_BOSS_RALLY */
