#include "frametime.h"
#include "race.h"
#include "screen.h"
#include "timing.h"
#include "tirespray.h"

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
