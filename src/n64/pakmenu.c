#include "pakmenu.h"

#include <PR/os.h>
#include <PR/os_libc.h>

#include "common/fade.h"
#include "common/game.h"
#include "common/heap.h"
#include "common/input.h"
#include "common/render.h"
#include "common/rumble.h"
#include "common/system.h"
#include "common/text.h"

u8 DecalCharToAscii(u8 c) {
    /* The original copies the whole table to the stack to read one character of it. */
    DecalCharMap map = g_szDecalCharMap;

    if (c < (s32)sizeof(map.chars)) {
        return map.chars[c];
    }
    return ' ';
}

void ProcessPakMenuScreen(void) {
#ifdef __sgi
    /* IDO needs function-local state to match; GCC reuses the ROM's fixed BSS slots. */
    static u8 g_bPakDirLoaded;
    static u8 g_bPakSelectedNote;
    static u8 g_bPakFirstValidNote;
    static u8 g_bPakLastValidNote;
    static s32 g_dwPakNoteCount;
    static s32 g_dwPakFreeBlocks;
    static OSPfsState g_aPakDirEntries[PAK_NOTES];
    static u8 g_bPakMenuState;
#endif
    int y;
    int i;
    int k;
    int w;
#ifdef __sgi
    int x;
#endif
#if TGR_REGION != TGR_REGION_US
    int screenBlanked;
#endif
    int ox;
#ifdef __sgi
#if TGR_REGION == TGR_REGION_US
    int framePaddingBeforeButtons[4];
#else
    int framePaddingBeforeButtons[3];
#endif
#endif
    int by;
    int ty;
    int bx;
#ifdef __sgi
    int framePaddingAfterButtons[2];
#endif
    char c;
    OSPfsState *st;
    int maxFiles;
    unsigned char pattern;
    char num[11];
#if TGR_REGION == TGR_REGION_JP
    unsigned char name[128];
    unsigned char *p;
    int kana;
    int was;
    unsigned char ch;
#else
    unsigned char name[28];
#endif
#ifdef __sgi
    int framePaddingAfterName;
#if TGR_REGION == TGR_REGION_JP
    int framePaddingJapanese;
#endif
#endif

#if TGR_REGION != TGR_REGION_US
    screenBlanked = 1;
#if TGR_REGION == TGR_REGION_ASIA && defined(ENABLE_PATCHES)
    (void)screenBlanked;
#endif
#endif
    g_wSerialInterfaceIdle = 0;
    if (g_bPakMenuInitialized == 0) {
        g_bPakMenuInitialized = 1;
        g_bPakDirLoaded = 0;
#if TGR_REGION == TGR_REGION_EU || TGR_REGION == TGR_REGION_JP
        g_dwKeepScreenBlanked = screenBlanked;
        BlankDisplay();
        StartHighResFrame();
        FlushDisplayList();
        g_dwKeepScreenBlanked = 0;
#endif
        osPfsIsPlug(&g_SiMesgQueue, &pattern);
        if ((pattern & 1) == 0) {
            SetCurrentScreenCallback(MainMenuEntry);
            g_bPakMenuInitialized = 0;
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
        g_bPakMenuState = 1;
        SetFadeTarget(1.0f, 0.2f);
#if TGR_REGION == TGR_REGION_EU || TGR_REGION == TGR_REGION_JP
        g_dwKeepScreenBlanked = 0;
#endif
    }
    UpdateFrameTiming();
    UpdateFadeTransitions();
    StartHighResFrame();
    SetupProjectionMatrix(g_dwScreenWidth, g_dwScreenHeight);
    SetViewport(0, 0, g_dwScreenWidth, g_dwScreenHeight, 1);
    ClearDepthBuffer();
    ClearScreen(0, 0, 0);
    DrawSpriteTinted(&g_texBackground, 0x50, 8, 0x80);
    DrawSpriteRegion(
        &g_texLogo, g_texLogo.originX + 0x18, g_texLogo.originY + 2, 0x208, 0x41, 0x80, 0x80, 0x80);
    SetTextAlignCenter();
    SetTextStyleNormal();
    SetTextSize(22);
    DrawText("%ryCONTROLLER PAK MENU", 0xa0, 0x20);
    SetTextAlignLeft();
    SetTextSize(12);
    DrawText("%yw#", 0x48, 0x34);
#if TGR_REGION == TGR_REGION_JP
    SetTextSize(13);
    DrawText("%yw%j\xB9(\xA6\xC4(\xC3", 0x74, 0x34);
    SetTextAlignRight();
    DrawText("%yw%j\xBD(\x9A", 0xfa, 0x34);
    SetTextSize(11);
#else
    DrawText("%ywGAME NOTE", 0x74, 0x34);
    SetTextAlignRight();
    DrawText("%ywPAGES", 0xfa, 0x34);
    SetTextSize(9);
#endif
    y = 0x3f;
    for (i = 0; i < 16; i++) {
        sprintf(num, "%d", i + 1);
        if (g_dwPakNoteCount != 0 && i == g_bPakSelectedNote) {
            SetTextCustomColors(0x80, 0xff, 0x80, 0, 0xff, 0);
        } else {
            SetTextCustomColors(0xff, 0xff, 0xff, 0x80, 0x80, 0x80);
        }
        DrawText(num, 0x52, i * 9 + y);
    }
    if (g_bPakDirLoaded != 0 && g_bPakMenuState != 4) {
        for (i = 0; i < 16; i++) {
            if ((g_wPakEmptySlotMask & (1 << i)) == 0) {
#if TGR_REGION == TGR_REGION_JP
                kana = 0;
                for (k = 0, p = name; k < 16; k++) {
                    ch = g_aPakDirEntries[i].game_name[k];
                    was = kana;
                    if (ch < 0x42) {
                        if (kana) {
                            *p++ = '%';
                            *p++ = 'e';
                            kana = 0;
                        }
                    } else if (!kana) {
                        *p++ = '%';
                        *p++ = 'j';
                        kana = 1;
                    }
                    c = DecalCharToAscii(ch);
                    *p = c;
                    if (c == 0) {
                        break;
                    }
                    p++;
                    if (p[-1] == '.' && was) {
                        *p++ = ' ';
                    }
                    if (p[-1] == '%') {
                        *p++ = '%';
                    }
                }
                c = DecalCharToAscii(g_aPakDirEntries[i].ext_name[0]);
                if (c != 0) {
                    if (kana) {
                        *p++ = '%';
                        *p++ = 'e';
                    }
                    *p++ = ' ';
                    *p++ = '.';
                    if (g_aPakDirEntries[i].ext_name[0] >= 0x42) {
                        *p++ = '%';
                        *p++ = 'j';
                    }
                    *p++ = c;
                    *p = 0;
                } else {
                    *p = 0;
                }
#else
                for (k = 0; k < 16; k++) {
                    name[k] = DecalCharToAscii(g_aPakDirEntries[i].game_name[k]);
                    if (name[k] == 0) {
                        break;
                    }
                }
                c = DecalCharToAscii(g_aPakDirEntries[i].ext_name[0]);
                if (c != 0) {
                    name[k] = ' ';
                    name[k + 1] = '.';
                    name[k + 2] = c;
                    name[k + 3] = 0;
                } else {
                    name[k] = 0;
                }
#endif
                SetTextAlignLeft();
                if (g_dwPakNoteCount != 0 && i == g_bPakSelectedNote) {
                    SetTextCustomColors(0x80, 0xff, 0x80, 0, 0xff, 0);
                } else {
                    SetTextCustomColors(0xff, 0xff, 0xff, 0x80, 0x80, 0x80);
                }
#if TGR_REGION == TGR_REGION_JP
                DrawText((char *)name, 0x5a, i * 9 + y);
                sprintf(num, "%d", g_aPakDirEntries[i].file_size >> 8);
                SetTextAlignRight();
                DrawText(num, 0xf8, i * 9 + y);
#else
                DrawText((char *)name, 0x62, i * 9 + y);
                sprintf(num, "%d", g_aPakDirEntries[i].file_size >> 8);
                SetTextAlignRight();
                DrawText(num, 0xf4, i * 9 + y);
#endif
            }
        }
    }
#if TGR_REGION == TGR_REGION_JP
    SetTextCustomColors(0xff, 0xff, 0xff, 0x80, 0x80, 0x80);
#endif
    switch (g_bPakMenuState) {
    case 0:
        gControllerPakState = osPfsInitPak(&g_SiMesgQueue, &g_aPakPfs[0], 0);
        if (gControllerPakState != 0 && gControllerPakState == 10) {
            if (osMotorInit(&g_SiMesgQueue, &g_aContPfs[0], 0) == 0) {
                gControllerPakState = 0;
                SetCurrentScreenCallback(MainMenuEntry);
                g_wSerialInterfaceIdle = 1;
                return;
            }
            gControllerPakState = osPfsRepairId(&g_aPakPfs[0]);
            if (gControllerPakState == 0) {
                osPfsInitPak(&g_SiMesgQueue, &g_aPakPfs[0], 0);
            }
        }
        if (gControllerPakState == 0) {
            bcopy(g_aPakPfs[0].id, g_aabSavePakIds[0], 0x20);
            g_bPakMenuState = 1;
        } else {
            g_bPakMenuState = 4;
        }
        break;
    case 1:
        gControllerPakState = osPfsNumFiles(&g_aPakPfs[0], &maxFiles, &g_dwPakNoteCount);
        if (gControllerPakState != 0) {
            if (gControllerPakState == 3) {
                if (osPfsChecker(&g_aPakPfs[0]) != 0) {
                    g_bPakMenuState = 4;
                    break;
                }
            } else {
                g_bPakMenuState = 4;
                break;
            }
        }
#if TGR_REGION == TGR_REGION_JP
        g_wPakEmptySlotMask = 0;
#endif
        for (i = 0; i < 16; i++) {
            if (osPfsFileState(&g_aPakPfs[0], i, &g_aPakDirEntries[i]) != 0) {
                g_wPakEmptySlotMask |= 1 << i;
            }
        }
        g_bPakFirstValidNote = 0;
        for (i = 0; i < 16; i++) {
            if ((g_wPakEmptySlotMask & (1 << i)) == 0) {
                g_bPakFirstValidNote = i;
                break;
            }
        }
        g_bPakLastValidNote = 0;
        /* The original reads this loop index, which affects IDO register allocation. */
        if (i != 0) {
        }
        for (i = 0; i < 16; i++) {
            if ((g_wPakEmptySlotMask & (1 << i)) == 0) {
                g_bPakLastValidNote = i;
            }
        }
        gControllerPakState = osPfsFreeBlocks(&g_aPakPfs[0], &g_dwPakFreeBlocks);
        if (gControllerPakState != 0) {
            if (gControllerPakState == 3) {
                if (osPfsChecker(&g_aPakPfs[0]) != 0) {
                    g_bPakMenuState = 4;
                    break;
                }
            } else {
                g_bPakMenuState = 4;
                break;
            }
        }
        g_bPakDirLoaded = 1;
        g_bPakMenuState = 2;
        g_bPakSelectedNote = g_bPakFirstValidNote;
        break;
    case 2:
        UpdateStickRepeat(&g_aPlayerInputState[0]);
        if (g_aPlayerInputState[0].buttons & 8) {
            TransferFlags(&g_aPlayerInputState[0], 8);
            if (g_dwPakNoteCount != 0 && g_bPakSelectedNote != g_bPakFirstValidNote) {
                g_bPakSelectedNote--;
                while ((g_wPakEmptySlotMask & (1 << g_bPakSelectedNote)) &&
                       g_bPakSelectedNote != g_bPakFirstValidNote) {
                    g_bPakSelectedNote--;
                }
            }
        } else if (g_aPlayerInputState[0].buttons & 2) {
            TransferFlags(&g_aPlayerInputState[0], 2);
            if (g_dwPakNoteCount != 0 && g_bPakSelectedNote != g_bPakLastValidNote) {
                g_bPakSelectedNote++;
                while ((g_wPakEmptySlotMask & (1 << g_bPakSelectedNote)) &&
                       g_bPakSelectedNote != g_bPakLastValidNote) {
                    g_bPakSelectedNote++;
                }
            }
        } else if (g_aPlayerInputState[0].buttons & 0x10) {
            TransferFlags(&g_aPlayerInputState[0], 0x10);
            if (g_dwPakNoteCount != 0 && IsFadedToWhite()) {
                g_bPakMenuState = 3;
            }
        } else if ((g_aPlayerInputState[0].buttons & 0x4000) && IsFadedToWhite()) {
            TransferFlags(&g_aPlayerInputState[0], 0x4000);
            g_bPakMenuState = 7;
        }
        break;
    case 3:
#if TGR_REGION == TGR_REGION_JP
        if (g_bPakSelectedNote + 1 < 10) {
            w = 0x15a;
        } else {
            w = 0x166;
        }
#else
        if (g_bPakSelectedNote + 1 < 10) {
            w = 0x128;
        } else {
            w = 0x134;
        }
#endif
#if TGR_REGION == TGR_REGION_JP
        ox = (0x280 - w) >> 1; /* dead: numbers the box's left edge ahead of by */
#endif
        by = 0x110 - g_texButtonA.height;
        DrawRectBorder((0x280 - w) >> 1, 200, w, 0x50, 3, 0, 0, 0x80, 0x80, 0x80);
        SetTextAlignCenter();
        SetTextStyleNormal();
#if TGR_REGION == TGR_REGION_JP
        SetTextSize(14);
        sprintf((char *)name,
                "%%yw%%j\xB9(\xA6\xC4(\xC3w\xE0"
                "B6R1%%%%%%e NO%d",
                g_bPakSelectedNote + 1);
#else
        SetTextSize(12);
        sprintf((char *)name, "%%ywDELETE GAME NOTE #%d?", g_bPakSelectedNote + 1);
#endif
        DrawText((char *)name, 0x9f, 0x73);
        SetTextAlignLeft();
        SetTextSize(10);
        ox = ((0x280 - w) >> 1) + 0x38;
        bx = ((0x280 - w) >> 1) + w - 0x9c;
        ty = (by + 0x12) >> 1;
        DrawText("%wwOK", (ox + g_texButtonA.width + 7U) >> 1, ty);
        DrawText("%wwCANCEL", (bx + g_texButtonB.width + 7U) >> 1, ty);
        DrawTexturedSprite(&g_texButtonA, ox, by);
        DrawTexturedSprite(&g_texButtonB, bx, by);
        if (g_aPlayerInputState[0].buttons & 0x10) {
            TransferFlags(&g_aPlayerInputState[0], 0x10);
            g_bPakMenuState = 5;
        } else if (g_aPlayerInputState[0].buttons & 0x20) {
            TransferFlags(&g_aPlayerInputState[0], 0x20);
            g_bPakMenuState = 2;
        }
        break;
    case 4:
        DrawRectBorder(0xb1, 0xcb, 0x11e, 0x4a, 3, 0, 0, 0x80, 0x80, 0x80);
        SetTextAlignCenter();
        SetTextStyleNormal();
#if TGR_REGION == TGR_REGION_JP
        SetTextSize(14);
        DrawText("%j\xC1\xCF\xC3\xC8(\x88\x8D\x8F\xA1"
                 "D",
                 0x9f,
                 0x75);
        DrawText(" %j\xB0\x88(9\xED\xEE"
                 "B6B3&",
                 0x9f,
                 0x83);
#else
        SetTextSize(12);
        DrawText("%ywCONTROLLER PAK ERROR", 0x9f, 0x75);
        DrawText("%ywHAS BEEN DETECTED", 0x9f, 0x83);
#endif
        if (g_aPlayerInputState[0].buttons & 0x4030) {
            TransferFlags(&g_aPlayerInputState[0], 0x4030);
            g_bPakMenuState = 7;
        }
        g_dwPakFreeBlocks = 0;
        maxFiles = 16;
        g_dwPakNoteCount = 0;
        break;
    case 5:
        st = &g_aPakDirEntries[g_bPakSelectedNote];
        gControllerPakState = osPfsDeleteFile(&g_aPakPfs[0],
                                              st->company_code,
                                              st->game_code,
                                              (u8 *)st->game_name,
                                              (u8 *)st->ext_name);
        if (gControllerPakState == 0) {
            g_wPakEmptySlotMask |= 1 << g_bPakSelectedNote;
            g_aPakDirEntries[g_bPakSelectedNote].game_name[0] = 0;
            g_aPakDirEntries[g_bPakSelectedNote].ext_name[0] = 0;
            g_dwPakFreeBlocks += g_aPakDirEntries[g_bPakSelectedNote].file_size;
            g_aPakDirEntries[g_bPakSelectedNote].file_size = 0;
            if (g_dwPakNoteCount > 0) {
                g_dwPakNoteCount--;
            }
            if (g_bPakSelectedNote == g_bPakLastValidNote) {
                g_bPakLastValidNote--;
                while ((g_wPakEmptySlotMask & (1 << g_bPakLastValidNote)) &&
                       g_bPakLastValidNote != 0) {
                    g_bPakLastValidNote--;
                }
                g_bPakSelectedNote--;
                while ((g_wPakEmptySlotMask & (1 << g_bPakSelectedNote)) &&
                       g_bPakSelectedNote != 0) {
                    g_bPakSelectedNote--;
                }
            } else {
                if (g_bPakSelectedNote == g_bPakFirstValidNote) {
                    g_bPakFirstValidNote++;
                    while ((g_wPakEmptySlotMask & (1 << g_bPakFirstValidNote)) &&
                           g_bPakFirstValidNote != g_bPakLastValidNote) {
                        g_bPakFirstValidNote++;
                    }
                }
                g_bPakSelectedNote++;
                while ((g_wPakEmptySlotMask & (1 << g_bPakSelectedNote)) &&
                       g_bPakSelectedNote != g_bPakLastValidNote) {
                    g_bPakSelectedNote++;
                }
                /* The original reads the boundary again after moving the cursor. */
                if (g_bPakLastValidNote != 0) {
                }
            }
            if (g_bPakLastValidNote != 0) {
            }
            g_bPakMenuState = 2;
        } else if (gControllerPakState == 2) {
            g_bPakMenuState = 6;
        } else {
            g_bPakMenuState = 4;
        }
        break;
    case 6:
        SetTextStyleNormal();
        SetTextAlignLeft();
#if TGR_REGION == TGR_REGION_JP
        DrawRectBorder(0x99, 0xb5, 0x14e, 0x89, 3, 0, 0, 0x80, 0x80, 0x80);
        SetTextSize(14);
        DrawText("%j038B@\xC1\xCF\xC3\xC8(\x88\x8D\x8F\xA1"
                 "9",
                 0x54,
                 0x6a);
        DrawText("%j2Bq6hc@6R&", 0x54, 0x77);
        DrawText("%jqt\xC1\xCF\xC3\xC8(\x88\x8D\x8F\xA1w'", 0x54, 0x8a);
        DrawText("%j\xDE@6R&", 0x54, 0x97);
#else
#if TGR_REGION == TGR_REGION_EU
        DrawRectBorder(0xaf, 0xb5, 0x122, 0x89, 3, 0, 0, 0x80, 0x80, 0x80);
        SetTextSize(12);
#else
        SetTextSize(12);
        DrawRectBorder(0xaf, 0xb5, 0x122, 0x89, 3, 0, 0, 0x80, 0x80, 0x80);
#endif
        DrawText("%ywA NEW CONTROLLER PAK", 0x5f, 0x6a);
        DrawText("%ywWAS INSERTED.", 0x5f, 0x77);
        DrawText("%ywTHIS CONTROLLER PAK", 0x5f, 0x8a);
        DrawText("%ywWILL BE USED.", 0x5f, 0x97);
#endif
        if (g_aPlayerInputState[0].buttons & 0x8030) {
            TransferFlags(&g_aPlayerInputState[0], 0x8030);
            g_bPakDirLoaded = 0;
            gControllerPakState = 0;
            g_bPakMenuState = 0;
        }
        break;
    case 7:
        SetFadeTarget(0.0f, 0.2f);
        g_bPakMenuState = 8;
        break;
    case 8:
        break;
    }
    SetTextAlignLeft();
    SetTextSize(10);
    DrawText("%ywPAGES USED:", 0x47, 0xd3);
    sprintf(num, "%%yw%d", 0x7b - (g_dwPakFreeBlocks >> 8));
    DrawText(num, 0x85, 0xd3);
    DrawText("%ywPAGES FREE:", 0xac, 0xd3);
    sprintf(num, "%%yw%d", g_dwPakFreeBlocks >> 8);
    SetTextAlignRight();
    DrawText(num, 0xfa, 0xd3);
    DrawTexturedSprite(&g_texButtonA, 0xd6, 0x1c8 - g_texButtonA.height);
    DrawTexturedSprite(&g_texButtonBSmall, 0x158, 0x1c8 - g_texButtonA.height);
    SetTextAlignLeft();
    DrawText("%wwSELECT", 0x7b, 0xe1);
    DrawText("%wwEXIT", 0xbc, 0xe1);
    gDPPipeSync(gDisplayListPtr++);
    ApplyScreenFade();
    FlushDisplayList();
    if (IsFadedToBlack()) {
        g_bPakMenuInitialized = 0;
        gControllerPakState = 0;
        StartHighResFrame();
        ClearScreen(0, 0, 0);
        FlushDisplayList();
#if TGR_REGION == TGR_REGION_EU || TGR_REGION == TGR_REGION_JP
        g_dwKeepScreenBlanked = 1;
        BlankDisplay();
        StartHighResFrame();
        ClearScreen(0, 0, 0);
        FlushDisplayList();
        g_dwKeepScreenBlanked = 0;
#else
        osViBlack(1);
        StartHighResFrame();
        ClearScreen(0, 0, 0);
        FlushDisplayList();
#endif
        SetCurrentScreenCallback(MainMenuEntry);
    }
    g_wSerialInterfaceIdle = 1;
}

#if TGR_REGION == TGR_REGION_US
#define UNUSED_ABOVE 6
#define UNUSED_BELOW 10
#elif TGR_REGION == TGR_REGION_ASIA
#define UNUSED_ABOVE 6
#define UNUSED_BELOW 10
#elif TGR_REGION == TGR_REGION_EU
#define UNUSED_ABOVE 1
#define UNUSED_BELOW 9
#else
#define UNUSED_ABOVE 1
#define UNUSED_BELOW 7
#endif /* TGR_REGION == TGR_REGION_US */

s32 InitPakAndEnumerateFiles(void) {
    /* The original reserves stack for locals it does not use, around maxFiles. */
    s32 unusedAbove[UNUSED_ABOVE];
    s32 maxFiles;
    s32 unusedBelow[UNUSED_BELOW];
    s32 note;
    OSPfsState *state;
    /**
     * The notes found empty.
     *
     * @ghidraAddress USA: 0x8028dda0
     * @ghidraAddress Asia: 0x8028D8F0
     * @ghidraAddress EUR: 0x80290540
     * @ghidraAddress JPN: 0x8028ffe4
     */
    static u16 g_wPakScanEmptyMask = 0;

    (void)unusedAbove;
    (void)unusedBelow;
    g_wSerialInterfaceIdle = 0;
    gControllerPakState = osPfsInitPak(&g_SiMesgQueue, &g_aPakPfs[0], 0);
    if (gControllerPakState != 0 && gControllerPakState == PFS_ERR_ID_FATAL) {
        if (osMotorInit(&g_SiMesgQueue, &g_aContPfs[0], 0) == 0) {
            g_wSerialInterfaceIdle = 1;
            return TRUE;
        }
        gControllerPakState = osPfsRepairId(&g_aPakPfs[0]);
        if (gControllerPakState == 0) {
            /* The result is discarded, and the state remains that of the repair. */
            osPfsInitPak(&g_SiMesgQueue, &g_aPakPfs[0], 0);
        }
    }
    if (gControllerPakState != 0) {
        g_wSerialInterfaceIdle = 1;
        return TRUE;
    }
    bcopy(g_aPakPfs[0].id, g_aabSavePakIds[0], sizeof(g_aPakPfs[0].id));
    gControllerPakState = osPfsNumFiles(&g_aPakPfs[0], &maxFiles, &g_nPakFilesUsed);
    if (gControllerPakState != 0) {
        if (gControllerPakState == PFS_ERR_INCONSISTENT) {
            if (osPfsChecker(&g_aPakPfs[0]) != 0) {
                g_wSerialInterfaceIdle = 1;
                return TRUE;
            }
        } else {
            g_wSerialInterfaceIdle = 1;
            return TRUE;
        }
    }

#if TGR_REGION == TGR_REGION_JP
    g_wPakScanEmptyMask = 0;
#endif
    for (note = 0; note < PAK_NOTES; note++) {
        if (osPfsFileState(&g_aPakPfs[0], note, &g_aPakFileStates[note]) != 0) {
            g_wPakScanEmptyMask |= 1 << note;
        }
    }
    for (note = 0; note < PAK_NOTES; note++) {
        if (!(g_wPakScanEmptyMask & (1 << note))) {
            state = &g_aPakFileStates[note];
#if TGR_REGION != TGR_REGION_JP
            osSyncPrintf("%d: %08x %04x\n", note, state->game_code, state->company_code);
#endif
            if (state->game_code != GAME_CODE || state->company_code != COMPANY_CODE) {
                g_wSerialInterfaceIdle = 1;
                return FALSE;
            }
        }
    }
    g_wSerialInterfaceIdle = 1;
    return TRUE;
}
