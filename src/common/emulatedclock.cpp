#include "common/emulatedclock.h"

namespace {

/**
 * The time of the console the game emulates, in ticks of its CPU counter.
 *
 * @ghidraAddress BRD3D: 0x118abde0
 * @ghidraAddress BRGlide: 0x118eeef8
 */
u64 g_qwEmulatedOsTime;

} // namespace

// The ticks of the CPU counter of the console in a frame of 1/30 of a second.
#define OS_TICKS_PER_FRAME (46875000 / 30)

u64 AdvanceEmulatedOsTime(void) {
    g_qwEmulatedOsTime += OS_TICKS_PER_FRAME;
    return g_qwEmulatedOsTime;
}
