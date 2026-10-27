#include <PR/gu.h>

#include "frametime.h"
#include "race.h"
#include "render.h"
#include "screen.h"
#include "timing.h"
#include "tirespray.h"
#include "track.h"
#include "vecmath.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
Model *g_pStartLightsModel;
s32 g_dwStartLightsAnimation;
#define ANIMATION_MARKER(bar, red, green, blue, alpha) NullStub()
#else
#define ANIMATION_MARKER(bar, red, green, blue, alpha) AddTimingMarker(bar, red, green, blue, alpha)
#endif

/* The original saves the frame time only when the lights are held. A step that holds the lights
 * restores an uninitialised value. */
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
void AnimateStartLights(void) {
    f32 delta;

    if (g_dwStartLightsAnimation == 0) {
        return;
    }
    if (g_dwStartLightsAnimation == -1) {
        /* Held, the lights take their first pose without moving on. */
        delta = g_fFrameDeltaTime;
        g_fFrameDeltaTime = 0.0f;
    }
    AdvanceModelAnimations(g_pStartLightsModel);
    if (g_dwStartLightsAnimation == -1) {
        g_fFrameDeltaTime = delta;
    }
}
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

void UpdateRaceAnimations(void) {
    if (g_dwRacePauseState) {
        return;
    }
    ANIMATION_MARKER(0, 0x80, 0x80, 0xF0, 0xFF);
    AnimateStartLights();
    ANIMATION_MARKER(0, 0, 0, 0xC0, 0xFF);
    MoveTireSpray();
    ANIMATION_MARKER(0, 0, 0x82, 0, 0xFF);
}

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#define START_LIGHT_ROW(index) (&matrix.rows[index].vector)
#else
#define START_LIGHT_ROW(index) ((Vec3f *)m[index])
#endif

void RenderStartLights(void) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    union {
        f32 values[4][4];
        struct {
            Vec3f vector;
            f32 w;
        } rows[4];
    } matrix;
    f32(*m)[4] = matrix.values;
#else
    f32 m[4][4];
#endif

    if (!g_bStartLightsVisible || g_LoadedTrack.rootSection == NULL) {
        return;
    }
    /* Face the lights across the track from the right edge of its first point. */
    m[0][3] = 0.0f;
    m[1][3] = 0.0f;
    m[2][3] = 0.0f;
    m[2][0] = 0.0f;
    m[2][1] = 0.0f;
    m[3][3] = 1.0f;
    m[2][2] = 1.0f;
    Vec3fDirection(START_LIGHT_ROW(1),
                   &g_LoadedTrack.rootSection->points[0].position,
                   &g_LoadedTrack.rootSection->points[0].right);
    Vec3fCross(START_LIGHT_ROW(0), START_LIGHT_ROW(1), START_LIGHT_ROW(2));
    Vec3fCross(START_LIGHT_ROW(1), START_LIGHT_ROW(2), START_LIGHT_ROW(0));
    m[3][0] = g_LoadedTrack.rootSection->points[0].position.x;
    m[3][1] = g_LoadedTrack.rootSection->points[0].position.y;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    m[3][2] =
        ((long double)g_LoadedTrack.rootSection->points[0].position.z + g_fStartLightsDrop) + 2.0f;
#else
    m[3][2] = g_LoadedTrack.rootSection->points[0].position.z + 2.0f + g_fStartLightsDrop;
#endif
    Vec3fMulAdd(START_LIGHT_ROW(3), START_LIGHT_ROW(3), START_LIGHT_ROW(0), -0.3f);
    Vec3fMulAdd(START_LIGHT_ROW(3), START_LIGHT_ROW(3), START_LIGHT_ROW(1), -0.6f);
    gSPMatrix(gDisplayListPtr++, &g_mtxIdentity, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_PUSH);
    gSPMatrix(gDisplayListPtr++, g_pProjectionMtx, G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    guScaleF(g_afMvpMatrix, 1.0f / 1024.0f, 1.0f / 1024.0f, 1.0f / 1024.0f);
    guMtxCatF(g_afMvpMatrix, m, m);
    DrawModelList(g_pStartLightsModel, m);
}
