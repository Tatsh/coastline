#include "common/mathutil.h"

#if TGR_PLATFORM != TGR_PLATFORM_N64 || (!defined(__mips__) && !defined(__sgi))
s32 FloatToInt(float value) {
#if TGR_BOSS_RALLY
#if defined(ENABLE_PATCHES) && TGR_PLATFORM == TGR_PLATFORM_SDL
    /* The original x87 FISTP stores the 64-bit indefinite integer on invalid conversion.
     * FloatToInt returns its low word, zero. */
    if (!(value >= -9223372036854775808.0f && value < 9223372036854775808.0f)) {
        return 0;
    }
#endif
    return (s32)(s64)value;
#else
    return (s32)value;
#endif
}
#endif
