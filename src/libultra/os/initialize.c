#include "common/libultra/io/siint.h"
#include "common/libultra/os/osint.h"

u64 osClockRate = OS_CLOCK_RATE;
s32 osViClock = VI_NTSC_CLOCK;
/**
 * @ghidraAddress USA: 0x802a614c
 * @ghidraAddress EUR: 0x802a7c5c
 * @ghidraAddress JPN: 0x802a779c
 */
u32 __osShutdown = 0;
u32 __OSGlobalIntMask = OS_IM_ALL;

/* The four exception vectors, each of which receives a copy of __osExceptionPreamble(). */
#define UT_VEC 0x80000000
#define XUT_VEC 0x80000080
#define ECC_VEC 0x80000100
#define E_VEC 0x80000180

typedef struct {
    u32 inst1; /*!< The first instruction of the vector, copied from __osExceptionPreamble(). */
    u32 inst2; /*!< The second instruction of the vector. */
    u32 inst3; /*!< The third instruction of the vector. */
    u32 inst4; /*!< The fourth instruction of the vector. */
} __osExceptionVector;

void osInitialize(void) {
    u32 pifdata;
    u32 clock = 0;

    __osFinalrom = TRUE;
    __osSetSR(__osGetSR() | SR_CU1);
    __osSetFpcCsr(FPCSR_FS | FPCSR_EV);

    /* Tell the PIF that the game has started, so that it does not reset the console. */
    while (__osSiRawReadIo(PIF_RAM_END - 3, &pifdata)) {
    }
    while (__osSiRawWriteIo(PIF_RAM_END - 3, pifdata | 8)) {
    }

    *(__osExceptionVector *)UT_VEC = *(__osExceptionVector *)__osExceptionPreamble;
    *(__osExceptionVector *)XUT_VEC = *(__osExceptionVector *)__osExceptionPreamble;
    *(__osExceptionVector *)ECC_VEC = *(__osExceptionVector *)__osExceptionPreamble;
    *(__osExceptionVector *)E_VEC = *(__osExceptionVector *)__osExceptionPreamble;
    osWritebackDCache((void *)UT_VEC, E_VEC - UT_VEC + sizeof(__osExceptionVector));
    osInvalICache((void *)UT_VEC, E_VEC - UT_VEC + sizeof(__osExceptionVector));
    osMapTLBRdb();

    /* The cartridge header can override the CPU clock rate. */
    osPiRawReadIo(4, &clock);
    clock &= ~0xF;
    if (clock != 0) {
        osClockRate = clock;
    }
    osClockRate = osClockRate * 3 / 4;

    if (osResetType == 0) {
        bzero(osAppNMIBuffer, sizeof(osAppNMIBuffer));
    }

    if (osTvType == OS_TV_PAL) {
        osViClock = VI_PAL_CLOCK;
    } else if (osTvType == OS_TV_MPAL) {
        osViClock = VI_MPAL_CLOCK;
    } else {
        osViClock = VI_NTSC_CLOCK;
    }
}
