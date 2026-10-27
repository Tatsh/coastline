#include "frametime.h"
#include "vecmath.h"
#include "weather.h"

#if TGR_PLATFORM != TGR_PLATFORM_WIN
#include <PR/gu.h>
#endif

#define RANDOM_TO_TWO (2.0f / 65535.0f)
#define WIND_TURN 6.2831855f
#define WIND_STRENGTH_MAX 1.0f
#define WIND_STRENGTH_MIN 0.5f

#if TGR_PLATFORM == TGR_PLATFORM_WIN
/* The x87 instructions preserve extended results and return large angles unchanged. */
static inline long double WindCosine(f32 angle) {
    long double result;
    __asm__("fcos" : "=t"(result) : "0"((long double)angle));
    return result;
}

static inline long double WindSine(f32 angle) {
    long double result;
    __asm__("fsin" : "=t"(result) : "0"((long double)angle));
    return result;
}
#endif

void UpdateWindDirection(void) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    long double updatedAngle;
    long double updatedStrength;
    long double cosine;
    long double sine;

    updatedAngle =
        ((long double)(s32)(AdvanceRng() & 0xFFFF) * RANDOM_TO_TWO - 1.0f) * g_fFrameDeltaTime +
        g_fWindAngle;
    g_fWindAngle = updatedAngle;
    if (updatedAngle >= WIND_TURN) {
        g_fWindAngle = (long double)g_fWindAngle - WIND_TURN;
    } else if (!(g_fWindAngle >= 0.0f)) {
        g_fWindAngle = (long double)g_fWindAngle - -WIND_TURN;
    }
    updatedStrength =
        ((long double)(s32)(AdvanceRng() & 0xFFFF) * RANDOM_TO_TWO - 1.0f) * g_fFrameDeltaTime +
        g_fWindStrength;
    g_fWindStrength = updatedStrength;
    if (updatedStrength > WIND_STRENGTH_MAX) {
        g_fWindStrength = WIND_STRENGTH_MAX;
    } else if (!(g_fWindStrength >= WIND_STRENGTH_MIN)) {
        g_fWindStrength = WIND_STRENGTH_MIN;
    }
    g_vWindVelocity.z = 0.0f;
    cosine = WindCosine(g_fWindAngle);
    sine = WindSine(g_fWindAngle);
    g_vWindVelocity.x = cosine * g_fWindStrength * g_fFrameDeltaTime;
    g_vWindVelocity.y = sine * g_fWindStrength * g_fFrameDeltaTime;
#else
    g_fWindAngle += ((f32)(s32)(AdvanceRng() & 0xFFFF) * RANDOM_TO_TWO - 1.0f) * g_fFrameDeltaTime;
    if (g_fWindAngle >= WIND_TURN) {
        g_fWindAngle -= WIND_TURN;
    } else if (g_fWindAngle < 0.0f) {
        g_fWindAngle += WIND_TURN;
    }
    g_fWindStrength +=
        ((f32)(s32)(AdvanceRng() & 0xFFFF) * RANDOM_TO_TWO - 1.0f) * g_fFrameDeltaTime;
    if (g_fWindStrength > WIND_STRENGTH_MAX) {
        g_fWindStrength = WIND_STRENGTH_MAX;
    } else if (g_fWindStrength < WIND_STRENGTH_MIN) {
        g_fWindStrength = WIND_STRENGTH_MIN;
    }
    g_vWindVelocity.x = cosf(g_fWindAngle) * (g_fFrameDeltaTime * g_fWindStrength);
    g_vWindVelocity.y = sinf(g_fWindAngle) * (g_fFrameDeltaTime * g_fWindStrength);
    g_vWindVelocity.z = 0.0f;
#endif
}
