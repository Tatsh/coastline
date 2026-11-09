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

#ifdef __cplusplus
extern "C" {
#endif

/** The notes a Controller Pak holds. */
#define PAK_NOTES 16

/* The codes of the notes of this game. */
#if TGR_REGION == TGR_REGION_US
#define GAME_CODE 0x4E475245 /*!< The game code of the notes, "NGRE" in ASCII. */
#define COMPANY_CODE 0x3544  /*!< The company code of the notes, "5D" in ASCII. */
#elif TGR_REGION == TGR_REGION_EU
#define GAME_CODE 0x4E545250 /*!< The game code of the notes, "NTRP" in ASCII. */
#define COMPANY_CODE 0x3746  /*!< The company code of the notes, "7F" in ASCII. */
#elif TGR_REGION == TGR_REGION_ASIA
#define GAME_CODE 0x4E475258 /*!< The game code of the notes, "NGRX" in ASCII. */
#define COMPANY_CODE 0x3746  /*!< The company code of the notes, "7F" in ASCII. */
#else
#define GAME_CODE 0x4E54524A /*!< The game code of the notes, "NTRJ" in ASCII. */
#define COMPANY_CODE 0x3238  /*!< The company code of the notes, "28" in ASCII. */
#endif                       /* TGR_REGION == TGR_REGION_US */

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
 * @ghidraAddress Asia: 0x8028C690
 * @ghidraAddress EUR: 0x8028f2a0
 * @ghidraAddress JPN: 0x8028ecf0
 */
extern Sprite g_texBackground;

/**
 * The logo sprite shared by the Controller Pak and other menu screens.
 *
 * @ghidraAddress USA: 0x8028cb70
 * @ghidraAddress Asia: 0x8028C6C0
 * @ghidraAddress EUR: 0x8028f2d0
 * @ghidraAddress JPN: 0x8028ed20
 */
extern Sprite g_texLogo;

/**
 * The A-button prompt sprite shared by menu screens.
 *
 * @ghidraAddress USA: 0x8028d0b0
 * @ghidraAddress Asia: 0x8028CC00
 * @ghidraAddress EUR: 0x8028f810
 * @ghidraAddress JPN: 0x8028f280
 */
extern Sprite g_texButtonA;

/**
 * The B-button prompt sprite shared by menu screens.
 *
 * @ghidraAddress USA: 0x8028d0e0
 * @ghidraAddress Asia: 0x8028CC30
 * @ghidraAddress EUR: 0x8028f840
 * @ghidraAddress JPN: 0x8028f2b0
 */
extern Sprite g_texButtonB;

/** @cond */
/**
 * @ghidraAddress USA: 0x8028dd20
 * @ghidraAddress Asia: 0x8028D870
 * @ghidraAddress EUR: 0x802904c0
 * @ghidraAddress JPN: 0x8028ff10
 */
extern Sprite g_texButtonBSmall;
/** @endcond */

#if TGR_REGION == TGR_REGION_JP
/** The number of characters in the Japanese Controller Pak note name map. */
#define DECAL_CHAR_COUNT 149
#else
/** The number of characters in a non-Japanese Controller Pak note name map. */
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
 * @ghidraAddress Asia: 0x8028D8A0
 * @ghidraAddress EUR: 0x802904f0
 * @ghidraAddress JPN: 0x8028ff40
 */
extern const DecalCharMap g_szDecalCharMap;

/** @cond */
/**
 * @ghidraAddress USA: 0x8028dd94
 * @ghidraAddress Asia: 0x8028D8E4
 * @ghidraAddress EUR: 0x80290534
 * @ghidraAddress JPN: 0x8028ffd8
 */
extern u8 g_bPakMenuInitialized;
/** @endcond */

/**
 * The notes the menu found empty, a bit for each.
 *
 * @ghidraAddress USA: 0x8028dd98
 * @ghidraAddress Asia: 0x8028D8E8
 * @ghidraAddress EUR: 0x80290538
 * @ghidraAddress JPN: 0x8028ffdc
 */
extern u16 g_wPakEmptySlotMask;

/** @cond */
/**
 * @ghidraAddress USA: 0x8036a060
 * @ghidraAddress Asia: 0x80369B10
 * @ghidraAddress EUR: 0x8036c560
 * @ghidraAddress JPN: 0x8036b1d0
 */
extern u8 g_bPakDirLoaded;

/**
 * @ghidraAddress USA: 0x8036a061
 * @ghidraAddress Asia: 0x80369B11
 * @ghidraAddress EUR: 0x8036c561
 * @ghidraAddress JPN: 0x8036b1d1
 */
extern u8 g_bPakSelectedNote;

/**
 * @ghidraAddress USA: 0x8036a062
 * @ghidraAddress Asia: 0x80369B12
 * @ghidraAddress EUR: 0x8036c562
 * @ghidraAddress JPN: 0x8036b1d2
 */
extern u8 g_bPakFirstValidNote;

/**
 * @ghidraAddress USA: 0x8036a063
 * @ghidraAddress Asia: 0x80369B13
 * @ghidraAddress EUR: 0x8036c563
 * @ghidraAddress JPN: 0x8036b1d3
 */
extern u8 g_bPakLastValidNote;

/**
 * @ghidraAddress USA: 0x8036a064
 * @ghidraAddress Asia: 0x80369B14
 * @ghidraAddress EUR: 0x8036c564
 * @ghidraAddress JPN: 0x8036b1d4
 */
extern s32 g_dwPakNoteCount;
/** @endcond */

/**
 * The free space of the Pak in bytes.
 *
 * @ghidraAddress USA: 0x8036a068
 * @ghidraAddress Asia: 0x80369B18
 * @ghidraAddress EUR: 0x8036c568
 * @ghidraAddress JPN: 0x8036b1d8
 */
extern s32 g_dwPakFreeBlocks;

/**
 * The notes of the Pak the menu shows.
 *
 * @ghidraAddress USA: 0x8036a070
 * @ghidraAddress Asia: 0x80369B20
 * @ghidraAddress EUR: 0x8036c570
 * @ghidraAddress JPN: 0x8036b1e0
 */
extern OSPfsState g_aPakDirEntries[PAK_NOTES];

/** @cond */
/**
 * @ghidraAddress USA: 0x8036a270
 * @ghidraAddress Asia: 0x80369D20
 * @ghidraAddress EUR: 0x8036c770
 * @ghidraAddress JPN: 0x8036b3e0
 */
extern u8 g_bPakMenuState;

/**
 * @ghidraAddress USA: 0x8036a278
 * @ghidraAddress Asia: 0x80369D28
 * @ghidraAddress EUR: 0x8036c778
 * @ghidraAddress JPN: 0x8036b3e8
 */
extern s32 g_nPakFilesUsed;
/** @endcond */

/**
 * The notes InitPakAndEnumerateFiles() reads.
 *
 * @ghidraAddress USA: 0x8036a280
 * @ghidraAddress Asia: 0x80369D30
 * @ghidraAddress EUR: 0x8036c780
 * @ghidraAddress JPN: 0x8036b3f0
 */
extern OSPfsState g_aPakFileStates[PAK_NOTES];

/**
 * The file system of the Pak of each controller.
 *
 * @ghidraAddress USA: 0x80369ec0
 * @ghidraAddress Asia: 0x80369970
 * @ghidraAddress EUR: 0x8036c3c0
 * @ghidraAddress JPN: 0x8036b030
 */
extern OSPfs g_aPakPfs[MAXCONTROLLERS];

/**
 * The result of the last Controller Pak call.
 *
 * @ghidraAddress USA: 0x802724f0
 * @ghidraAddress Asia: 0x80272040
 * @ghidraAddress EUR: 0x802744a0
 * @ghidraAddress JPN: 0x80273f50
 */
extern s32 gControllerPakState;

/**
 * The identity of the Pak each of the first two players saves to, to tell when another Pak is
 * inserted.
 *
 * @ghidraAddress USA: 0x803163e0
 * @ghidraAddress Asia: 0x80315E90
 * @ghidraAddress EUR: 0x803180c0
 * @ghidraAddress JPN: 0x80316cf0
 */
extern u8 g_aabSavePakIds[2][32];

/**
 * The ASCII of a character of the name of a note.
 *
 * @param c The character code of the note name.
 * @return The ASCII character, or a space for a code outside the map.
 * @ghidraAddress USA: 0x80253460
 * @ghidraAddress Asia: 0x80252FB0
 * @ghidraAddress EUR: 0x802549c0
 * @ghidraAddress JPN: 0x802541b0
 */
u8 DecalCharToAscii(u8 c);

/**
 * Run one frame of the Controller Pak menu, which draws and runs itself until it fades out to
 * the main menu.
 *
 * @ghidraAddress USA: 0x802534dc
 * @ghidraAddress Asia: 0x8025302C
 * @ghidraAddress EUR: 0x80254a3c
 * @ghidraAddress JPN: 0x8025422c
 */
void ProcessPakMenuScreen(void);

/**
 * Mount the Pak of the first controller, read its notes, and check that each belongs to this
 * game.
 *
 * @return 0 if a note of another game was found, and 1 otherwise, failures included.
 * @ghidraAddress USA: 0x80254620
 * @ghidraAddress Asia: 0x80254170
 * @ghidraAddress EUR: 0x80255bc0
 * @ghidraAddress JPN: 0x802554c0
 */
s32 InitPakAndEnumerateFiles(void);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_PAKMENU_H */
