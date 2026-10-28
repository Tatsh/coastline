#include "frametime.h"
#include "track.h"
#include "vecmath.h"
#include "weather.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
/**
 * The time since the last strike, in 343 units a second.
 *
 * @ghidraAddress USA: 0x8028c820
 * @ghidraAddress EUR: 0x8028ef80
 * @ghidraAddress JPN: 0x8028e9d0
 * @ghidraAddress BRD3D: 0x104b255c
 * @ghidraAddress BRGlide: 0x104add3c
 */
static f32 g_fLightningFlashTimer;
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

#define LIGHTNING_TIMER_RATE 343.0f
#define LIGHTNING_TIMER_LIMIT 2048.0
#define LIGHTNING_CHANCE 0x80
#define LIGHTNING_FLASH_FRAMES 3
#define LIGHTNING_AREA_MASK 0x7FF

void UpdateLightningFlash(void) {
    if (g_dwLightningFlashCounter >= 0) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        g_fLightningFlashTimer = (long double)g_fLightningFlashTimer -
                                 (long double)g_fFrameDeltaTime * -LIGHTNING_TIMER_RATE;
#else
        g_fLightningFlashTimer += LIGHTNING_TIMER_RATE * g_fFrameDeltaTime;
#endif
        if (g_dwLightningFlashCounter > 0) {
            g_dwLightningFlashCounter--;
        } else if (g_fLightningFlashTimer > LIGHTNING_TIMER_LIMIT) {
            g_dwLightningFlashCounter = -1;
        }
    } else if ((s32)(AdvanceRng() & 0xFFFF) < LIGHTNING_CHANCE) {
        g_dwLightningFlashCounter = LIGHTNING_FLASH_FRAMES;
        g_fLightningFlashTimer = 0.0f;
        g_avLightningBolt[0].x = (f32)(s32)(AdvanceRng() & LIGHTNING_AREA_MASK);
        g_avLightningBolt[0].y = (f32)(s32)(AdvanceRng() & LIGHTNING_AREA_MASK);
        g_avLightningBolt[0].z = g_LoadedTrack.minZ;
    }
}
