#include <ultra64.h>

#include "../os/osint.h"
#include "viint.h"

void *osViGetCurrentFramebuffer(void) {
    register OSIntMask saveMask = __osDisableInt();
    void *framep = __osViCurr->framep;

    __osRestoreInt(saveMask);
    return framep;
}
