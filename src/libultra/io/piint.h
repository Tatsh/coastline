#ifndef LIBULTRA_IO_PIINT_H
#define LIBULTRA_IO_PIINT_H

/**
 * @file
 * The PI manager, the PI access queue, and the PI handles of the cartridge and the 64DD.
 */

#include <ultra64.h>

/**
 * @ghidraAddress USA: 0x802a62e0
 * @ghidraAddress EUR: 0x802a7e40
 * @ghidraAddress JPN: 0x802a7930
 */
extern u32 __osPiAccessQueueEnabled;

/**
 * @ghidraAddress USA: 0x803819e8
 * @ghidraAddress EUR: 0x80383968
 * @ghidraAddress JPN: 0x803825d8
 */
extern OSMesgQueue __osPiAccessQueue;

/**
 * @ghidraAddress USA: 0x803819e0
 * @ghidraAddress EUR: 0x80383960
 * @ghidraAddress JPN: 0x803825d0
 */
extern OSMesg piAccessBuf[1];

/**
 * @ghidraAddress USA: 0x802a61a0
 * @ghidraAddress EUR: 0x802a7d00
 * @ghidraAddress JPN: 0x802a77f0
 */
extern OSPiHandle *__osCurrentHandle[2];

/**
 * @ghidraAddress USA: 0x802a6180
 * @ghidraAddress EUR: 0x802a7ce0
 * @ghidraAddress JPN: 0x802a77d0
 */
extern OSDevMgr __osPiDevMgr;

/**
 * @ghidraAddress USA: 0x802a619c
 * @ghidraAddress EUR: 0x802a7cfc
 * @ghidraAddress JPN: 0x802a77ec
 */
extern OSPiHandle *__osPiTable;

/**
 * @ghidraAddress USA: 0x803806b0
 * @ghidraAddress EUR: 0x803825f0
 * @ghidraAddress JPN: 0x80381260
 */
extern OSThread piThread;

/**
 * @ghidraAddress USA: 0x80380860
 * @ghidraAddress EUR: 0x803827a0
 * @ghidraAddress JPN: 0x80381410
 */
extern u64 piThreadStack[0x1000 / sizeof(u64)];

/**
 * @ghidraAddress USA: 0x80381860
 * @ghidraAddress EUR: 0x803837a0
 * @ghidraAddress JPN: 0x80382410
 */
extern OSMesgQueue piEventQueue;

/**
 * @ghidraAddress USA: 0x80381878
 * @ghidraAddress EUR: 0x803837b8
 * @ghidraAddress JPN: 0x80382428
 */
extern OSMesg piEventBuf[1];

/**
 * @ghidraAddress USA: 0x803818e0
 * @ghidraAddress EUR: 0x80383860
 * @ghidraAddress JPN: 0x803824d0
 */
extern OSPiHandle CartRomHandle;

/**
 * @ghidraAddress USA: 0x80381960
 * @ghidraAddress EUR: 0x803838e0
 * @ghidraAddress JPN: 0x80382550
 */
extern OSPiHandle LeoDiskHandle;

/**
 * @ghidraAddress USA: 0x803819d4
 * @ghidraAddress EUR: 0x80383954
 * @ghidraAddress JPN: 0x803825c4
 */
extern OSPiHandle *__osDiskHandle;

/**
 * @ghidraAddress USA: 0x8026c5c0
 * @ghidraAddress EUR: 0x8026e590
 * @ghidraAddress JPN: 0x8026e040
 */
void __osPiCreateAccessQueue(void);

/**
 * @ghidraAddress USA: 0x8026c610
 * @ghidraAddress EUR: 0x8026e5e0
 * @ghidraAddress JPN: 0x8026e090
 */
void __osPiGetAccess(void);

/**
 * @ghidraAddress USA: 0x8026c654
 * @ghidraAddress EUR: 0x8026e624
 * @ghidraAddress JPN: 0x8026e0d4
 */
void __osPiRelAccess(void);

/**
 * @ghidraAddress USA: 0x8026c990
 * @ghidraAddress EUR: 0x8026e960
 * @ghidraAddress JPN: 0x8026e410
 */
void __osDevMgrMain(void *arg);

/**
 * Wait while the PI is busy with a DMA or an I/O access. The status is read once before the loop
 * tests it, and again on each pass.
 */
#define WAIT_ON_IOBUSY(stat)                                                                       \
    stat = IO_READ(PI_STATUS_REG);                                                                 \
    while (stat & (PI_STATUS_IO_BUSY | PI_STATUS_DMA_BUSY))                                        \
    stat = IO_READ(PI_STATUS_REG)

/* The registers of the 64DD. */
#define LEO_BASE_REG 0x05000000                /*!< The base address of the 64DD registers. */
#define LEO_C2_BUFF (LEO_BASE_REG + 0x000)     /*!< The buffer of the C2 error correction. */
#define LEO_SECTOR_BUFF (LEO_BASE_REG + 0x400) /*!< The buffer of a sector. */
#define LEO_STATUS (LEO_BASE_REG + 0x508)      /*!< The status register. */
#define LEO_BM_CTL (LEO_BASE_REG + 0x510)      /*!< The buffer manager control, on write. */
#define LEO_BM_STATUS (LEO_BASE_REG + 0x510)   /*!< The buffer manager status, on read. */

/* Bits of the status register. */
#define LEO_STATUS_DATA_REQUEST 0x40000000         /*!< The drive requests data. */
#define LEO_STATUS_C2_TRANSFER 0x10000000          /*!< A C2 transfer is ready. */
#define LEO_STATUS_BUFFER_MANAGER_ERROR 0x08000000 /*!< The buffer manager reported an error. */
#define LEO_STATUS_MECHANIC_INTERRUPT 0x02000000   /*!< The mechanism raised an interrupt. */

/* Bits of the buffer manager control. */
#define LEO_BM_CTL_START 0x80000000             /*!< Start the buffer manager. */
#define LEO_BM_CTL_RESET 0x10000000             /*!< Reset the buffer manager. */
#define LEO_BM_CTL_CLR_MECHANIC_INTR 0x01000000 /*!< Clear the interrupt of the mechanism. */

/* Bits of the buffer manager status. */
#define LEO_BM_STATUS_MICRO 0x02000000    /*!< The micro error. */
#define LEO_BM_STATUS_C1DOUBLE 0x00400000 /*!< A C1 double error. */
#define LEO_BM_STATUS_C1SINGLE 0x00200000 /*!< A C1 single error. */

/* Transfer commands and modes. */
#define LEO_CMD_TYPE_0 0  /*!< A transfer command of type 0. */
#define LEO_CMD_TYPE_1 1  /*!< A transfer command of type 1. */
#define LEO_CMD_TYPE_2 2  /*!< A transfer command of type 2. */
#define LEO_TRACK_MODE 2  /*!< Transfer a whole track. */
#define LEO_SECTOR_MODE 3 /*!< Transfer one sector. */

/* Results of a block transfer. */
#define LEO_ERROR_GOOD 0 /*!< The transfer succeeded. */
#define LEO_ERROR_4 4    /*!< Error 4 of the 64DD. */
#define LEO_ERROR_22 22  /*!< Error 22 of the 64DD. */
#define LEO_ERROR_23 23  /*!< Error 23 of the 64DD. */
#define LEO_ERROR_24 24  /*!< Error 24 of the 64DD. */
#define LEO_ERROR_29 29  /*!< Error 29 of the 64DD. */

/**
 * @ghidraAddress USA: 0x8026dbe0
 * @ghidraAddress EUR: 0x8026fbb0
 * @ghidraAddress JPN: 0x8026f660
 */
s32 __osLeoInterrupt(void);

#endif /* LIBULTRA_IO_PIINT_H */
