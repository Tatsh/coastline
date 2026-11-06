#include <ultra64.h>

s32 __osAiDeviceBusy(void) {
    register s32 status = IO_READ(AI_STATUS_REG);

    if (status & AI_STATUS_FIFO_FULL) {
        return TRUE;
    }
    return FALSE;
}

u32 osAiGetStatus(void) {
    return IO_READ(AI_STATUS_REG);
}

u32 osAiGetLength(void) {
    return IO_READ(AI_LEN_REG);
}

s32 osAiSetFrequency(u32 frequency) {
    register u32 dacRate;
    register u8 bitRate;
    register f32 f;

    f = osViClock / (f32)frequency + 0.5f;
    dacRate = f;
    if (dacRate < AI_MIN_DAC_RATE) {
        return -1;
    }
    bitRate = dacRate / 66;
    if (bitRate > AI_MAX_BIT_RATE) {
        bitRate = AI_MAX_BIT_RATE;
    }
    IO_WRITE(AI_DACRATE_REG, dacRate - 1);
    IO_WRITE(AI_BITRATE_REG, bitRate - 1);
    IO_WRITE(AI_CONTROL_REG, AI_CONTROL_DMA_ON);
    return osViClock / (s32)dacRate;
}

s32 osAiSetNextBuffer(void *bufPtr, u32 size) {
    char *bptr = bufPtr;

    if (aiHdwrBugFlag != 0) {
        bptr -= 0x2000;
    }
    if ((((u32)bufPtr + size) & 0x3FFF) == 0x2000) {
        aiHdwrBugFlag = 1;
    } else {
        aiHdwrBugFlag = 0;
    }
    if (__osAiDeviceBusy()) {
        return -1;
    }
    IO_WRITE(AI_DRAM_ADDR_REG, osVirtualToPhysical(bptr));
    IO_WRITE(AI_LEN_REG, size);
    return 0;
}
