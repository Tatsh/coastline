#include <ultra64.h>

#include "piint.h"

s32 osEPiRawStartDma(OSPiHandle *pihandle, s32 direction, u32 devAddr, void *dramAddr, u32 size) {
    u32 stat;
    u32 domain;
    OSPiHandle *curHandle;

    WAIT_ON_IOBUSY(stat);
    domain = pihandle->domain;
    curHandle = __osCurrentHandle[domain];
    if (curHandle != pihandle) {
        if (domain == PI_DOMAIN1) {
            if (curHandle->latency != pihandle->latency) {
                IO_WRITE(PI_BSD_DOM1_LAT_REG, pihandle->latency);
            }
            if (curHandle->pageSize != pihandle->pageSize) {
                IO_WRITE(PI_BSD_DOM1_PGS_REG, pihandle->pageSize);
            }
            if (curHandle->relDuration != pihandle->relDuration) {
                IO_WRITE(PI_BSD_DOM1_RLS_REG, pihandle->relDuration);
            }
            if (curHandle->pulse != pihandle->pulse) {
                IO_WRITE(PI_BSD_DOM1_PWD_REG, pihandle->pulse);
            }
        } else {
            if (curHandle->latency != pihandle->latency) {
                IO_WRITE(PI_BSD_DOM2_LAT_REG, pihandle->latency);
            }
            if (curHandle->pageSize != pihandle->pageSize) {
                IO_WRITE(PI_BSD_DOM2_PGS_REG, pihandle->pageSize);
            }
            if (curHandle->relDuration != pihandle->relDuration) {
                IO_WRITE(PI_BSD_DOM2_RLS_REG, pihandle->relDuration);
            }
            if (curHandle->pulse != pihandle->pulse) {
                IO_WRITE(PI_BSD_DOM2_PWD_REG, pihandle->pulse);
            }
        }
        __osCurrentHandle[domain] = pihandle;
    }
    IO_WRITE(PI_DRAM_ADDR_REG, osVirtualToPhysical(dramAddr));
    IO_WRITE(PI_CART_ADDR_REG, K1_TO_PHYS(pihandle->baseAddress | devAddr));
    switch (direction) {
    case OS_READ:
        IO_WRITE(PI_WR_LEN_REG, size - 1);
        break;
    case OS_WRITE:
        IO_WRITE(PI_RD_LEN_REG, size - 1);
        break;
    default:
        return -1;
    }
    return 0;
}
