#ifndef PR_OS_CONT_H
#define PR_OS_CONT_H

/**
 * @file
 * The controllers and the Controller Pak file system.
 */

#include <PR/os.h>
#include <PR/ultratypes.h>

#define MAXCONTROLLERS 4

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

#endif
