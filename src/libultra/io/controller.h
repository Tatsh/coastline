#ifndef LIBULTRA_IO_CONTROLLER_H
#define LIBULTRA_IO_CONTROLLER_H

/**
 * @file
 * The PIF RAM formats of the controller commands, the buffers the commands go through, and the
 * internal routines of the controllers and the Controller Pak.
 */

#include <ultra64.h>

/* Commands of the joybus. */
#define CONT_CMD_REQUEST_STATUS 0 /*!< Request the type and status of a device. */
#define CONT_CMD_READ_BUTTON 1    /*!< Read the buttons and the stick. */
#define CONT_CMD_READ_MEMPACK 2   /*!< Read a block of the Pak. */
#define CONT_CMD_WRITE_MEMPACK 3  /*!< Write a block of the Pak. */
#define CONT_CMD_RESET 0xFF       /*!< Reset the device. */

/* The bytes each command sends and receives. */
#define CONT_CMD_REQUEST_STATUS_TX 1 /*!< The bytes a status request sends. */
#define CONT_CMD_REQUEST_STATUS_RX 3 /*!< The bytes a status request receives. */
#define CONT_CMD_READ_BUTTON_TX 1    /*!< The bytes a button read sends. */
#define CONT_CMD_READ_BUTTON_RX 4    /*!< The bytes a button read receives. */
#define CONT_CMD_READ_MEMPACK_TX 3   /*!< The bytes a Pak read sends. */
#define CONT_CMD_READ_MEMPACK_RX 33  /*!< The bytes a Pak read receives: a block and its CRC. */
#define CONT_CMD_WRITE_MEMPACK_TX 35 /*!< The bytes a Pak write sends: an address and a block. */
#define CONT_CMD_WRITE_MEMPACK_RX 1  /*!< The bytes a Pak write receives: the CRC of the block. */

/* Bytes of the PIF RAM that are not commands. */
#define CONT_CMD_EXE 1    /*!< The flag that starts the commands of the PIF RAM. */
#define CONT_CMD_NOP 0xFF /*!< A byte that skips to the next. */
#define CONT_CMD_END 0xFE /*!< The end of the commands. */

/** The error bits of a channel, the top two bits of its receive size, as 0x0 to 0xC. */
#define CHNL_ERR(format) (((format).rxsize & 0xC0) >> 4)

/* The layout of the Controller Pak, read and written in blocks of 32 bytes. */
#define BLOCKSIZE 32                /*!< The bytes of a block. */
#define PFS_ONE_PAGE 8              /*!< The blocks of a page. */
#define PFS_ID_0AREA 1              /*!< The block of the first copy of the identity. */
#define PFS_ID_1AREA 3              /*!< The block of the second copy of the identity. */
#define PFS_ID_2AREA 4              /*!< The block of the third copy of the identity. */
#define PFS_ID_3AREA 6              /*!< The block of the fourth copy of the identity. */
#define PFS_LABEL_AREA 7            /*!< The block of the label. */
#define PFS_BANK_LAPPED_BY 0x400    /*!< The blocks after which a bank wraps around. */
#define PFS_MAX_BANKS 62            /*!< The most banks a Pak may have. */
#define PFS_INODE_SIZE_PER_PAGE 128 /*!< The entries of a page of the page table. */
#define DEF_DIR_PAGES 2             /*!< The pages of the directory. */

/* Page table entries with special meanings. */
#define PFS_EOF 1           /*!< The last page of a file. */
#define PFS_PAGE_NOT_USED 3 /*!< A free page. */

/** The bit of a directory entry that marks a file as written. */
#define DIR_STATUS_OCCUPIED 0x02

/** The address the motor of a Rumble Pak listens at. */
#define MOTOR_ADDRESS 0x600

/** Return the result of a call unless it is 0. */
#define ERRCK(fn)                                                                                  \
    ret = (fn);                                                                                    \
    if (ret != 0) {                                                                                \
        return ret;                                                                                \
    }

/** Select bank 0 unless it is already active, returning any error. */
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

/**
 * @ghidraAddress USA: 0x8037f3f0
 * @ghidraAddress EUR: 0x80381330
 * @ghidraAddress JPN: 0x8037ffa0
 */
extern OSPifRam __osContPifRam;

/**
 * @ghidraAddress USA: 0x8037f0b0
 * @ghidraAddress EUR: 0x80380ff0
 * @ghidraAddress JPN: 0x8037fc60
 */
extern OSPifRam __osPfsPifRam;

/**
 * @ghidraAddress USA: 0x8037f430
 * @ghidraAddress EUR: 0x80381370
 * @ghidraAddress JPN: 0x8037ffe0
 */
extern u8 __osContLastCmd;

/**
 * @ghidraAddress USA: 0x8037f431
 * @ghidraAddress EUR: 0x80381371
 * @ghidraAddress JPN: 0x8037ffe1
 */
extern u8 __osMaxControllers;

/**
 * @ghidraAddress USA: 0x802a6130
 * @ghidraAddress EUR: 0x802a7c40
 * @ghidraAddress JPN: 0x802a7780
 */
extern u32 __osContinitialized;

/**
 * @ghidraAddress USA: 0x8037f458
 * @ghidraAddress EUR: 0x80381398
 * @ghidraAddress JPN: 0x80380008
 */
extern OSMesgQueue __osEepromTimerQ;

/**
 * @ghidraAddress USA: 0x8037f470
 * @ghidraAddress EUR: 0x803813b0
 * @ghidraAddress JPN: 0x80380020
 */
extern OSMesg __osEepromTimerMsg;

/**
 * @ghidraAddress USA: 0x8037f0f0
 * @ghidraAddress EUR: 0x80381030
 * @ghidraAddress JPN: 0x8037fca0
 */
extern OSPifRam _MotorStopData[MAXCONTROLLERS];

/**
 * @ghidraAddress USA: 0x8037f1f0
 * @ghidraAddress EUR: 0x80381130
 * @ghidraAddress JPN: 0x8037fda0
 */
extern OSPifRam _MotorStartData[MAXCONTROLLERS];

/**
 * @ghidraAddress USA: 0x8037f2f0
 * @ghidraAddress EUR: 0x80381230
 * @ghidraAddress JPN: 0x8037fea0
 */
extern u8 _motorstopbuf[BLOCKSIZE];

/**
 * @ghidraAddress USA: 0x8037f310
 * @ghidraAddress EUR: 0x80381250
 * @ghidraAddress JPN: 0x8037fec0
 */
extern u8 _motorstartbuf[BLOCKSIZE];

/**
 * @ghidraAddress USA: 0x80265b08
 * @ghidraAddress EUR: 0x80267298
 * @ghidraAddress JPN: 0x80266d98
 */
void __osContGetInitData(u8 *pattern, OSContStatus *data);

/**
 * @ghidraAddress USA: 0x80265bd8
 * @ghidraAddress EUR: 0x80267368
 * @ghidraAddress JPN: 0x80266e68
 */
void __osPackRequestData(u8 cmd);

/**
 * @ghidraAddress USA: 0x802648cc
 * @ghidraAddress EUR: 0x8026612c
 * @ghidraAddress JPN: 0x80265b5c
 */
void __osPackReadData(void);

/**
 * @ghidraAddress USA: 0x8026a780
 * @ghidraAddress EUR: 0x8026c5d0
 * @ghidraAddress JPN: 0x8026c080
 */
u8 __osContAddressCrc(u16 addr);

/**
 * @ghidraAddress USA: 0x8026a830
 * @ghidraAddress EUR: 0x8026c680
 * @ghidraAddress JPN: 0x8026c130
 */
u8 __osContDataCrc(u8 *data);

/**
 * @ghidraAddress USA: 0x8026a3f0
 * @ghidraAddress EUR: 0x8026c240
 * @ghidraAddress JPN: 0x8026bcf0
 */
s32 __osContRamRead(OSMesgQueue *mq, int channel, u16 address, u8 *buffer);

/**
 * @ghidraAddress USA: 0x8026a60c
 * @ghidraAddress EUR: 0x8026c45c
 * @ghidraAddress JPN: 0x8026bf0c
 */
void __osPackRamReadData(int channel, u16 address);

/**
 * @ghidraAddress USA: 0x8026a900
 * @ghidraAddress EUR: 0x8026c750
 * @ghidraAddress JPN: 0x8026c200
 */
s32 __osContRamWrite(OSMesgQueue *mq, int channel, u16 address, u8 *buffer, int force);

/**
 * @ghidraAddress USA: 0x8026ab04
 * @ghidraAddress EUR: 0x8026c954
 * @ghidraAddress JPN: 0x8026c404
 */
void __osPackRamWriteData(int channel, u16 address, u8 *buffer);

/**
 * @ghidraAddress USA: 0x80269580
 * @ghidraAddress EUR: 0x8026b3d0
 * @ghidraAddress JPN: 0x8026ae80
 */
s32 __osPfsGetStatus(OSMesgQueue *queue, int channel);

/**
 * @ghidraAddress USA: 0x80261ae0
 * @ghidraAddress EUR: 0x80263260
 * @ghidraAddress JPN: 0x80262d00
 */
void __osPfsRequestData(u8 cmd);

/**
 * @ghidraAddress USA: 0x80261bdc
 * @ghidraAddress EUR: 0x8026335c
 * @ghidraAddress JPN: 0x80262dfc
 */
void __osPfsGetInitData(u8 *pattern, OSContStatus *data);

/**
 * @ghidraAddress USA: 0x80269690
 * @ghidraAddress EUR: 0x8026b4e0
 * @ghidraAddress JPN: 0x8026af90
 */
u16 __osSumcalc(u8 *ptr, int length);

/**
 * @ghidraAddress USA: 0x802696ec
 * @ghidraAddress EUR: 0x8026b53c
 * @ghidraAddress JPN: 0x8026afec
 */
s32 __osIdCheckSum(u16 *ptr, u16 *csum, u16 *icsum);

/**
 * @ghidraAddress USA: 0x80269754
 * @ghidraAddress EUR: 0x8026b5a4
 * @ghidraAddress JPN: 0x8026b054
 */
s32 __osRepairPackId(OSPfs *pfs, __OSPackId *badid, __OSPackId *newid);

/**
 * @ghidraAddress USA: 0x80269b6c
 * @ghidraAddress EUR: 0x8026b9bc
 * @ghidraAddress JPN: 0x8026b46c
 */
s32 __osCheckPackId(OSPfs *pfs, __OSPackId *temp);

/**
 * @ghidraAddress USA: 0x80269d04
 * @ghidraAddress EUR: 0x8026bb54
 * @ghidraAddress JPN: 0x8026b604
 */
s32 __osGetId(OSPfs *pfs);

/**
 * @ghidraAddress USA: 0x80269f60
 * @ghidraAddress EUR: 0x8026bdb0
 * @ghidraAddress JPN: 0x8026b860
 */
s32 __osCheckId(OSPfs *pfs);

/**
 * @ghidraAddress USA: 0x8026a05c
 * @ghidraAddress EUR: 0x8026beac
 * @ghidraAddress JPN: 0x8026b95c
 */
s32 __osPfsRWInode(OSPfs *pfs, __OSInode *inode, u8 flag, u8 bank);

/**
 * @ghidraAddress USA: 0x8026a37c
 * @ghidraAddress EUR: 0x8026c1cc
 * @ghidraAddress JPN: 0x8026bc7c
 */
s32 __osPfsSelectBank(OSPfs *pfs);

#if TGR_REGION == TGR_REGION_JP || TGR_REGION == TGR_REGION_EU
/* The joybus commands of the EEPROM. */
#define CONT_CMD_READ_EEPROM 4  /*!< Read a block of the EEPROM. */
#define CONT_CMD_WRITE_EEPROM 5 /*!< Write a block of the EEPROM. */

/* The bytes each EEPROM command sends and receives. */
#define CONT_CMD_READ_EEPROM_TX 2 /*!< The bytes a read sends: the command and the address. */
#define CONT_CMD_READ_EEPROM_RX 8 /*!< The bytes a read receives: a block. */
#define CONT_CMD_WRITE_EEPROM_TX                                                                   \
    10                             /*!< The bytes a write sends: the command, address, and         \
                                        block. */
#define CONT_CMD_WRITE_EEPROM_RX 1 /*!< The bytes a write receives: the status. */

typedef struct {
    u8 txsize;                  /*!< The bytes sent. */
    u8 rxsize;                  /*!< The bytes received, with the error bits of the channel in
                                     the top two bits. */
    u8 cmd;                     /*!< The command, CONT_CMD_READ_EEPROM or CONT_CMD_WRITE_EEPROM. */
    u8 address;                 /*!< The block. */
    u8 data[EEPROM_BLOCK_SIZE]; /*!< The block read or written. */
} __OSContEepromFormat;

/**
 * @ghidraAddress EUR: 0x803837e0
 * @ghidraAddress JPN: 0x80382450
 */
extern OSPifRam __osEepPifRam;

/**
 * @ghidraAddress EUR: 0x80381378
 * @ghidraAddress JPN: 0x8037ffe8
 */
extern OSTimer __osEepromTimer;

/**
 * @ghidraAddress EUR: 0x8026acf0
 * @ghidraAddress JPN: 0x8026a7a0
 */
void __osPackEepWriteData(u8 address, u8 *buffer);

/**
 * @ghidraAddress EUR: 0x8026adfc
 * @ghidraAddress JPN: 0x8026a8ac
 */
s32 __osEepStatus(OSMesgQueue *mq, OSContStatus *data);

/**
 * @ghidraAddress EUR: 0x8026b210
 * @ghidraAddress JPN: 0x8026acc0
 */
void __osPackEepReadData(u8 address);
#endif /* TGR_REGION == TGR_REGION_JP || TGR_REGION == TGR_REGION_EU */

#endif /* LIBULTRA_IO_CONTROLLER_H */
