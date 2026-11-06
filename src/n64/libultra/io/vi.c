#include <ultra64.h>

#include "viint.h"

void __osViInit(void) {
    bzero(__osViContexts, sizeof(__osViContexts));
    __osViCurr = &__osViContexts[0];
    __osViNext = &__osViContexts[1];
    __osViNext->retraceCount = 1;
    __osViCurr->retraceCount = 1;
    __osViNext->framep = (void *)K0BASE;
    __osViCurr->framep = (void *)K0BASE;
    if (osTvType == OS_TV_PAL) {
        __osViNext->modep = &osViModePalLan1;
    } else if (osTvType == OS_TV_MPAL) {
        __osViNext->modep = &osViModeMpalLan1;
    } else {
        __osViNext->modep = &osViModeNtscLan1;
    }
    __osViNext->state = VI_STATE_BLACK;
    __osViNext->control = __osViNext->modep->comRegs.ctrl;
    /* The registers are written at the top of a field. */
    while (IO_READ(VI_CURRENT_REG) > 10) {
    }
    IO_WRITE(VI_STATUS_REG, 0);
    __osViSwapContext();
}
