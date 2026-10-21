#include <ultra64.h>

void __osSpSetStatus(u32 data) {
    IO_WRITE(SP_STATUS_REG, data);
}

s32 __osSpSetPc(u32 pc) {
    register u32 status = IO_READ(SP_STATUS_REG);

    if (!(status & SP_STATUS_HALT)) {
        return -1;
    }
    IO_WRITE(SP_PC_REG, pc);
    return 0;
}

u32 __osSpDeviceBusy(void) {
    register u32 stat = IO_READ(SP_STATUS_REG);

    if (stat & (SP_STATUS_DMA_BUSY | SP_STATUS_DMA_FULL | SP_STATUS_IO_FULL)) {
        return 1;
    }
    return 0;
}

s32 __osSpRawStartDma(s32 direction, u32 devAddr, void *dramAddr, u32 size) {
    if (__osSpDeviceBusy()) {
        return -1;
    }
    IO_WRITE(SP_MEM_ADDR_REG, devAddr);
    IO_WRITE(SP_DRAM_ADDR_REG, osVirtualToPhysical(dramAddr));
    if (direction == OS_READ) {
        IO_WRITE(SP_WR_LEN_REG, size - 1);
    } else {
        IO_WRITE(SP_RD_LEN_REG, size - 1);
    }
    return 0;
}
