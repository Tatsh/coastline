#include "effects.h"

#include <PR/gbi.h>
#include <PR/gu.h>
#include <PR/os_libc.h>

#include "particlecommands.h"
#include "race.h"
#include "render.h"
#include "system.h"
#include "track.h"
#include "trackselect.h"
#include "weather.h"

s32 ComputeScreenDepthZ(f32 z, f32 w) {
    return (s32)((z / w * (f32)g_dwViewportScaleZ + (f32)g_dwViewportTransZ) * 32.0f);
}
