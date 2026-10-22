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

/* The codes of the notes of this game. */
#if TGR_REGION == TGR_REGION_US
#define GAME_CODE 0x4E475245 /*!< The game code of the notes, "NGRE" in ASCII. */
#define COMPANY_CODE 0x3544  /*!< The company code of the notes, "5D" in ASCII. */
#elif TGR_REGION == TGR_REGION_EU
#define GAME_CODE 0x4E545250 /*!< The game code of the notes, "NTRP" in ASCII. */
#define COMPANY_CODE 0x3746  /*!< The company code of the notes, "7F" in ASCII. */
#else
#define GAME_CODE 0x4E54524A /*!< The game code of the notes, "NTRJ" in ASCII. */
#define COMPANY_CODE 0x3238  /*!< The company code of the notes, "28" in ASCII. */
#endif

/** The states of the Controller Pak menu. */
enum {
    PAK_MENU_MOUNT,          /*!< Mount the Controller Pak of the first controller. */
    PAK_MENU_READ_DIRECTORY, /*!< Read the notes and the free space of the Pak. */
    PAK_MENU_BROWSE,         /*!< Move through the notes, with A to delete one and Start to exit. */
    PAK_MENU_CONFIRM_DELETE, /*!< Prompt for confirmation of the deletion of the selected note. */
    PAK_MENU_ERROR,          /*!< Show an error of the Pak until a button is pressed. */
    PAK_MENU_DELETE,         /*!< Delete the selected note. */
    PAK_MENU_NEW_PAK,        /*!< Report that a new Pak was inserted, then mount it. */
    PAK_MENU_EXIT,           /*!< Start the fade out of the menu. */
    PAK_MENU_FADING_OUT,     /*!< Wait for the fade out, then return to the main menu. */
};

/**
 * The pictures of the menu.
 *
 * @ghidraAddress USA: 0x8028cb40
 * @ghidraAddress JPN: 0x8028ecf0
 * @ghidraAddress EUR: 0x8028f2a0
 */
extern Sprite g_texBackground;

/**
 * @ghidraAddress USA: 0x8028cb70
 * @ghidraAddress JPN: 0x8028ed20
 * @ghidraAddress EUR: 0x8028f2d0
 */
extern Sprite g_texLogo;

/**
 * @ghidraAddress USA: 0x8028d0b0
 * @ghidraAddress JPN: 0x8028f280
 * @ghidraAddress EUR: 0x8028f810
 */
extern Sprite g_texButtonA;

/**
 * @ghidraAddress USA: 0x8028d0e0
 * @ghidraAddress JPN: 0x8028f2b0
 * @ghidraAddress EUR: 0x8028f840
 */
extern Sprite g_texButtonB;

/**
 * @ghidraAddress USA: 0x8028dd20
 * @ghidraAddress JPN: 0x8028ff10
 * @ghidraAddress EUR: 0x802904c0
 */
extern Sprite g_texButtonBSmall;

#if TGR_REGION == TGR_REGION_JP
/* The Japanese table has 83 more characters. */
#define DECAL_CHAR_COUNT 149
#else
#define DECAL_CHAR_COUNT 66
#endif

/** The table of the characters of the names of notes, which is copied whole to read one. */
typedef struct DecalCharMap {
    char chars[DECAL_CHAR_COUNT]; /*!< The ASCII of each character, a space for those it lacks. */
} DecalCharMap;

/**
 * The ASCII of the characters of the names of notes, a space for those it lacks.
 *
 * @ghidraAddress USA: 0x8028dd50
 * @ghidraAddress JPN: 0x8028ff40
 * @ghidraAddress EUR: 0x802904f0
 */
extern const DecalCharMap g_szDecalCharMap;

/**
 * @ghidraAddress USA: 0x8028dd94
 * @ghidraAddress JPN: 0x8028ffd8
 * @ghidraAddress EUR: 0x80290534
 */
extern u8 g_bPakMenuInitialized;

/**
 * The notes the menu found empty, a bit for each.
 *
 * @ghidraAddress USA: 0x8028dd98
 * @ghidraAddress JPN: 0x8028ffdc
 * @ghidraAddress EUR: 0x80290538
 */
extern u16 g_wPakEmptySlotMask;

/**
 * @ghidraAddress USA: 0x8036a060
 * @ghidraAddress JPN: 0x8036b1d0
 * @ghidraAddress EUR: 0x8036c560
 */
extern u8 g_bPakDirLoaded;

/**
 * @ghidraAddress USA: 0x8036a061
 * @ghidraAddress JPN: 0x8036b1d1
 * @ghidraAddress EUR: 0x8036c561
 */
extern u8 g_bPakSelectedNote;

/**
 * @ghidraAddress USA: 0x8036a062
 * @ghidraAddress JPN: 0x8036b1d2
 * @ghidraAddress EUR: 0x8036c562
 */
extern u8 g_bPakFirstValidNote;

/**
 * @ghidraAddress USA: 0x8036a063
 * @ghidraAddress JPN: 0x8036b1d3
 * @ghidraAddress EUR: 0x8036c563
 */
extern u8 g_bPakLastValidNote;

/**
 * @ghidraAddress USA: 0x8036a064
 * @ghidraAddress JPN: 0x8036b1d4
 * @ghidraAddress EUR: 0x8036c564
 */
extern s32 g_dwPakNoteCount;

/**
 * The free space of the Pak in bytes.
 *
 * @ghidraAddress USA: 0x8036a068
 * @ghidraAddress JPN: 0x8036b1d8
 * @ghidraAddress EUR: 0x8036c568
 */
extern s32 g_dwPakFreeBlocks;

/**
 * The notes of the Pak the menu shows.
 *
 * @ghidraAddress USA: 0x8036a070
 * @ghidraAddress JPN: 0x8036b1e0
 * @ghidraAddress EUR: 0x8036c570
 */
extern OSPfsState g_aPakDirEntries[PAK_NOTES];

/**
 * @ghidraAddress USA: 0x8036a270
 * @ghidraAddress JPN: 0x8036b3e0
 * @ghidraAddress EUR: 0x8036c770
 */
extern u8 g_bPakMenuState;

/**
 * @ghidraAddress USA: 0x8036a278
 * @ghidraAddress JPN: 0x8036b3e8
 * @ghidraAddress EUR: 0x8036c778
 */
extern s32 g_nPakFilesUsed;

/**
 * The notes InitPakAndEnumerateFiles() reads.
 *
 * @ghidraAddress USA: 0x8036a280
 * @ghidraAddress JPN: 0x8036b3f0
 * @ghidraAddress EUR: 0x8036c780
 */
extern OSPfsState g_aPakFileStates[PAK_NOTES];

/**
 * The file system of the Pak of each controller.
 *
 * @ghidraAddress USA: 0x80369ec0
 * @ghidraAddress JPN: 0x8036b030
 * @ghidraAddress EUR: 0x8036c3c0
 */
extern OSPfs g_aPakPfs[MAXCONTROLLERS];

/**
 * The result of the last Controller Pak call.
 *
 * @ghidraAddress USA: 0x802724f0
 * @ghidraAddress JPN: 0x80273f50
 * @ghidraAddress EUR: 0x802744a0
 */
extern s32 gControllerPakState;

/**
 * The identity of the Pak each of the first two players saves to, to tell when another Pak is
 * inserted.
 *
 * @ghidraAddress USA: 0x803163e0
 * @ghidraAddress JPN: 0x80316cf0
 * @ghidraAddress EUR: 0x803180c0
 */
extern u8 g_aabSavePakIds[2][32];

/**
 * The ASCII of a character of the name of a note.
 *
 * @param c The character code of the note name.
 * @return The ASCII character, or a space for a code outside the map.
 * @ghidraAddress USA: 0x80253460
 * @ghidraAddress JPN: 0x802541b0
 * @ghidraAddress EUR: 0x802549c0
 */
u8 DecalCharToAscii(u8 c);

/**
 * Run one frame of the Controller Pak menu, which draws and runs itself until it fades out to
 * the main menu.
 *
 * @ghidraAddress USA: 0x802534dc
 * @ghidraAddress EUR: 0x80254a3c
 * @ghidraAddress JPN: 0x8025422c
 * @stub{JPN}
 */
void ProcessPakMenuScreen(void);

/**
 * Mount the Pak of the first controller, read its notes, and check that each belongs to this
 * game.
 *
 * @return 0 if a note of another game was found, and 1 otherwise, failures included.
 * @ghidraAddress USA: 0x80254620
 * @ghidraAddress JPN: 0x802554c0
 * @ghidraAddress EUR: 0x80255bc0
 */
s32 InitPakAndEnumerateFiles(void);

#endif
