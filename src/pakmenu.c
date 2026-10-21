#include "pakmenu.h"

#include <PR/os.h>
#include <PR/os_libc.h>

#include "fade.h"
#include "game.h"
#include "heap.h"
#include "input.h"
#include "render.h"
#include "rumble.h"
#include "system.h"
#include "text.h"

/* The step of the fades of the menu. */
#define PAK_MENU_FADE_STEP 0.2f

/* The pages of a Controller Pak, and the bytes of a page. */
#define PAK_PAGES 123
#define PAK_PAGE_SHIFT 8

/* The rows of the table of notes. */
#define TABLE_TOP 0x3F
#define TABLE_ROW 9

/* The width of the screen the dialogs are centered in. */
#define DIALOG_SCREEN_WIDTH 0x280

u8 DecalCharToAscii(u8 c) {
    /* The original copies the whole table to the stack to read one character of it. */
    DecalCharMap map = g_szDecalCharMap;

    return c < sizeof(map.chars) ? map.chars[c] : ' ';
}

/* The note under the cursor is green on a white table, which is otherwise gray. */
static void SetNoteColors(s32 note) {
    if (g_dwPakNoteCount != 0 && note == g_bPakSelectedNote) {
        SetTextCustomColors(0x80, 0xFF, 0x80, 0x00, 0xFF, 0x00);
    } else {
        SetTextCustomColors(0xFF, 0xFF, 0xFF, 0x80, 0x80, 0x80);
    }
}

/* Mount the Pak, repairing its identity if it is fatal, unless the controller holds a Rumble
 * Pak. Return FALSE for a Rumble Pak. */
static s32 MountPak(void) {
    gControllerPakState = osPfsInitPak(&g_SiMesgQueue, &g_aPakPfs[0], 0);
    if (gControllerPakState != 0 && gControllerPakState == PFS_ERR_ID_FATAL) {
        if (osMotorInit(&g_SiMesgQueue, &g_aContPfs[0], 0) == 0) {
            return FALSE;
        }
        gControllerPakState = osPfsRepairId(&g_aPakPfs[0]);
        if (gControllerPakState == 0) {
            /* The result is discarded, and the state remains that of the repair. */
            osPfsInitPak(&g_SiMesgQueue, &g_aPakPfs[0], 0);
        }
    }
    return TRUE;
}

/* Whether a Pak call succeeded, or failed with an inconsistency the checker repairs. */
static s32 PakCallSucceeded(void) {
    if (gControllerPakState == 0) {
        return TRUE;
    }
    return gControllerPakState == PFS_ERR_INCONSISTENT && osPfsChecker(&g_aPakPfs[0]) == 0;
}

static s32 IsNoteEmpty(s32 note) {
    return (g_wPakEmptySlotMask & (1 << note)) != 0;
}

/* Draw the note in a row of the table: its name, its extension and its pages. */
static void DrawNote(s32 note) {
    OSPfsState *entry = &g_aPakDirEntries[note];
    char name[20], text[16];
    char *p;
    u8 c;
    s32 y = note * TABLE_ROW + TABLE_TOP;
    s32 i;

    for (p = name, i = 0; p != name + 16; i++) {
        c = DecalCharToAscii(entry->game_name[i]);
        *p = c;
        if (c == '\0') {
            break;
        }
        p++;
    }
    c = DecalCharToAscii(entry->ext_name[0]);
    if (c != '\0') {
        p[0] = ' ';
        p[1] = '.';
        p[2] = c;
        p[3] = '\0';
    } else {
        p[0] = '\0';
    }

    SetTextAlignLeft();
    SetNoteColors(note);
    DrawText(name, 0x62, y);
    sprintf(text, "%d", entry->file_size >> PAK_PAGE_SHIFT);
    SetTextAlignRight();
    DrawText(text, 0xF4, y);
}

void ProcessPakMenuScreen(void) {
    u8 plugged;
    char text[16], prompt[32];
    s32 note, y, maxFiles;
    s32 width, left, buttonY, okX, cancelX, labelY;
    PlayerInputState *input;

    g_wSerialInterfaceIdle = 0;
    if (!g_bPakMenuInitialized) {
        g_bPakMenuInitialized = TRUE;
        g_bPakDirLoaded = FALSE;
        osPfsIsPlug(&g_SiMesgQueue, &plugged);
        if (!(plugged & 1)) {
            SetCurrentScreenCallback(MainMenuEntry);
            g_bPakMenuInitialized = FALSE;
            g_wSerialInterfaceIdle = 1;
            return;
        }
        ResetHeap();
        LoadTextureFromRom(&g_texBackground);
        LoadTextureFromRom(&g_texLogo);
        LoadTextureFromRom(&g_texButtonA);
        LoadTextureFromRom(&g_texButtonB);
        LoadTextureFromRom(&g_texButtonBSmall);
        gControllerPakState = 0;
        g_bPakMenuState = PAK_MENU_READ_DIRECTORY;
        SetFadeTarget(1.0f, PAK_MENU_FADE_STEP);
    }

    UpdateFrameTiming();
    UpdateFadeTransitions();
    StartHighResFrame();
    SetupProjectionMatrix(g_dwScreenWidth, g_dwScreenHeight);
    SetViewport(0, 0, g_dwScreenWidth, g_dwScreenHeight, TRUE);
    ClearDepthBuffer();
    ClearScreen(0, 0, 0);
    DrawSpriteTinted(&g_texBackground, 0x50, 8, 0x80);
    DrawSpriteRegion(
        &g_texLogo, g_texLogo.originX + 0x18, g_texLogo.originY + 2, 0x208, 0x41, 0x80, 0x80, 0x80);

    SetTextAlignCenter();
    SetTextStyleNormal();
    SetTextSize(0x16);
    DrawText("%ryCONTROLLER PAK MENU", 0xA0, 0x20);
    SetTextAlignLeft();
    SetTextSize(0xC);
    DrawText("%yw#", 0x48, 0x34);
    DrawText("%ywGAME NOTE", 0x74, 0x34);
    SetTextAlignRight();
    DrawText("%ywPAGES", 0xFA, 0x34);

    /* The numbers of the notes, then the notes the directory holds. */
    SetTextSize(9);
    y = TABLE_TOP;
    for (note = 0; note < PAK_NOTES; note++) {
        sprintf(text, "%d", note + 1);
        SetNoteColors(note);
        DrawText(text, 0x52, y);
        y += TABLE_ROW;
    }
    if (g_bPakDirLoaded && g_bPakMenuState != PAK_MENU_ERROR) {
        for (note = 0; note < PAK_NOTES; note++) {
            if (!IsNoteEmpty(note)) {
                DrawNote(note);
            }
        }
    }

    input = &g_aPlayerInputState[0];
    switch (g_bPakMenuState) {
    case PAK_MENU_MOUNT:
        if (!MountPak()) {
            gControllerPakState = 0;
            SetCurrentScreenCallback(MainMenuEntry);
            g_wSerialInterfaceIdle = 1;
            return;
        }
        if (gControllerPakState != 0) {
            g_bPakMenuState = PAK_MENU_ERROR;
            break;
        }
        bcopy(g_aPakPfs[0].id, g_aabSavePakIds[0], sizeof(g_aPakPfs[0].id));
        g_bPakMenuState = PAK_MENU_READ_DIRECTORY;
        break;

    case PAK_MENU_READ_DIRECTORY:
        gControllerPakState = osPfsNumFiles(&g_aPakPfs[0], &maxFiles, &g_dwPakNoteCount);
        if (!PakCallSucceeded()) {
            g_bPakMenuState = PAK_MENU_ERROR;
            break;
        }
        for (note = 0; note < PAK_NOTES; note++) {
            if (osPfsFileState(&g_aPakPfs[0], note, &g_aPakDirEntries[note]) != 0) {
                g_wPakEmptySlotMask |= 1 << note;
            }
        }
        g_bPakFirstValidNote = 0;
        for (note = 0; note < PAK_NOTES; note++) {
            if (!IsNoteEmpty(note)) {
                g_bPakFirstValidNote = note;
                break;
            }
        }
        g_bPakLastValidNote = 0;
        for (note = 0; note < PAK_NOTES; note++) {
            if (!IsNoteEmpty(note)) {
                g_bPakLastValidNote = note;
            }
        }
        gControllerPakState = osPfsFreeBlocks(&g_aPakPfs[0], &g_dwPakFreeBlocks);
        if (!PakCallSucceeded()) {
            g_bPakMenuState = PAK_MENU_ERROR;
            break;
        }
        g_bPakDirLoaded = TRUE;
        g_bPakMenuState = PAK_MENU_BROWSE;
        g_bPakSelectedNote = g_bPakFirstValidNote;
        break;

    case PAK_MENU_BROWSE:
        UpdateStickRepeat(input);
        if (input->buttons & INPUT_UP) {
            TransferFlags(input, INPUT_UP);
            if (g_dwPakNoteCount != 0 && g_bPakFirstValidNote != g_bPakSelectedNote) {
                do {
                    g_bPakSelectedNote--;
                } while (IsNoteEmpty(g_bPakSelectedNote) &&
                         g_bPakSelectedNote != g_bPakFirstValidNote);
            }
        } else if (input->buttons & INPUT_DOWN) {
            TransferFlags(input, INPUT_DOWN);
            if (g_dwPakNoteCount != 0 && g_bPakLastValidNote != g_bPakSelectedNote) {
                do {
                    g_bPakSelectedNote++;
                } while (IsNoteEmpty(g_bPakSelectedNote) &&
                         g_bPakSelectedNote != g_bPakLastValidNote);
            }
        } else if (input->buttons & INPUT_A) {
            TransferFlags(input, INPUT_A);
            if (g_dwPakNoteCount != 0 && IsFadedToWhite()) {
                g_bPakMenuState = PAK_MENU_CONFIRM_DELETE;
            }
        } else if (input->buttons & INPUT_START) {
            if (IsFadedToWhite()) {
                TransferFlags(input, INPUT_START);
                g_bPakMenuState = PAK_MENU_EXIT;
            }
        }
        break;

    case PAK_MENU_CONFIRM_DELETE:
        width = g_bPakSelectedNote + 1 < 10 ? 0x128 : 0x134;
        buttonY = 0x110 - g_texButtonA.height;
        left = (DIALOG_SCREEN_WIDTH - width) >> 1;
        DrawRectBorder(left, 0xC8, width, 0x50, 3, 0, 0, 0x80, 0x80, 0x80);
        SetTextAlignCenter();
        SetTextStyleNormal();
        SetTextSize(0xC);
        sprintf(prompt, "%%ywDELETE GAME NOTE #%d?", g_bPakSelectedNote + 1);
        DrawText(prompt, 0x9F, 0x73);
        SetTextAlignLeft();
        SetTextSize(0xA);
        okX = left + 0x38;
        labelY = (buttonY + 0x12) >> 1;
        DrawText("%wwOK", (u32)(okX + g_texButtonA.width + 7) >> 1, labelY);
        cancelX = left + width - 0x9C;
        DrawText("%wwCANCEL", (u32)(cancelX + g_texButtonB.width + 7) >> 1, labelY);
        DrawTexturedSprite(&g_texButtonA, okX, buttonY);
        DrawTexturedSprite(&g_texButtonB, cancelX, buttonY);
        if (input->buttons & INPUT_A) {
            TransferFlags(input, INPUT_A);
            g_bPakMenuState = PAK_MENU_DELETE;
        } else if (input->buttons & INPUT_B) {
            TransferFlags(input, INPUT_B);
            g_bPakMenuState = PAK_MENU_BROWSE;
        }
        break;

    case PAK_MENU_ERROR:
        DrawRectBorder(0xB1, 0xCB, 0x11E, 0x4A, 3, 0, 0, 0x80, 0x80, 0x80);
        SetTextAlignCenter();
        SetTextStyleNormal();
        SetTextSize(0xC);
        DrawText("%ywCONTROLLER PAK ERROR", 0x9F, 0x75);
        DrawText("%ywHAS BEEN DETECTED", 0x9F, 0x83);
        if (input->buttons & (INPUT_START | INPUT_A | INPUT_B)) {
            TransferFlags(input, INPUT_START | INPUT_A | INPUT_B);
            g_bPakMenuState = PAK_MENU_EXIT;
        }
        g_dwPakFreeBlocks = 0;
        maxFiles = PAK_NOTES;
        (void)maxFiles;
        g_dwPakNoteCount = 0;
        break;

    case PAK_MENU_DELETE: {
        OSPfsState *entry = &g_aPakDirEntries[g_bPakSelectedNote];
        u8 selected;

        gControllerPakState = osPfsDeleteFile(&g_aPakPfs[0],
                                              entry->company_code,
                                              entry->game_code,
                                              (u8 *)entry->game_name,
                                              (u8 *)entry->ext_name);
        if (gControllerPakState != 0) {
            g_bPakMenuState =
                gControllerPakState == PFS_ERR_NEW_PACK ? PAK_MENU_NEW_PAK : PAK_MENU_ERROR;
            break;
        }
        selected = g_bPakSelectedNote;
        entry = &g_aPakDirEntries[selected];
        g_wPakEmptySlotMask |= 1 << selected;
        entry->game_name[0] = '\0';
        entry->ext_name[0] = '\0';
        g_dwPakFreeBlocks += entry->file_size;
        entry->file_size = 0;
        if (g_dwPakNoteCount > 0) {
            g_dwPakNoteCount--;
        }

        /* Move the ends of the notes and the cursor off the deleted note. */
        if (g_bPakLastValidNote == selected) {
            do {
                g_bPakLastValidNote--;
            } while (IsNoteEmpty(g_bPakLastValidNote) && g_bPakLastValidNote != 0);
            do {
                g_bPakSelectedNote--;
            } while (IsNoteEmpty(g_bPakSelectedNote) && g_bPakSelectedNote != 0);
        } else {
            if (g_bPakFirstValidNote == selected) {
                do {
                    g_bPakFirstValidNote++;
                } while (IsNoteEmpty(g_bPakFirstValidNote) &&
                         g_bPakFirstValidNote != g_bPakLastValidNote);
            }
            do {
                g_bPakSelectedNote++;
            } while (IsNoteEmpty(g_bPakSelectedNote) && g_bPakSelectedNote != g_bPakLastValidNote);
        }
        g_bPakMenuState = PAK_MENU_BROWSE;
        break;
    }

    case PAK_MENU_NEW_PAK:
        SetTextStyleNormal();
        SetTextAlignLeft();
        SetTextSize(0xC);
        DrawRectBorder(0xAF, 0xB5, 0x122, 0x89, 3, 0, 0, 0x80, 0x80, 0x80);
        DrawText("%ywA NEW CONTROLLER PAK", 0x5F, 0x6A);
        DrawText("%ywWAS INSERTED.", 0x5F, 0x77);
        DrawText("%ywTHIS CONTROLLER PAK", 0x5F, 0x8A);
        DrawText("%ywWILL BE USED.", 0x5F, 0x97);
        if (input->buttons & (INPUT_Z | INPUT_A | INPUT_B)) {
            TransferFlags(input, INPUT_Z | INPUT_A | INPUT_B);
            g_bPakDirLoaded = FALSE;
            gControllerPakState = 0;
            g_bPakMenuState = PAK_MENU_MOUNT;
        }
        break;

    case PAK_MENU_EXIT:
        SetFadeTarget(0.0f, PAK_MENU_FADE_STEP);
        g_bPakMenuState = PAK_MENU_FADING_OUT;
        break;
    }

    /* The pages used and free, and the buttons. */
    SetTextAlignLeft();
    SetTextSize(0xA);
    DrawText("%ywPAGES USED:", 0x47, 0xD3);
    sprintf(text, "%%yw%d", PAK_PAGES - (g_dwPakFreeBlocks >> PAK_PAGE_SHIFT));
    DrawText(text, 0x85, 0xD3);
    DrawText("%ywPAGES FREE:", 0xAC, 0xD3);
    sprintf(text, "%%yw%d", g_dwPakFreeBlocks >> PAK_PAGE_SHIFT);
    SetTextAlignRight();
    DrawText(text, 0xFA, 0xD3);
    DrawTexturedSprite(&g_texButtonA, 0xD6, 0x1C8 - g_texButtonA.height);
    /* The original places the small B button by the height of the A button. */
    DrawTexturedSprite(&g_texButtonBSmall, 0x158, 0x1C8 - g_texButtonA.height);
    SetTextAlignLeft();
    DrawText("%wwSELECT", 0x7B, 0xE1);
    DrawText("%wwEXIT", 0xBC, 0xE1);

    gDPPipeSync(gDisplayListPtr++);
    ApplyScreenFade();
    FlushDisplayList();
    if (IsFadedToBlack()) {
        g_bPakMenuInitialized = FALSE;
        gControllerPakState = 0;
        StartHighResFrame();
        ClearScreen(0, 0, 0);
        FlushDisplayList();
        osViBlack(TRUE);
        StartHighResFrame();
        ClearScreen(0, 0, 0);
        FlushDisplayList();
        SetCurrentScreenCallback(MainMenuEntry);
    }
    g_wSerialInterfaceIdle = 1;
}

s32 InitPakAndEnumerateFiles(void) {
    s32 maxFiles, note;
    OSPfsState *state;

    g_wSerialInterfaceIdle = 0;
    if (!MountPak()) {
        g_wSerialInterfaceIdle = 1;
        return TRUE;
    }
    if (gControllerPakState != 0) {
        g_wSerialInterfaceIdle = 1;
        return TRUE;
    }
    bcopy(g_aPakPfs[0].id, g_aabSavePakIds[0], sizeof(g_aPakPfs[0].id));
    gControllerPakState = osPfsNumFiles(&g_aPakPfs[0], &maxFiles, &g_nPakFilesUsed);
    if (!PakCallSucceeded()) {
        g_wSerialInterfaceIdle = 1;
        return TRUE;
    }

    for (note = 0; note < PAK_NOTES; note++) {
        if (osPfsFileState(&g_aPakPfs[0], note, &g_aPakFileStates[note]) != 0) {
            g_wPakScanEmptyMask |= 1 << note;
        }
    }
    for (note = 0; note < PAK_NOTES; note++) {
        if (g_wPakScanEmptyMask & (1 << note)) {
            continue;
        }
        state = &g_aPakFileStates[note];
        osSyncPrintf("%d: %08x %04x\n", note, state->game_code, state->company_code);
        if (state->game_code != GAME_CODE || state->company_code != COMPANY_CODE) {
            g_wSerialInterfaceIdle = 1;
            return FALSE;
        }
    }
    g_wSerialInterfaceIdle = 1;
    return TRUE;
}
