#ifndef LIBULTRA_IO_CONTROLLER_H
#define LIBULTRA_IO_CONTROLLER_H

/**
 * @file
 * The PIF RAM formats of the controller commands, the buffers the commands go through, and the
 * internal routines of the controllers and the Controller Pak.
 */

#include <ultra64.h>

/* Commands of the joybus. */
#define CONT_CMD_REQUEST_STATUS 0
#define CONT_CMD_READ_BUTTON 1
#define CONT_CMD_READ_MEMPACK 2
#define CONT_CMD_WRITE_MEMPACK 3
#define CONT_CMD_RESET 0xFF

/* The bytes each command sends and receives. */
#define CONT_CMD_REQUEST_STATUS_TX 1
#define CONT_CMD_REQUEST_STATUS_RX 3
#define CONT_CMD_READ_BUTTON_TX 1
#define CONT_CMD_READ_BUTTON_RX 4
#define CONT_CMD_READ_MEMPACK_TX 3
#define CONT_CMD_READ_MEMPACK_RX 33
#define CONT_CMD_WRITE_MEMPACK_TX 35
#define CONT_CMD_WRITE_MEMPACK_RX 1

/* Bytes of the PIF RAM that are not commands. */
#define CONT_CMD_EXE 1
#define CONT_CMD_NOP 0xFF
#define CONT_CMD_END 0xFE

/* The error bits of a channel are the top two bits of its receive size. */
#define CHNL_ERR(format) (((format).rxsize & 0xC0) >> 4)

/* The Controller Pak is read and written in blocks of 32 bytes. */
#define BLOCKSIZE 32
#define PFS_ONE_PAGE 8
#define PFS_ID_0AREA 1
#define PFS_ID_1AREA 3
#define PFS_ID_2AREA 4
#define PFS_ID_3AREA 6
#define PFS_LABEL_AREA 7
#define PFS_BANK_LAPPED_BY 0x400
#define PFS_MAX_BANKS 62
#define PFS_INODE_SIZE_PER_PAGE 128
#define DEF_DIR_PAGES 2

/* Page table entries with special meanings. */
#define PFS_EOF 1
#define PFS_PAGE_NOT_USED 3

/* The bit of a directory entry that marks a file as written. */
#define DIR_STATUS_OCCUPIED 0x02

/* The address the motor of a Rumble Pak listens at. */
#define MOTOR_ADDRESS 0x600

/* Return the result of a call unless it is 0. */
#define ERRCK(fn)                                                                                  \
    ret = (fn);                                                                                    \
    if (ret != 0) {                                                                                \
        return ret;                                                                                \
    }

/* Select bank 0 unless it is already active. */
#define SET_ACTIVEBANK_TO_ZERO                                                                     \
    if (pfs->activebank != 0) {                                                                    \
        pfs->activebank = 0;                                                                       \
        ERRCK(__osPfsSelectBank(pfs));                                                             \
    }

/** The 64 bytes of the PIF RAM. The last word starts the commands. */
typedef struct {
    u32 ramarray[15];
    u32 pifstatus;
} OSPifRam;

/** The request and answer of a status command. */
typedef struct {
    u8 dummy;
    u8 txsize;
    u8 rxsize;
    u8 cmd;
    u8 typeh;
    u8 typel;
    u8 status;
    u8 dummy1;
} __OSContRequesFormat;

/** The request and answer of a button read. */
typedef struct {
    u8 dummy;
    u8 txsize;
    u8 rxsize;
    u8 cmd;
    u16 button;
    s8 stick_x;
    s8 stick_y;
} __OSContReadFormat;

/** The request and answer of a read or a write of a Controller Pak block. */
typedef struct {
    u8 dummy;
    u8 txsize;
    u8 rxsize;
    u8 cmd;
    u16 address;
    u8 data[BLOCKSIZE];
    u8 datacrc;
} __OSContRamReadFormat;

/** The identification block of a Controller Pak. */
typedef struct {
    u32 repaired;
    u32 random;
    u64 serial_mid;
    u64 serial_low;
    u16 deviceid;
    u8 banks;
    u8 version;
    u16 checksum;
    u16 inverted_checksum;
} __OSPackId;

/** An entry of the page table of a Controller Pak. */
typedef union {
    struct {
        u8 bank;
        u8 page;
    } inode_t;
    u16 ipage;
} __OSInodeUnit;

/** The page table of one bank. */
typedef struct {
    __OSInodeUnit inode_page[128];
} __OSInode;

/** A directory entry of a Controller Pak. */
typedef struct {
    u32 game_code;
    u16 company_code;
    __OSInodeUnit start_page;
    u8 status;
    s8 reserved;
    u16 data_sum;
    u8 ext_name[4];
    u8 game_name[16];
} __OSDir;

/**
 * The page table of the bank osPfsChecker() last read, and for each group of four pages a bit for
 * every other bank whose table points into the group.
 */
typedef struct {
    __OSInode inode;
    u8 bank;
    u8 map[256];
} __OSInodeCache;

/**
 * The PIF RAM image of the controller commands.
 *
 * @ghidraAddress USA: 0x8037f3f0
 */
extern OSPifRam __osContPifRam;

/**
 * The PIF RAM image of the Controller Pak commands.
 *
 * @ghidraAddress USA: 0x8037f0b0
 */
extern OSPifRam __osPfsPifRam;

/**
 * The command the controller PIF RAM image last held, so that a repeated button read skips the
 * write.
 *
 * @ghidraAddress USA: 0x8037f430
 */
extern u8 __osContLastCmd;

/** @ghidraAddress USA: 0x8037f431 */
extern u8 __osMaxControllers;

/** Whether osContInit() has run. @ghidraAddress USA: 0x802a6130 */
extern u32 __osContinitialized;

/** @ghidraAddress USA: 0x8037f458 */
extern OSMesgQueue __osEepromTimerQ;

/** @ghidraAddress USA: 0x8037f470 */
extern OSMesg __osEepromTimerMsg;

/**
 * The PIF RAM image of the command that stops the motor of each channel.
 *
 * @ghidraAddress USA: 0x8037f0f0
 */
extern OSPifRam _MotorStopData[MAXCONTROLLERS];

/**
 * The PIF RAM image of the command that starts the motor of each channel.
 *
 * @ghidraAddress USA: 0x8037f1f0
 */
extern OSPifRam _MotorStartData[MAXCONTROLLERS];

/** The block written to stop a motor. @ghidraAddress USA: 0x8037f2f0 */
extern u8 _motorstopbuf[BLOCKSIZE];

/** The block written to start a motor. @ghidraAddress USA: 0x8037f310 */
extern u8 _motorstartbuf[BLOCKSIZE];

/**
 * Decode the answers of a status command.
 *
 * @ghidraAddress USA: 0x80265b08
 */
void __osContGetInitData(u8 *pattern, OSContStatus *data);

/**
 * Fill the controller PIF RAM image with a status command for every controller.
 *
 * @ghidraAddress USA: 0x80265bd8
 */
void __osPackRequestData(u8 cmd);

/**
 * Fill the controller PIF RAM image with a button read for every controller.
 *
 * @ghidraAddress USA: 0x802648cc
 */
void __osPackReadData(void);

/**
 * Return the 5-bit CRC of a Controller Pak address.
 *
 * @ghidraAddress USA: 0x8026a780
 */
u8 __osContAddressCrc(u16 addr);

/**
 * Return the CRC of a 32-byte block.
 *
 * @ghidraAddress USA: 0x8026a830
 */
u8 __osContDataCrc(u8 *data);

/**
 * Read a 32-byte block of a Controller Pak.
 *
 * @ghidraAddress USA: 0x8026a3f0
 */
s32 __osContRamRead(OSMesgQueue *mq, int channel, u16 address, u8 *buffer);

/**
 * Fill the Pak PIF RAM image with a block read.
 *
 * @ghidraAddress USA: 0x8026a60c
 */
void __osPackRamReadData(int channel, u16 address);

/**
 * Write a 32-byte block of a Controller Pak. The ID and label blocks are written only when force
 * is TRUE.
 *
 * @ghidraAddress USA: 0x8026a900
 */
s32 __osContRamWrite(OSMesgQueue *mq, int channel, u16 address, u8 *buffer, int force);

/**
 * Fill the Pak PIF RAM image with a block write.
 *
 * @ghidraAddress USA: 0x8026ab04
 */
void __osPackRamWriteData(int channel, u16 address, u8 *buffer);

/**
 * Return why a Controller Pak did not answer, or 0.
 *
 * @ghidraAddress USA: 0x80269580
 */
s32 __osPfsGetStatus(OSMesgQueue *queue, int channel);

/**
 * Fill the Pak PIF RAM image with a command for every controller.
 *
 * @ghidraAddress USA: 0x80261ae0
 */
void __osPfsRequestData(u8 cmd);

/**
 * Decode the answers of a status command in the Pak PIF RAM image.
 *
 * @ghidraAddress USA: 0x80261bdc
 */
void __osPfsGetInitData(u8 *pattern, OSContStatus *data);

/**
 * Return the 16-bit sum of the bytes of a buffer.
 *
 * @ghidraAddress USA: 0x80269690
 */
u16 __osSumcalc(u8 *ptr, int length);

/**
 * Compute the checksum and inverted checksum of an ID block.
 *
 * @ghidraAddress USA: 0x802696ec
 */
s32 __osIdCheckSum(u16 *ptr, u16 *csum, u16 *icsum);

/**
 * Write a new ID block after counting the banks of the Pak.
 *
 * @ghidraAddress USA: 0x80269754
 */
s32 __osRepairPackId(OSPfs *pfs, __OSPackId *badid, __OSPackId *newid);

/**
 * Find a valid copy of the ID block and restore the others from it.
 *
 * @ghidraAddress USA: 0x80269b6c
 */
s32 __osCheckPackId(OSPfs *pfs, __OSPackId *temp);

/**
 * Read the ID block, repairing it if needed, and set up the layout of the Pak.
 *
 * @ghidraAddress USA: 0x80269d04
 */
s32 __osGetId(OSPfs *pfs);

/**
 * Return whether the Pak still has the ID osPfsInit() read.
 *
 * @ghidraAddress USA: 0x80269f60
 */
s32 __osCheckId(OSPfs *pfs);

/**
 * Read or write the page table of a bank, keeping the main and mirror copies consistent.
 *
 * @ghidraAddress USA: 0x8026a05c
 */
s32 __osPfsRWInode(OSPfs *pfs, __OSInode *inode, u8 flag, u8 bank);

/**
 * Select the active bank of the Pak.
 *
 * @ghidraAddress USA: 0x8026a37c
 */
s32 __osPfsSelectBank(OSPfs *pfs);

#endif
