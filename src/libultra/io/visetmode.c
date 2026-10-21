#include <ultra64.h>

#include "../os/osint.h"
#include "viint.h"

void osViSetMode(OSViMode *modep) {
    register OSIntMask saveMask = __osDisableInt();

    __osViNext->modep = modep;
    __osViNext->state = VI_STATE_MODE_UPDATED;
    __osViNext->control = __osViNext->modep->comRegs.ctrl;
    __osRestoreInt(saveMask);
}
