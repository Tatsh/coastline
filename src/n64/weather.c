#include "common/weather.h"

#include <PR/gbi.h>
#include <PR/gu.h>

#include "common/audiomix.h"
#include "common/camera.h"
#include "common/render.h"
#include "common/system.h"
#include "common/timing.h"
#include "common/track.h"
#include "common/vehicle.h"

void SubdivideLightningBolt(s32 index, s32 span) {
    s32 mask = span * 8 - 1;
    Vec3f *point;
    f32 half = (f32)mask * 0.5f;

    g_avLightningBolt[index].x =
        (g_avLightningBolt[index - span].x + g_avLightningBolt[index + span].x) * 0.5f +
        (f32)(s32)(AdvanceRng() & mask) - half;
    g_avLightningBolt[index].y =
        (g_avLightningBolt[index - span].y + g_avLightningBolt[index + span].y) * 0.5f +
        (f32)(s32)(AdvanceRng() & mask) - half;
    point = &g_avLightningBolt[index];
    point->z = (g_avLightningBolt[index - span].z + g_avLightningBolt[index + span].z) * 0.5f;
    span >>= 1;
    if (span != 0) {
        g_nLightningBoltDepth += 4;
        SubdivideLightningBolt(index - span, span);
        SubdivideLightningBolt(index + span, span);
        g_nLightningBoltDepth -= 4;
    }
}

void StubWeatherUpdate(void) {
}
