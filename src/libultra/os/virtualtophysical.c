#include "osint.h"

u32 osVirtualToPhysical(void *addr) {
    if (IS_KSEG0(addr)) {
        return K0_TO_PHYS(addr);
    }
    if (IS_KSEG1(addr)) {
        return K1_TO_PHYS(addr);
    }
    return __osProbeTLB(addr);
}
