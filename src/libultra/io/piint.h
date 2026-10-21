#ifndef LIBULTRA_IO_PIINT_H
#define LIBULTRA_IO_PIINT_H

/**
 * @file
 * The PI manager, the PI access queue, and the PI handles of the cartridge and the 64DD.
 */

#include <ultra64.h>

/** @ghidraAddress USA: 0x802a62e0 */
extern u32 __osPiAccessQueueEnabled;

/** @ghidraAddress USA: 0x803819e8 */
extern OSMesgQueue __osPiAccessQueue;

/** @ghidraAddress USA: 0x803819e0 */
extern OSMesg piAccessBuf[1];

/** @ghidraAddress USA: 0x802a61a0 */
extern OSPiHandle *__osCurrentHandle[2];

/** @ghidraAddress USA: 0x802a6180 */
extern OSDevMgr __osPiDevMgr;

/** @ghidraAddress USA: 0x802a619c */
extern OSPiHandle *__osPiTable;

/** @ghidraAddress USA: 0x803806b0 */
extern OSThread piThread;

/** @ghidraAddress USA: 0x80380860 */
extern u64 piThreadStack[0x1000 / sizeof(u64)];

/** @ghidraAddress USA: 0x80381860 */
extern OSMesgQueue piEventQueue;

/** @ghidraAddress USA: 0x80381878 */
extern OSMesg piEventBuf[1];

/** @ghidraAddress USA: 0x803818e0 */
extern OSPiHandle CartRomHandle;

/** @ghidraAddress USA: 0x80381960 */
extern OSPiHandle LeoDiskHandle;

/** @ghidraAddress USA: 0x803819d4 */
extern OSPiHandle *__osDiskHandle;

/** @ghidraAddress USA: 0x8026c5c0 */
void __osPiCreateAccessQueue(void);

/** @ghidraAddress USA: 0x8026c610 */
void __osPiGetAccess(void);

/** @ghidraAddress USA: 0x8026c654 */
void __osPiRelAccess(void);

/** @ghidraAddress USA: 0x8026c990 */
void __osDevMgrMain(void *arg);

/* Wait while the PI is busy with a DMA or an I/O access. The status is read once before the loop
 * tests it, and again on each pass. */
#define WAIT_ON_IOBUSY(stat)                                                                       \
    stat = IO_READ(PI_STATUS_REG);                                                                 \
    while (stat & (PI_STATUS_IO_BUSY | PI_STATUS_DMA_BUSY))                                        \
    stat = IO_READ(PI_STATUS_REG)

/* The registers and bits of the 64DD. */
#define LEO_BASE_REG 0x05000000
#define LEO_C2_BUFF (LEO_BASE_REG + 0x000)
#define LEO_SECTOR_BUFF (LEO_BASE_REG + 0x400)
#define LEO_STATUS (LEO_BASE_REG + 0x508)
#define LEO_BM_CTL (LEO_BASE_REG + 0x510)
#define LEO_BM_STATUS (LEO_BASE_REG + 0x510)

#define LEO_STATUS_DATA_REQUEST 0x40000000
#define LEO_STATUS_C2_TRANSFER 0x10000000
#define LEO_STATUS_BUFFER_MANAGER_ERROR 0x08000000
#define LEO_STATUS_MECHANIC_INTERRUPT 0x02000000

#define LEO_BM_CTL_START 0x80000000
#define LEO_BM_CTL_RESET 0x10000000
#define LEO_BM_CTL_CLR_MECHANIC_INTR 0x01000000

#define LEO_BM_STATUS_MICRO 0x02000000
#define LEO_BM_STATUS_C1DOUBLE 0x00400000
#define LEO_BM_STATUS_C1SINGLE 0x00200000

/* Transfer commands and modes. */
#define LEO_CMD_TYPE_0 0
#define LEO_CMD_TYPE_1 1
#define LEO_CMD_TYPE_2 2
#define LEO_TRACK_MODE 2
#define LEO_SECTOR_MODE 3

/* Results of a block transfer. */
#define LEO_ERROR_GOOD 0
#define LEO_ERROR_4 4
#define LEO_ERROR_22 22
#define LEO_ERROR_23 23
#define LEO_ERROR_24 24
#define LEO_ERROR_29 29

/** @ghidraAddress USA: 0x8026dbe0 */
s32 __osLeoInterrupt(void);

#endif
