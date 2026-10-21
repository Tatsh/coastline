#ifndef PR_OS_CONT_H
#define PR_OS_CONT_H

/**
 * @file
 * The controllers and the Controller Pak file system.
 */

#include <PR/os.h>
#include <PR/ultratypes.h>

/** The ports of the console. */
#define MAXCONTROLLERS 4

/* Bits of OSContStatus.type. */
#define CONT_ABSOLUTE 0x0001 /*!< The device reports absolute positions, as a controller does. */
#define CONT_RELATIVE 0x0002 /*!< The device reports relative movement, as a mouse does. */
#define CONT_JOYPORT 0x0004  /*!< The device has a port for a Pak. */

/* Bits of OSContStatus.status. */
#define CONT_CARD_ON 0x01     /*!< A Pak is plugged in. */
#define CONT_CARD_PULL 0x02   /*!< A Pak was pulled out since the last check. */
#define CONT_ADDR_CRC_ER 0x04 /*!< The CRC of the last address was wrong. */

/* Results of the Controller Pak routines. */
#define PFS_ERR_NOPACK 1       /*!< No Pak is plugged in. */
#define PFS_ERR_NEW_PACK 2     /*!< The Pak was changed. */
#define PFS_ERR_INCONSISTENT 3 /*!< The file system needs repair. */
#define PFS_ERR_CONTRFAIL 4    /*!< The transfer with the controller failed. */
#define PFS_ERR_INVALID 5      /*!< An argument is invalid or the file does not exist. */
#define PFS_ERR_BAD_DATA 6     /*!< The data read is wrong. */
#define PFS_DATA_FULL 7        /*!< No pages are free. */
#define PFS_DIR_FULL 8         /*!< No entries of the directory are free. */
#define PFS_ERR_EXIST 9        /*!< The file already exists. */
#define PFS_ERR_ID_FATAL 10    /*!< The identity area cannot be repaired. */
#define PFS_ERR_DEVICE 11      /*!< The Pak is not a Controller Pak. */

/* Directions of osPfsReadWriteFile(). */
#define PFS_READ 0  /*!< Read from the file. */
#define PFS_WRITE 1 /*!< Write to the file. */

/* Bits of OSContPad.button. */
#define CONT_A 0x8000     /*!< The A button. */
#define CONT_B 0x4000     /*!< The B button. */
#define CONT_G 0x2000     /*!< The Z trigger. */
#define CONT_START 0x1000 /*!< The Start button. */
#define CONT_UP 0x0800    /*!< Up on the directional pad. */
#define CONT_DOWN 0x0400  /*!< Down on the directional pad. */
#define CONT_LEFT 0x0200  /*!< Left on the directional pad. */
#define CONT_RIGHT 0x0100 /*!< Right on the directional pad. */
#define CONT_L 0x0020     /*!< The L trigger. */
#define CONT_R 0x0010     /*!< The R trigger. */
#define CONT_E 0x0008     /*!< The up C button. */
#define CONT_D 0x0004     /*!< The down C button. */
#define CONT_C 0x0002     /*!< The left C button. */
#define CONT_F 0x0001     /*!< The right C button. */

/* The names of the buttons the SDK documents. */
#define A_BUTTON CONT_A         /*!< The A button. */
#define B_BUTTON CONT_B         /*!< The B button. */
#define L_TRIG CONT_L           /*!< The L trigger. */
#define R_TRIG CONT_R           /*!< The R trigger. */
#define Z_TRIG CONT_G           /*!< The Z trigger. */
#define START_BUTTON CONT_START /*!< The Start button. */
#define U_JPAD CONT_UP          /*!< Up on the directional pad. */
#define L_JPAD CONT_LEFT        /*!< Left on the directional pad. */
#define R_JPAD CONT_RIGHT       /*!< Right on the directional pad. */
#define D_JPAD CONT_DOWN        /*!< Down on the directional pad. */
#define U_CBUTTONS CONT_E       /*!< The up C button. */
#define L_CBUTTONS CONT_C       /*!< The left C button. */
#define R_CBUTTONS CONT_F       /*!< The right C button. */
#define D_CBUTTONS CONT_D       /*!< The down C button. */

/** OSContPad.errno of a controller that does not respond. */
#define CONT_NO_RESPONSE_ERROR 8

/* Bits of OSPfs.status. */
#define PFS_INITIALIZED 0x01 /*!< The file system was initialised. */
#define PFS_CORRUPTED 0x02   /*!< The file system is corrupted. */

/** What osContInit() learns of a controller. */
typedef struct {
    u16 type;  /*!< The controller type, from the CONT_ABSOLUTE, CONT_RELATIVE, and CONT_JOYPORT
                  bits. */
    u8 status; /*!< The accessory status, from the CONT_CARD_ON, CONT_CARD_PULL, and
                  CONT_ADDR_CRC_ER bits. */
    u8 errno;  /*!< The channel error of the query, 0 or CONT_NO_RESPONSE_ERROR. */
} OSContStatus;

/** The buttons and stick of a controller. */
typedef struct {
    u16 button; /*!< The pressed buttons, from the CONT_* button bits. */
    s8 stick_x; /*!< The horizontal stick position, positive to the right. */
    s8 stick_y; /*!< The vertical stick position, positive upward. */
    u8 errno;   /*!< The channel error of the read, 0 or CONT_NO_RESPONSE_ERROR. */
} OSContPad;

/** An open Controller Pak. */
typedef struct {
    int status; /*!< The state of the pak, from the PFS_INITIALIZED and PFS_CORRUPTED bits. */
    OSMesgQueue *queue;   /*!< The queue of SI events the pak routines wait on. */
    int channel;          /*!< The controller port of the pak, 0 to 3. */
    u8 id[32];            /*!< A copy of the identification block of the pak. */
    u8 label[32];         /*!< A copy of the label area of the pak. */
    int version;          /*!< The file system version from the identification block. */
    int dir_size;         /*!< The number of entries of the directory. */
    int inode_table;      /*!< The block address of the inode table of bank 0. */
    int minode_table;     /*!< The block address of the mirror of the inode table of bank 0. */
    int dir_table;        /*!< The block address of the directory. */
    int inode_start_page; /*!< The first page of bank 0 available for file data. */
    u8 banks;             /*!< The number of 32 KiB banks of the pak. */
    u8 activebank;        /*!< The bank last selected on the pak, 0xFF when none is known. */
} OSPfs;

/** What osPfsFileState() reports of a file. */
typedef struct {
    u32 file_size;      /*!< The size of the file in bytes. */
    u32 game_code;      /*!< The game code of the game that created the file. */
    u16 company_code;   /*!< The company code of the game that created the file. */
    char ext_name[4];   /*!< The extension of the file name, in the pak character set. */
    char game_name[16]; /*!< The file name, in the pak character set. */
} OSPfsState;

/**
 * Wait until the controllers have powered up, then find which are present.
 *
 * @param mq The queue of SI events, registered with osSetEventMesg() for OS_EVENT_SI.
 * @param bitpattern Receives a bit for each controller that answered.
 * @param data Receives the status of each of the MAXCONTROLLERS controllers.
 * @return 0, or -1 when the SI was busy. A second call returns 0 without a query.
 * @ghidraAddress USA: 0x80265910
 */
s32 osContInit(OSMesgQueue *mq, u8 *bitpattern, OSContStatus *data);

/**
 * Start reading the buttons and sticks of every controller.
 *
 * @param mq The queue of SI events. The routine blocks on it only when the read command must be
 *     written first, and the caller waits on it for the end of the read.
 * @return 0, or -1 when the SI was busy.
 * @ghidraAddress USA: 0x80264760
 */
s32 osContStartReadData(OSMesgQueue *mq);

/**
 * Copy the buttons and sticks of the read osContStartReadData() started.
 *
 * @param data Receives the pad of each of the MAXCONTROLLERS controllers. A pad with a nonzero
 *     errno is not otherwise written.
 * @ghidraAddress USA: 0x80264824
 */
void osContGetReadData(OSContPad *data);

/**
 * Find which controllers have a Controller Pak inserted.
 *
 * @param queue The queue of SI events, blocked on for each transfer.
 * @param pattern Receives a bit for each controller with a Pak.
 * @return 0, or -1 when the SI was busy.
 * @ghidraAddress USA: 0x80261940
 */
s32 osPfsIsPlug(OSMesgQueue *queue, u8 *pattern);

/**
 * Open the Controller Pak of a channel, repairing its ID if needed, and check its file system.
 *
 * @param queue The queue of SI events, blocked on for each transfer.
 * @param pfs Receives the description of the open Pak.
 * @param channel The controller port, 0 to 3.
 * @return 0, or PFS_ERR_NOPACK, PFS_ERR_NEW_PACK, PFS_ERR_CONTRFAIL, PFS_ERR_ID_FATAL,
 *     PFS_ERR_DEVICE, or PFS_ERR_INCONSISTENT.
 * @ghidraAddress USA: 0x80265cd0
 */
s32 osPfsInit(OSMesgQueue *queue, OSPfs *pfs, int channel);

/**
 * Open the Controller Pak of a channel like osPfsInit(), starting from the ID checksum of the
 * caller's stack.
 *
 * @param queue The queue of SI events, blocked on for each transfer.
 * @param pfs Receives the description of the open Pak.
 * @param channel The controller port, 0 to 3.
 * @return 0, or PFS_ERR_NOPACK, PFS_ERR_NEW_PACK, PFS_ERR_CONTRFAIL, PFS_ERR_ID_FATAL,
 *     PFS_ERR_DEVICE, or PFS_ERR_INCONSISTENT.
 * @ghidraAddress USA: 0x80261cb0
 */
s32 osPfsInitPak(OSMesgQueue *queue, OSPfs *pfs, int channel);

/**
 * Read the ID block again, repairing it if needed.
 *
 * @param pfs The open Pak, updated with the ID, the geometry, and the label.
 * @return 0, or PFS_ERR_NEW_PACK, PFS_ERR_CONTRFAIL, PFS_ERR_ID_FATAL, or PFS_ERR_DEVICE.
 * @ghidraAddress USA: 0x80262660
 */
s32 osPfsRepairId(OSPfs *pfs);

/**
 * Remove directory entries whose page chains are broken, and rebuild the page tables from the
 * chains that remain.
 *
 * @param pfs The open Pak. PFS_CORRUPTED in its status is set when the check finds damage.
 * @return 0, or PFS_ERR_NEW_PACK, PFS_ERR_CONTRFAIL, or PFS_ERR_ID_FATAL.
 * @ghidraAddress USA: 0x80262a80
 */
s32 osPfsChecker(OSPfs *pfs);

/**
 * Find a file by its codes, and by its names unless they are NULL.
 *
 * @param pfs The open Pak.
 * @param company_code The company code of the file.
 * @param game_code The game code of the file.
 * @param game_name The 16-byte file name in the Pak character set, or NULL to match every name.
 * @param ext_name The 4-byte extension in the Pak character set, or NULL to match every one.
 * @param file_no Receives the directory index, or -1.
 * @return 0, PFS_ERR_INVALID when no file matches, or PFS_ERR_NEW_PACK or PFS_ERR_CONTRFAIL.
 * @ghidraAddress USA: 0x802628c0
 */
s32 osPfsFindFile(
    OSPfs *pfs, u16 company_code, u32 game_code, u8 *game_name, u8 *ext_name, s32 *file_no);

/**
 * Read or write whole blocks of a file.
 *
 * @param pfs The open Pak.
 * @param file_no The directory index of the file.
 * @param flag PFS_READ or PFS_WRITE.
 * @param offset The byte offset in the file, a multiple of 32.
 * @param size_in_bytes The count of bytes, a positive multiple of 32.
 * @param data_buffer The buffer read into or written from.
 * @return 0, or PFS_ERR_INVALID, PFS_ERR_NEW_PACK, PFS_ERR_INCONSISTENT, PFS_ERR_BAD_DATA, or
 *     PFS_ERR_CONTRFAIL.
 * @ghidraAddress USA: 0x802635dc
 */
s32 osPfsReadWriteFile(
    OSPfs *pfs, s32 file_no, u8 flag, int offset, int size_in_bytes, u8 *data_buffer);

/**
 * Return the bytes of the pages no file uses.
 *
 * @param pfs The open Pak.
 * @param bytes_not_used Receives the free space in bytes, a multiple of 256.
 * @return 0, or PFS_ERR_INVALID, PFS_ERR_NEW_PACK, PFS_ERR_INCONSISTENT, or PFS_ERR_CONTRFAIL.
 * @ghidraAddress USA: 0x802639e0
 */
s32 osPfsFreeBlocks(OSPfs *pfs, s32 *bytes_not_used);

/**
 * Create a file of at least the given size.
 *
 * @param pfs The open Pak.
 * @param company_code The company code of the file, not 0.
 * @param game_code The game code of the file, not 0.
 * @param game_name The 16-byte file name in the Pak character set.
 * @param ext_name The 4-byte extension in the Pak character set.
 * @param file_size_in_bytes The size of the file, rounded up to a whole page of 256 bytes.
 * @param file_no Receives the directory index of the new file.
 * @return 0, or PFS_ERR_INVALID, PFS_ERR_NEW_PACK, PFS_ERR_EXIST, PFS_DATA_FULL, PFS_DIR_FULL,
 *     PFS_ERR_INCONSISTENT, or PFS_ERR_CONTRFAIL.
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
 * @param pfs The open Pak.
 * @param max_files Receives the number of directory entries.
 * @param files_used Receives the number of entries with nonzero codes.
 * @return 0, or PFS_ERR_INVALID, PFS_ERR_NEW_PACK, or PFS_ERR_CONTRFAIL.
 * @ghidraAddress USA: 0x802677e0
 */
s32 osPfsNumFiles(OSPfs *pfs, s32 *max_files, s32 *files_used);

/**
 * Report the size, codes and names of a file.
 *
 * @param pfs The open Pak.
 * @param file_no The directory index of the file.
 * @param state Receives the size, codes, and names of the file.
 * @return 0, or PFS_ERR_INVALID, PFS_ERR_NEW_PACK, PFS_ERR_INCONSISTENT, or PFS_ERR_CONTRFAIL.
 * @ghidraAddress USA: 0x80267930
 */
s32 osPfsFileState(OSPfs *pfs, s32 file_no, OSPfsState *state);

/**
 * Delete a file and free its pages.
 *
 * @param pfs The open Pak.
 * @param company_code The company code of the file, not 0.
 * @param game_code The game code of the file, not 0.
 * @param game_name The 16-byte file name in the Pak character set.
 * @param ext_name The 4-byte extension in the Pak character set.
 * @return 0, or PFS_ERR_INVALID, PFS_ERR_NEW_PACK, PFS_ERR_INCONSISTENT, or PFS_ERR_CONTRFAIL.
 * @ghidraAddress USA: 0x80267c20
 */
s32 osPfsDeleteFile(OSPfs *pfs, u16 company_code, u32 game_code, u8 *game_name, u8 *ext_name);

/**
 * Open the Rumble Pak of a channel and build the commands that start and stop its motor.
 *
 * @param mq The queue of SI events, blocked on for each transfer.
 * @param pfs Receives the description of the Rumble Pak.
 * @param channel The controller port, 0 to 3.
 * @return 0, or PFS_ERR_NOPACK, PFS_ERR_CONTRFAIL, or PFS_ERR_DEVICE when the Pak is not a Rumble
 *     Pak.
 * @ghidraAddress USA: 0x80262370
 */
s32 osMotorInit(OSMesgQueue *mq, OSPfs *pfs, int channel);

/**
 * Start the motor of a Rumble Pak.
 *
 * @param pfs The Rumble Pak osMotorInit() opened.
 * @return 0, or the channel error, or PFS_ERR_CONTRFAIL when the Pak returns a wrong CRC.
 * @ghidraAddress USA: 0x80262088
 */
s32 osMotorStart(OSPfs *pfs);

/**
 * Stop the motor of a Rumble Pak.
 *
 * @param pfs The Rumble Pak osMotorInit() opened.
 * @return 0, or the channel error, or PFS_ERR_CONTRFAIL when the Pak returns a wrong CRC.
 * @ghidraAddress USA: 0x80261f20
 */
s32 osMotorStop(OSPfs *pfs);

#endif
