#include "common/libultra/os/osint.h"

void __osResetGlobalIntMask(u32 mask) {
    register u32 saveMask = __osDisableInt();

    /* The RCP interrupt and the interrupt enable bit stay set. */
    __OSGlobalIntMask &= ~(mask & ~OS_IM_RCP);
    __osRestoreInt(saveMask);
}
