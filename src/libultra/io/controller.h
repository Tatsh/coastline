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
    u32 ramarray[15]; /*!< The commands and their responses, one channel after another. */
    u32 pifstatus;    /*!< The control word, set to CONT_CMD_EXE to run the commands. */
} OSPifRam;

typedef struct {
    u8 dummy;  /*!< A CONT_CMD_NOP byte of padding before the command. */
    u8 txsize; /*!< The bytes sent, CONT_CMD_REQUEST_STATUS_TX. */
    u8 rxsize; /*!< The bytes received, with the error bits of the channel in the top two bits. */
    u8 cmd;    /*!< The command, CONT_CMD_REQUEST_STATUS or CONT_CMD_RESET. */
    u8 typeh;  /*!< The received byte that becomes the low byte of the device type. */
    u8 typel;  /*!< The received byte that becomes the high byte of the device type. */
    u8 status; /*!< The received status of the device, such as CONT_CARD_ON. */
    u8 dummy1; /*!< A CONT_CMD_NOP byte of padding after the response. */
} __OSContRequesFormat;

typedef struct {
    u8 dummy;   /*!< A CONT_CMD_NOP byte of padding before the command. */
    u8 txsize;  /*!< The bytes sent, CONT_CMD_READ_BUTTON_TX. */
    u8 rxsize;  /*!< The bytes received, with the error bits of the channel in the top two bits. */
    u8 cmd;     /*!< The command, CONT_CMD_READ_BUTTON. */
    u16 button; /*!< The received buttons that are pressed, a bit for each. */
    s8 stick_x; /*!< The received horizontal position of the control stick. */
    s8 stick_y; /*!< The received vertical position of the control stick. */
} __OSContReadFormat;

typedef struct {
    u8 dummy;    /*!< A CONT_CMD_NOP byte of padding before the command. */
    u8 txsize;   /*!< The bytes sent. */
    u8 rxsize;   /*!< The bytes received, with the error bits of the channel in the top two bits. */
    u8 cmd;      /*!< The command, CONT_CMD_READ_MEMPACK or CONT_CMD_WRITE_MEMPACK. */
    u16 address; /*!< The block address shifted left by 5, with its 5-bit CRC in the low bits. */
    u8 data[BLOCKSIZE]; /*!< The block read or written. */
    u8 datacrc;         /*!< The CRC of the data, received from the Pak. */
} __OSContRamReadFormat;

typedef struct {
    u32 repaired;          /*!< Set to 0xFFFFFFFF when a repair rewrites the block. */
    u32 random;            /*!< The count register at the time of a repair. */
    u64 serial_mid;        /*!< The middle part of the serial number. */
    u64 serial_low;        /*!< The low part of the serial number. */
    u16 deviceid;          /*!< The device flags, with bit 0 set on a Pak with at least one bank. */
    u8 banks;              /*!< The number of banks of 32 KB. */
    u8 version;            /*!< The version of the file system. */
    u16 checksum;          /*!< The 16-bit sum of the halfwords before the checksums. */
    u16 inverted_checksum; /*!< The 16-bit sum of the inverses of the halfwords before the
                                checksums. */
} __OSPackId;

typedef union {
    struct {
        u8 bank; /*!< The bank of the page. */
        u8 page; /*!< The page within the bank. */
    } inode_t;   /*!< The entry as a bank and a page. */
    u16 ipage;   /*!< The entry as one number, or PFS_EOF or PFS_PAGE_NOT_USED. */
} __OSInodeUnit;

typedef struct {
    __OSInodeUnit inode_page[128]; /*!< The page that follows each page in its file, by page. */
} __OSInode;

typedef struct {
    u32 game_code;            /*!< The game code of the file, or 0 for a free entry. */
    u16 company_code;         /*!< The company code of the file, or 0 for a free entry. */
    __OSInodeUnit start_page; /*!< The first page of the file. */
    u8 status;                /*!< The status bits, such as DIR_STATUS_OCCUPIED. */
    s8 reserved;              /*!< Reserved. */
    u16 data_sum;             /*!< Reserved for a sum of the data, written as 0. */
    u8 ext_name[4];   /*!< The extension of the file name, in the character set of the Pak. */
    u8 game_name[16]; /*!< The file name, in the character set of the Pak. */
} __OSDir;

typedef struct {
    __OSInode inode; /*!< The page table of the bank last read. */
    u8 bank;         /*!< The bank whose page table is in inode, or 0xFF for none. */
    u8 map[256];     /*!< For each four pages, a bit for each bank modulo 8 pointing into them. */
} __OSInodeCache;

/** @ghidraAddress USA: 0x8037f3f0 */
extern OSPifRam __osContPifRam;

/** @ghidraAddress USA: 0x8037f0b0 */
extern OSPifRam __osPfsPifRam;

/** @ghidraAddress USA: 0x8037f430 */
extern u8 __osContLastCmd;

/** @ghidraAddress USA: 0x8037f431 */
extern u8 __osMaxControllers;

/** @ghidraAddress USA: 0x802a6130 */
extern u32 __osContinitialized;

/** @ghidraAddress USA: 0x8037f458 */
extern OSMesgQueue __osEepromTimerQ;

/** @ghidraAddress USA: 0x8037f470 */
extern OSMesg __osEepromTimerMsg;

/** @ghidraAddress USA: 0x8037f0f0 */
extern OSPifRam _MotorStopData[MAXCONTROLLERS];

/** @ghidraAddress USA: 0x8037f1f0 */
extern OSPifRam _MotorStartData[MAXCONTROLLERS];

/** @ghidraAddress USA: 0x8037f2f0 */
extern u8 _motorstopbuf[BLOCKSIZE];

/** @ghidraAddress USA: 0x8037f310 */
extern u8 _motorstartbuf[BLOCKSIZE];

/** @ghidraAddress USA: 0x80265b08 */
void __osContGetInitData(u8 *pattern, OSContStatus *data);

/** @ghidraAddress USA: 0x80265bd8 */
void __osPackRequestData(u8 cmd);

/** @ghidraAddress USA: 0x802648cc */
void __osPackReadData(void);

/** @ghidraAddress USA: 0x8026a780 */
u8 __osContAddressCrc(u16 addr);

/** @ghidraAddress USA: 0x8026a830 */
u8 __osContDataCrc(u8 *data);

/** @ghidraAddress USA: 0x8026a3f0 */
s32 __osContRamRead(OSMesgQueue *mq, int channel, u16 address, u8 *buffer);

/** @ghidraAddress USA: 0x8026a60c */
void __osPackRamReadData(int channel, u16 address);

/** @ghidraAddress USA: 0x8026a900 */
s32 __osContRamWrite(OSMesgQueue *mq, int channel, u16 address, u8 *buffer, int force);

/** @ghidraAddress USA: 0x8026ab04 */
void __osPackRamWriteData(int channel, u16 address, u8 *buffer);

/** @ghidraAddress USA: 0x80269580 */
s32 __osPfsGetStatus(OSMesgQueue *queue, int channel);

/** @ghidraAddress USA: 0x80261ae0 */
void __osPfsRequestData(u8 cmd);

/** @ghidraAddress USA: 0x80261bdc */
void __osPfsGetInitData(u8 *pattern, OSContStatus *data);

/** @ghidraAddress USA: 0x80269690 */
u16 __osSumcalc(u8 *ptr, int length);

/** @ghidraAddress USA: 0x802696ec */
s32 __osIdCheckSum(u16 *ptr, u16 *csum, u16 *icsum);

/** @ghidraAddress USA: 0x80269754 */
s32 __osRepairPackId(OSPfs *pfs, __OSPackId *badid, __OSPackId *newid);

/** @ghidraAddress USA: 0x80269b6c */
s32 __osCheckPackId(OSPfs *pfs, __OSPackId *temp);

/** @ghidraAddress USA: 0x80269d04 */
s32 __osGetId(OSPfs *pfs);

/** @ghidraAddress USA: 0x80269f60 */
s32 __osCheckId(OSPfs *pfs);

/** @ghidraAddress USA: 0x8026a05c */
s32 __osPfsRWInode(OSPfs *pfs, __OSInode *inode, u8 flag, u8 bank);

/** @ghidraAddress USA: 0x8026a37c */
s32 __osPfsSelectBank(OSPfs *pfs);

#endif
