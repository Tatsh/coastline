#include <ultra64.h>

#include "../os/osint.h"
#include "viint.h"

void osViSwapBuffer(void *frameBufPtr) {
    OSIntMask saveMask = __osDisableInt();

    __osViNext->framep = frameBufPtr;
    __osViNext->state |= VI_STATE_BUFFER_UPDATED;
    __osRestoreInt(saveMask);
}
