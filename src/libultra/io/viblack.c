#include <ultra64.h>

#include "../os/osint.h"
#include "viint.h"

void osViBlack(u8 active) {
    register OSIntMask saveMask = __osDisableInt();

    if (active) {
        __osViNext->state |= VI_STATE_BLACK;
    } else {
        __osViNext->state &= ~VI_STATE_BLACK;
    }
    __osRestoreInt(saveMask);
}
