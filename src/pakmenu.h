#ifndef COASTLINE_PAKMENU_H
#define COASTLINE_PAKMENU_H

/**
 * @file
 * The Controller Pak menu, where a player deletes game notes, and the check of the notes of the
 * Pak at start-up.
 */

#include <PR/os_cont.h>
#include <PR/ultratypes.h>

#include "sprite.h"

/** The notes a Controller Pak holds. */
#define PAK_NOTES 16

/** The states of the Controller Pak menu. */
enum {
    PAK_MENU_MOUNT,
    PAK_MENU_READ_DIRECTORY,
    PAK_MENU_BROWSE,
    PAK_MENU_CONFIRM_DELETE,
    PAK_MENU_ERROR,
    PAK_MENU_DELETE,
    PAK_MENU_NEW_PAK,
    PAK_MENU_EXIT,
    PAK_MENU_FADING_OUT,
};

/** The pictures of the menu. @ghidraAddress USA: 0x8028cb40 */
extern Sprite g_texBackground;

/** @ghidraAddress USA: 0x8028cb70 */
extern Sprite g_texLogo;

/** @ghidraAddress USA: 0x8028d0b0 */
extern Sprite g_texButtonA;

/** @ghidraAddress USA: 0x8028d0e0 */
extern Sprite g_texButtonB;

/** @ghidraAddress USA: 0x8028dd20 */
extern Sprite g_texButtonBSmall;

/**
 * The ASCII of the 66 characters of the names of notes, a space for those it lacks.
 *
 * @ghidraAddress USA: 0x8028dd50
 */
extern const char g_szDecalCharMap[66];

/** @ghidraAddress USA: 0x8028dd94 */
extern u8 g_bPakMenuInitialized;

/** The notes the menu found empty, a bit for each. @ghidraAddress USA: 0x8028dd98 */
extern u16 g_wPakEmptySlotMask;

/** The notes InitPakAndEnumerateFiles() found empty. @ghidraAddress USA: 0x8028dda0 */
extern u16 g_wPakScanEmptyMask;

/** @ghidraAddress USA: 0x8036a060 */
extern u8 g_bPakDirLoaded;

/** @ghidraAddress USA: 0x8036a061 */
extern u8 g_bPakSelectedNote;

/** @ghidraAddress USA: 0x8036a062 */
extern u8 g_bPakFirstValidNote;

/** @ghidraAddress USA: 0x8036a063 */
extern u8 g_bPakLastValidNote;

/** @ghidraAddress USA: 0x8036a064 */
extern s32 g_dwPakNoteCount;

/** The free space of the Pak in bytes. @ghidraAddress USA: 0x8036a068 */
extern s32 g_dwPakFreeBlocks;

/** The notes of the Pak the menu shows. @ghidraAddress USA: 0x8036a070 */
extern OSPfsState g_aPakDirEntries[PAK_NOTES];

/** @ghidraAddress USA: 0x8036a270 */
extern u8 g_bPakMenuState;

/** @ghidraAddress USA: 0x8036a278 */
extern s32 g_nPakFilesUsed;

/** The notes InitPakAndEnumerateFiles() reads. @ghidraAddress USA: 0x8036a280 */
extern OSPfsState g_aPakFileStates[PAK_NOTES];

/** The file system of the Pak of the first controller. @ghidraAddress USA: 0x80369ec0 */
extern OSPfs g_aPakPfs;

/** The result of the last Controller Pak call. @ghidraAddress USA: 0x802724f0 */
extern s32 gControllerPakState;

/** The identity of the Pak the game saves to. @ghidraAddress USA: 0x803163e0 */
extern u8 g_aSaveIdPort0[32];

/**
 * The ASCII of a character of the name of a note.
 *
 * @ghidraAddress USA: 0x80253460
 */
u8 DecalCharToAscii(u8 c);

/**
 * Run one frame of the Controller Pak menu, which draws and runs itself until it fades out to
 * the main menu.
 *
 * @ghidraAddress USA: 0x802534dc
 */
void ProcessPakMenuScreen(void);

/**
 * Mount the Pak of the first controller, read its notes, and check that each belongs to this
 * game.
 *
 * @return 0 if a note of another game was found, and 1 otherwise, failures included.
 * @ghidraAddress USA: 0x80254620
 */
s32 InitPakAndEnumerateFiles(void);

#endif
