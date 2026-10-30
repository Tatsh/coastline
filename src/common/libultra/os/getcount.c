#include <PR/os.h>

#include "common/emulatedclock.h"

u64 osClockRate = 2165625000ULL;

u64 osGetCount(void) {
    return AdvanceEmulatedOsTime();
}
