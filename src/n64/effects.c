#include "common/effects.h"

#include <PR/gbi.h>
#include <PR/gu.h>
#include <PR/os_libc.h>

#include "common/particlecommands.h"
#include "common/race.h"
#include "common/render.h"
#include "common/system.h"
#include "common/track.h"
#include "common/trackselect.h"
#include "common/weather.h"

s32 ComputeScreenDepthZ(f32 z, f32 w) {
    return (s32)((z / w * (f32)g_dwViewportScaleZ + (f32)g_dwViewportTransZ) * 32.0f);
}
