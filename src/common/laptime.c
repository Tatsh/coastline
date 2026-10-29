#include "common/laptime.h"

#if TGR_PLATFORM == TGR_PLATFORM_N64
#include <PR/os_libc.h>
#else
#include <stdio.h>
#endif

void FormatLapTime(char *out, f32 seconds) {
#if TGR_BOSS_RALLY
    long double scaled = (long double)seconds * 100.0f;
    s32 hundredths = scaled >= -0x1p63L && scaled < 0x1p63L ? (s32)(s64)scaled : 0;
#else
    s32 hundredths = (s32)(seconds * 100.0f);
#endif
    s32 whole = hundredths / 100;

    sprintf(out,
#if TGR_BOSS_RALLY
            "%d:%02d.%02d",
#else
            "%d'%02d\"%02d",
#endif
            whole / 60,
            whole - whole / 60 * 60,
            hundredths - whole * 100);
}
