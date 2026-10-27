#include <PR/os.h>

#include "platform/win/emulatedclock.h"

u64 osGetCount(void) {
    return AdvanceEmulatedOsTime();
}
