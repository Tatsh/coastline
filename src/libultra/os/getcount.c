#include <PR/os.h>

#include "win32/emulatedclock.h"

u64 osClockRate = 2165625000ULL;

u64 osGetCount(void) {
    return AdvanceEmulatedOsTime();
}
