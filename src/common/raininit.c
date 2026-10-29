#include "common/vecmath.h"
#include "common/weather.h"

#if TGR_PC
/** @ghidraAddress BRD3D: 0x104b2550 */
s32 g_dwRainParticleCount;
#endif /* TGR_PC */

void InitRandomRainParticles(void) {
    s32 view, i;

    g_dwRainParticleCount = RAIN_PARTICLES;
    for (view = 0; view < WEATHER_VIEWS; view++) {
        for (i = 0; i < RAIN_PARTICLES; i++) {
            g_awRainParticlePos[view][i][0] = AdvanceRng();
            g_awRainParticlePos[view][i][1] = AdvanceRng();
            g_awRainParticlePos[view][i][2] = AdvanceRng();
        }
    }
}
