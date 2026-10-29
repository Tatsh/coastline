#include "common/libultra/os/osint.h"

void __osSetGlobalIntMask(u32 mask) {
    register u32 saveMask = __osDisableInt();

    __OSGlobalIntMask |= mask;
    __osRestoreInt(saveMask);
}
