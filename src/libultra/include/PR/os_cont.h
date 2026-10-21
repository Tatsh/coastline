#ifndef PR_OS_CONT_H
#define PR_OS_CONT_H

/**
 * @file
 * The controllers and the Controller Pak file system.
 */

#include <PR/os.h>
#include <PR/ultratypes.h>

#define MAXCONTROLLERS 4

/* Bits of OSContStatus.type. */
#define CONT_ABSOLUTE 0x0001
#define CONT_RELATIVE 0x0002
#define CONT_JOYPORT 0x0004

/* Bits of OSContStatus.status. */
#define CONT_CARD_ON 0x01
#define CONT_CARD_PULL 0x02
#define CONT_ADDR_CRC_ER 0x04

/* Results of the Controller Pak routines. */
#define PFS_ERR_NOPACK 1
#define PFS_ERR_NEW_PACK 2
#define PFS_ERR_INCONSISTENT 3
#define PFS_ERR_CONTRFAIL 4
#define PFS_ERR_INVALID 5
#define PFS_ERR_BAD_DATA 6
#define PFS_DATA_FULL 7
#define PFS_DIR_FULL 8
#define PFS_ERR_EXIST 9
#define PFS_ERR_ID_FATAL 10
#define PFS_ERR_DEVICE 11

/* Directions of osPfsReadWriteFile(). */
#define PFS_READ 0
#define PFS_WRITE 1

/* Bits of OSContPad.button. */
#define CONT_A 0x8000
#define CONT_B 0x4000
#define CONT_G 0x2000
#define CONT_START 0x1000
#define CONT_UP 0x0800
#define CONT_DOWN 0x0400
#define CONT_LEFT 0x0200
#define CONT_RIGHT 0x0100
#define CONT_L 0x0020
#define CONT_R 0x0010
#define CONT_E 0x0008
#define CONT_D 0x0004
#define CONT_C 0x0002
#define CONT_F 0x0001

#define A_BUTTON CONT_A
#define B_BUTTON CONT_B
#define L_TRIG CONT_L
#define R_TRIG CONT_R
#define Z_TRIG CONT_G
#define START_BUTTON CONT_START
#define U_JPAD CONT_UP
#define L_JPAD CONT_LEFT
#define R_JPAD CONT_RIGHT
#define D_JPAD CONT_DOWN
#define U_CBUTTONS CONT_E
#define L_CBUTTONS CONT_C
#define R_CBUTTONS CONT_F
#define D_CBUTTONS CONT_D

/* OSContPad.errno of a controller that does not answer. */
#define CONT_NO_RESPONSE_ERROR 8

/* Bits of OSPfs.status. */
#define PFS_INITIALIZED 0x01
#define PFS_CORRUPTED 0x02

/** What osContInit() learns of a controller. */
typedef struct {
    u16 type;
    u8 status;
    u8 errno;
} OSContStatus;

/** The buttons and stick of a controller. */
typedef struct {
    u16 button;
    s8 stick_x;
    s8 stick_y;
    u8 errno;
} OSContPad;

/** An open Controller Pak. */
typedef struct {
    int status;
    OSMesgQueue *queue;
    int channel;
    u8 id[32];
    u8 label[32];
    int version;
    int dir_size;
    int inode_table;
    int minode_table;
    int dir_table;
    int inode_start_page;
    u8 banks;
    u8 activebank;
} OSPfs;

/** What osPfsFileState() reports of a file. */
typedef struct {
    u32 file_size;
    u32 game_code;
    u16 company_code;
    char ext_name[4];
    char game_name[16];
} OSPfsState;

/**
 * Wait until the controllers have powered up, then find which are present.
 *
 * @param bitpattern Receives a bit for each controller that answered.
 * @ghidraAddress USA: 0x80265910
 */
s32 osContInit(OSMesgQueue *mq, u8 *bitpattern, OSContStatus *data);

/**
 * Start reading the buttons and sticks of every controller.
 *
 * @ghidraAddress USA: 0x80264760
 */
s32 osContStartReadData(OSMesgQueue *mq);

/**
 * Copy the buttons and sticks of the read osContStartReadData() started.
 *
 * @ghidraAddress USA: 0x80264824
 */
void osContGetReadData(OSContPad *data);

/**
 * Find which controllers hold a Controller Pak.
 *
 * @param pattern Receives a bit for each controller with a Pak.
 * @ghidraAddress USA: 0x80261940
 */
s32 osPfsIsPlug(OSMesgQueue *queue, u8 *pattern);

/**
 * Open the Controller Pak of a channel, repairing its ID if needed, and check its file system.
 *
 * @ghidraAddress USA: 0x80265cd0
 */
s32 osPfsInit(OSMesgQueue *queue, OSPfs *pfs, int channel);

/**
 * Open the Controller Pak of a channel like osPfsInit(), starting from the ID checksum of the
 * caller's stack.
 *
 * @ghidraAddress USA: 0x80261cb0
 */
s32 osPfsInitPak(OSMesgQueue *queue, OSPfs *pfs, int channel);

/**
 * Read the ID block again, repairing it if needed.
 *
 * @ghidraAddress USA: 0x80262660
 */
s32 osPfsRepairId(OSPfs *pfs);

/**
 * Remove directory entries whose page chains are broken, and rebuild the page tables from the
 * chains that remain.
 *
 * @ghidraAddress USA: 0x80262a80
 */
s32 osPfsChecker(OSPfs *pfs);

/**
 * Find a file by its codes, and by its names unless they are NULL.
 *
 * @param file_no Receives the directory index, or -1.
 * @ghidraAddress USA: 0x802628c0
 */
s32 osPfsFindFile(
    OSPfs *pfs, u16 company_code, u32 game_code, u8 *game_name, u8 *ext_name, s32 *file_no);

/**
 * Read or write whole blocks of a file.
 *
 * @param flag PFS_READ or PFS_WRITE.
 * @ghidraAddress USA: 0x802635dc
 */
s32 osPfsReadWriteFile(
    OSPfs *pfs, s32 file_no, u8 flag, int offset, int size_in_bytes, u8 *data_buffer);

/**
 * Return the bytes of the pages no file uses.
 *
 * @ghidraAddress USA: 0x802639e0
 */
s32 osPfsFreeBlocks(OSPfs *pfs, s32 *bytes_not_used);

/**
 * Create a file of at least the given size.
 *
 * @param file_no Receives the directory index of the new file.
 * @ghidraAddress USA: 0x80263b30
 */
s32 osPfsAllocateFile(OSPfs *pfs,
                      u16 company_code,
                      u32 game_code,
                      u8 *game_name,
                      u8 *ext_name,
                      int file_size_in_bytes,
                      s32 *file_no);

/**
 * Count the directory entries and the files that use them.
 *
 * @ghidraAddress USA: 0x802677e0
 */
s32 osPfsNumFiles(OSPfs *pfs, s32 *max_files, s32 *files_used);

/**
 * Report the size, codes and names of a file.
 *
 * @ghidraAddress USA: 0x80267930
 */
s32 osPfsFileState(OSPfs *pfs, s32 file_no, OSPfsState *state);

/**
 * Delete a file and free its pages.
 *
 * @ghidraAddress USA: 0x80267c20
 */
s32 osPfsDeleteFile(OSPfs *pfs, u16 company_code, u32 game_code, u8 *game_name, u8 *ext_name);

/**
 * Open the Rumble Pak of a channel and build the commands that start and stop its motor.
 *
 * @ghidraAddress USA: 0x80262370
 */
s32 osMotorInit(OSMesgQueue *mq, OSPfs *pfs, int channel);

/**
 * Start the motor of a Rumble Pak.
 *
 * @ghidraAddress USA: 0x80262088
 */
s32 osMotorStart(OSPfs *pfs);

/**
 * Stop the motor of a Rumble Pak.
 *
 * @ghidraAddress USA: 0x80261f20
 */
s32 osMotorStop(OSPfs *pfs);

#endif
