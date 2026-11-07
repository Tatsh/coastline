#include <PR/os.h>
#include <PR/os_cont.h>
#include <PR/os_libc.h>

#include "common/audio.h"
#include "common/fade.h"
#include "common/game.h"
#include "common/heap.h"
#include "common/input.h"
#include "common/playerinput.h"
#include "common/profile.h"
#include "common/render.h"
#include "common/replay.h"
#include "common/rumble.h"
#include "common/system.h"
#include "common/text.h"
#include "common/trackselect.h"
#include "common/vehicle.h"
#include "n64/mainmenu.h"
#include "n64/options.h"
#include "n64/pakmenu.h"
#include "n64/sprite.h"

#if TGR_REGION != TGR_REGION_US
#if TGR_REGION == TGR_REGION_EU
#define TGR_REGION_EUR 1
#else
#define TGR_REGION_JPN 1
#endif

#define PAD (&g_aPlayerInputState[g_dwMenuChoosingPlayer])
#define PFS (&g_aPakPfs[g_dwMenuChoosingPlayer])
#if TGR_REGION_JPN
#define COMPANY 0x3238  /* "28" */
#define GAME 0x4E54524A /* "NTRJ" */
#elif TGR_REGION_EUR
#define COMPANY 0x3746  /* "7F" */
#define GAME 0x4E545250 /* "NTRP" */
#else
#define COMPANY 0x3544  /* "5D" */
#define GAME 0x4E475245 /* "NGRE" */
#endif

#if TGR_REGION_EUR
#define DBG(args) osSyncPrintf args /* Europe keeps the debug output */
#else
#define DBG(args)
#endif
#if TGR_REGION_JPN || TGR_REGION_EUR
/* @implements 0x80211D70 tgr ProcessDataSaveLoadMenu */
void ProcessDataSaveLoadMenu(void) {
    int i;
    int mode;
    int h;
    int newSize; /* Europe: the packed ghost's size; Japan: a panel's width */
    int unusedCC TGR_MATCHING_UNUSED;
    int unusedC8 TGR_MATCHING_UNUSED;
    int unusedC4 TGR_MATCHING_UNUSED;
    int unusedC0 TGR_MATCHING_UNUSED;
    int y;
    int menuY; /* the menu rows' y; y is the panels' */
    unsigned char *name;
    int freeBytes;
    unsigned char pakBits;
    SaveLoadMenuItem *items;
    int unusedE4 TGR_MATCHING_UNUSED;
    int size;
    int opts[5];
    unsigned char nameSeason[16] = {0x2D,
                                    0x28,
                                    0x29,
                                    0x0F,
                                    0x20,
                                    0x1E,
                                    0x1A,
                                    0x2B,
                                    0x0F,
                                    0x2C,
                                    0x1E,
                                    0x1A,
                                    0x2C,
                                    0x28,
                                    0x27,
                                    0x00};
    unsigned char nameGhost[16] = {0x2D,
                                   0x28,
                                   0x29,
                                   0x0F,
                                   0x20,
                                   0x1E,
                                   0x1A,
                                   0x2B,
                                   0x0F,
                                   0x20,
                                   0x21,
                                   0x28,
                                   0x2C,
                                   0x2D,
                                   0x00,
                                   0x00};
    unsigned char nameOptions[16] = {0x2D,
                                     0x28,
                                     0x29,
                                     0x0F,
                                     0x20,
                                     0x1E,
                                     0x1A,
                                     0x2B,
                                     0x0F,
                                     0x28,
                                     0x29,
                                     0x2D,
                                     0x22,
                                     0x28,
                                     0x27,
                                     0x2C};
    unsigned char ext[4] = {0, 0, 0, 0};
    int u94 TGR_MATCHING_UNUSED; /* unused: the frame has it */
    int k;
    int *p2;
    unsigned char *data;
    int u84[2] TGR_MATCHING_UNUSED; /* unused: the frame has them */
    int len;
    int sum;
    int check;
    static SaveLoadMenuItem loadMenuItems[] = {
        {13, "-- GAME PAK --"},
        {6, "LOAD SEASON DATA"},
        {7, "SAVE SEASON DATA"},
        {8, "LOAD OPTIONS"},
        {9, "SAVE OPTIONS"},
        {14, ""},
        {10, "ERASE DATA"},
        {15, ""},
        {13, "-- CONTROLLER PAK --"},
        {0, "LOAD SEASON DATA"},
        {1, "SAVE SEASON DATA"},
        {2, "LOAD GHOST CAR DATA"},
        {3, "SAVE GHOST CAR DATA"},
        {4, "LOAD OPTIONS"},
        {5, "SAVE OPTIONS"},
        {0, 0},
    };
    static SaveLoadMenuItem saveSeasonMenuItems[] = {
#if TGR_REGION_EUR
        {7, "SAVE SEASON DATA TO GAME PAK"},
        {15, ""},
        {1, "SAVE SEASON DATA TO CONTROLLER PAK"},
        {15, ""},
        {11, "CONTINUE"},
#else
        {7,
         "%j\xB9(\xA6\x8D\x8F\xA1"
         "D\x92(\xAA\xCF\xBB(\x83w\xB2(\xAC"
         "B6R&"},
        {15, ""},
        {1,
         "%j\xC1\xCF\xC3\xC8(\x88\x8D\x8F\xA1"
         "D\x92(\xAA\xCF\xBB(\x83w\xB2(\xAC"
         "B6R&"},
        {15, ""},
        {11, "OK"},
#endif
        {0, 0},
    };
    static SaveLoadMenuItem saveGhostMenuItems[] = {
#if TGR_REGION_EUR
        {3, "SAVE GHOST CAR DATA TO CONTROLLER PAK"},
        {15, ""},
        {11, "DONE"},
#else
        {3,
         "%j\xC1\xCF\xC3\xC8(\x88\x8D\x8F\xA1"
         "D\xC9(\xA2\xC3\x81(\xBB(\x83w\xB2(\xAC"
         "B6R&"},
        {15, ""},
        {11, "OK"},
#endif
        {0, 0},
    };
    static unsigned char menuCursor = 0;  /* the row */
    static unsigned char menuStep = 1;    /* the screen's state */
    static unsigned char dataWasLoaded;   /* data was loaded */
    static unsigned char inputEnabled;    /* the menu is live (not behind a message) */
    static int noteIndex;                 /* the open file */
    static unsigned char skipEepromProbe; /* the EEPROM save was confirmed over good data */
    static unsigned char fadeDelayFrames; /* frames the fade has been done */

#if defined(ENABLE_PATCHES) || !defined(__sgi)
    items = loadMenuItems;
    if (g_bSaveLoadEntryPoint == 1) {
        items = saveSeasonMenuItems;
    } else if (g_bSaveLoadEntryPoint == 2) {
        items = saveGhostMenuItems;
    }
    name = nameOptions;
    size = 0x100;
#endif
    if (g_bSaveLoadMenuInitialized == 0) {
        g_bSaveLoadMenuInitialized = 1;
        dataWasLoaded = 0;
        inputEnabled = 1;
        gControllerPakState = 0;
        fadeDelayFrames = 0;
        ResetHeap();
        g_dwKeepScreenBlanked = 1;
        BlankDisplay();
        StartHighResFrame();
        FlushDisplayList();
        BlankDisplay();
        StartHighResFrame();
        FlushDisplayList();
        LoadTextureFromRom(&g_texBackground);
        LoadTextureFromRom(&g_texLogo);
        LoadTextureFromRom(&g_texButtonA);
        LoadTextureFromRom(&g_texButtonB);
        g_pPakDataBuffer = (unsigned char *)AllocHeapAligned(0x3A00);
        skipEepromProbe = 0;
        menuCursor = 0;
        if (g_bSaveLoadEntryPoint == 0) {
            items = loadMenuItems;
        } else if ((mode = g_bSaveLoadEntryPoint) == 1) {
            items = saveSeasonMenuItems;
        } else if (mode == 2) {
            items = saveGhostMenuItems;
        }
        if (items[0].action == 13) {
            menuCursor = 1;
        }
        menuStep = 0;
        SetFadeTarget(1.0f, 0.2f);
        g_dwKeepScreenBlanked = 0;
    }
    UpdateFrameTiming();
    g_wSerialInterfaceIdle = 0;
    UpdateFadeTransitions();
    StartHighResFrame();
    SetupProjectionMatrix((float)g_dwScreenWidth, (float)g_dwScreenHeight);
    SetViewport(0, 0, g_dwScreenWidth, g_dwScreenHeight, 1);
    ClearDepthBuffer();
    ClearScreen(0, 0, 0);
    DrawSpriteTinted(&g_texBackground, 0x60, 0, 0x80);
    DrawTexturedSprite(&g_texLogo, g_texLogo.originX, g_texLogo.originY + 8);
    switch (items[menuCursor].action) {
    case 0:
    case 1:
        name = nameSeason;
        size = 0x200;
        break;
    case 2:
    case 3:
        name = nameGhost;
        size = 0x3A00;
        break;
    case 4:
    case 5:
        name = nameOptions;
        size = 0x100;
        break;
    }
    SetTextStyleNormal();
    SetTextAlignCenter();
    SetTextSize(30);
    if (g_bSaveLoadEntryPoint == 0) {
        DrawText("%ryDATA LOAD/SAVE", 160, 41);
    } else {
        DrawText("%ryDATA SAVE", 160, 41);
    }
    menuY = 0;
    SetTextSize(13);
    for (i = 0; items[i].text != 0; i++) {
        if (items[i].action == 15) {
            menuY += 6;
        } else if (items[i].action == 14) {
            menuY += 3;
        } else {
            menuY += 12;
        }
    }
    menuY = 0x8B - (menuY >> 1);
    for (i = 0; items[i].text != 0; i++) {
        if (inputEnabled != 0) {
            if (i == menuCursor) {
                SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0);
            } else {
                SetTextCustomColors(0xFF, 0xF5, 0, 200, 0, 0);
            }
        } else if (i == menuCursor) {
            SetTextCustomColors(0x80, 0x80, 0x80, 0x84, 0x7C, 0);
        } else {
            SetTextCustomColors(0x80, 0x78, 0, 100, 0, 0);
        }
        DrawText(items[i].text, 158, menuY);
        if (items[i].action == 15) {
            menuY += 6;
        } else if (items[i].action == 14) {
            menuY += 3;
        } else {
            menuY += 12;
        }
    }
    DrawTexturedSprite(&g_texButtonA, 222, 432);
    DrawTexturedSprite(&g_texButtonB, 342, 432);
    SetTextAlignLeft();
    SetTextSize(11);
    DrawText("%wwSelect", 126, g_dwScreenHeight * 19 / 20 - 3);
    DrawText("%wwExit", 186, g_dwScreenHeight * 19 / 20 - 3);

    switch (menuStep) {
    case 0: /* fading in */
        if (IsFadedToWhite() != 0) {
            menuStep = 1;
        }
        break;
    case 1: /* the menu: the cursor skips headings and gaps */
        inputEnabled = 1;
        UpdateStickRepeat(PAD);
        if (PAD->buttons & 2) {
            TransferFlags(PAD, 2);
            do {
                menuCursor++;
                if (items[menuCursor].text == 0) {
                    menuCursor = 0;
                }
            } while (items[menuCursor].action == 15 || items[menuCursor].action == 14 ||
                     items[menuCursor].action == 13);
        } else if (PAD->buttons & 8) {
            TransferFlags(PAD, 8);
        up:
            if (menuCursor == 0) {
                while (items[menuCursor].text != 0) {
                    menuCursor++;
                }
            }
            menuCursor--;
            if (items[menuCursor].action == 15 || items[menuCursor].action == 14 ||
                items[menuCursor].action == 13) {
                goto up;
            }
        }
        if (PAD->buttons & 0x10) {
            TransferFlags(PAD, 0x10);
            menuStep = 2;
        } else if (PAD->buttons & 0x20) {
            TransferFlags(PAD, 0x20);
            menuStep = 18;
        }
        break;
    case 2:
        FadeSfxTo(0.0f, 0.2f);
        FadeMusicTo(0.0f, 0.2f);
        menuStep = 3;
        break;
    case 3: /* wait for the sound to go */
        if (HasSfxFadeFinished() != 0 && fadeDelayFrames++ == 4) {
            fadeDelayFrames = 0;
            if (items[menuCursor].action < 6) {
                menuStep = 7;
            } else if (items[menuCursor].action < 10) {
                menuStep = 4;
            } else if (items[menuCursor].action == 10) {
                menuStep = 6;
            } else {
                menuStep = 18;
            }
        }
        break;
    case 4: /* the Game Pak's EEPROM: load, or save */
        g_bSaveLoadRumbleSwapPending = 0;
        if (skipEepromProbe == 0 && osEepromProbe(&g_SiMesgQueue) == 0) {
            gControllerPakState = 1;
            menuStep = 15;
            break;
        }
        data = g_pPakDataBuffer + 4;
        p2 = (int *)(data + 0x128);
        len = (data - g_pPakDataBuffer + 0x14f) & ~7;
        if (skipEepromProbe == 0) {
            if (osEepromLongRead(&g_SiMesgQueue, 0, g_pPakDataBuffer, len) != 0 &&
                ((mode = items[menuCursor].action) == 6 || mode == 8)) {
                gControllerPakState = 5;
                menuStep = 15;
                break;
            }
            memcpy(&sum, g_pPakDataBuffer, 4);
            check = 0xBFED812;
            for (k = 4; k < len; k++) {
                check += g_pPakDataBuffer[k];
            }
            if (check != sum) {
#if TGR_REGION_EUR
                osSyncPrintf("checksum fail (%08x vs %08x/%02x%02x%02x%02x) (%d)\n",
                             check,
                             sum,
                             g_pPakDataBuffer[0],
                             g_pPakDataBuffer[1],
                             g_pPakDataBuffer[2],
                             g_pPakDataBuffer[3],
                             len);
#endif
                if ((mode = items[menuCursor].action) == 6 || mode == 8) {
                    gControllerPakState = 5;
                    menuStep = 15;
                    break;
                }
                memcpy(data, g_aVehicles[0].profile, 0x128);
                p2[0] = g_aVehicles[1].profile->handling;
                p2[1] = g_aVehicles[1].profile->transmission;
                p2[2] = g_aVehicles[1].profile->tires;
                p2[3] = g_aVehicles[1].profile->suspension;
                p2[4] = g_aVehicles[1].profile->controllerLayout;
                memcpy(data + 0x13c, &g_OptionSettings, 12);
            } else {
#if TGR_REGION_EUR
                osSyncPrintf("checksum pass\n");
#endif
                if (items[menuCursor].action == 7) {
                    menuStep = 13;
                    break;
                }
            }
        }
        skipEepromProbe = 0;
        switch (items[menuCursor].action) {
        case 6:
            memcpy(g_aVehicles[0].profile, data, 0x128);
            memcpy(g_aVehicles[1].profile, data, 0x128);
            g_aVehicles[1].profile->handling = p2[0];
            g_aVehicles[1].profile->transmission = p2[1];
            g_aVehicles[1].profile->tires = p2[2];
            g_aVehicles[1].profile->suspension = p2[3];
            g_aVehicles[1].profile->controllerLayout = p2[4];
            menuStep = 18;
            dataWasLoaded = 1;
            break;
        case 8:
            g_aVehicles[0].profile->handling = ((PlayerProfile *)data)->handling;
            g_aVehicles[0].profile->transmission = ((PlayerProfile *)data)->transmission;
            g_aVehicles[0].profile->tires = ((PlayerProfile *)data)->tires;
            g_aVehicles[0].profile->suspension = ((PlayerProfile *)data)->suspension;
            g_aVehicles[0].profile->controllerLayout = ((PlayerProfile *)data)->controllerLayout;
            g_aVehicles[1].profile->handling = p2[0];
            g_aVehicles[1].profile->transmission = p2[1];
            g_aVehicles[1].profile->tires = p2[2];
            g_aVehicles[1].profile->suspension = p2[3];
            g_aVehicles[1].profile->controllerLayout = p2[4];
            memcpy(&g_OptionSettings, data + 0x13c, 12);
            ApplyVolumeSettings();
            dataWasLoaded = 1;
            menuStep = 16;
            break;
        case 9:
            ((PlayerProfile *)data)->handling = g_aVehicles[0].profile->handling;
            ((PlayerProfile *)data)->transmission = g_aVehicles[0].profile->transmission;
            ((PlayerProfile *)data)->tires = g_aVehicles[0].profile->tires;
            ((PlayerProfile *)data)->suspension = g_aVehicles[0].profile->suspension;
            ((PlayerProfile *)data)->controllerLayout = g_aVehicles[0].profile->controllerLayout;
            k = 4;
            memcpy(data + 0x13c, &g_OptionSettings, 12);
            goto write;
        case 7:
            k = 4;
            memcpy(data, g_aVehicles[0].profile, 0x128);
        write:
            p2[0] = g_aVehicles[1].profile->handling;
            p2[1] = g_aVehicles[1].profile->transmission;
            p2[2] = g_aVehicles[1].profile->tires;
            p2[3] = g_aVehicles[1].profile->suspension;
            p2[4] = g_aVehicles[1].profile->controllerLayout;
            sum = 0xBFED812;
            for (k = 4; k < len; k++) {
                sum += g_pPakDataBuffer[k];
            }
            memcpy(g_pPakDataBuffer, &sum, 4);
            DBG(("checksum is (%08x/%02x%02x%02x%02x) (%d)\n",
                 sum,
                 g_pPakDataBuffer[0],
                 g_pPakDataBuffer[1],
                 g_pPakDataBuffer[2],
                 g_pPakDataBuffer[3],
                 len));
            if (osEepromLongWrite(&g_SiMesgQueue, 0, g_pPakDataBuffer, len) != 0) {
                gControllerPakState = 5;
                menuStep = 15;
            } else {
                menuStep = 16;
            }
            break;
        }
        break;
    case 5: /* erase: both seasons afresh, written to the EEPROM */
        g_bSaveLoadRumbleSwapPending = 0;
        if (osEepromProbe(&g_SiMesgQueue) == 0) {
            gControllerPakState = 1;
            menuStep = 15;
        } else {
            unsigned char *blk;
            int j;
            int unused6C TGR_MATCHING_UNUSED;
            int n;
            int s;
            int *p2;

            blk = g_pPakDataBuffer + 4;
            p2 = (int *)(blk + 0x128);
            n = (blk - g_pPakDataBuffer + 0x14f) & ~7;
            InitPlayerVehicleSlot(&g_aVehicles[0]);
            InitPlayerVehicleSlot(&g_aVehicles[1]);
            memcpy(blk, g_aVehicles[0].profile, 0x128);
            p2[0] = g_aVehicles[1].profile->handling;
            p2[1] = g_aVehicles[1].profile->transmission;
            p2[2] = g_aVehicles[1].profile->tires;
            p2[3] = g_aVehicles[1].profile->suspension;
            p2[4] = g_aVehicles[1].profile->controllerLayout;
            memcpy(blk + 0x13c, &g_OptionSettings, 12);
            s = 0xBFED812;
            for (j = 4; j < n; j++) {
                s += g_pPakDataBuffer[j];
            }
            memcpy(g_pPakDataBuffer, &s, 4);
#if TGR_REGION_EUR
            osSyncPrintf("checksum is (%08x/%02x%02x%02x%02x) (%d)\n",
                         s,
                         g_pPakDataBuffer[0],
                         g_pPakDataBuffer[1],
                         g_pPakDataBuffer[2],
                         g_pPakDataBuffer[3],
                         n);
#endif
            if (osEepromLongWrite(&g_SiMesgQueue, 0, g_pPakDataBuffer, n) != 0) {
                gControllerPakState = 5;
                menuStep = 15;
            } else {
                menuStep = 16;
            }
        }
        break;
    case 7: /* probe the pak */
        osPfsIsPlug(&g_SiMesgQueue, &pakBits);
        if ((pakBits & (1 << g_dwMenuChoosingPlayer)) == 0) {
            gControllerPakState = 1;
            menuStep = 15;
        } else {
#if TGR_REGION_EUR
            osSyncPrintf("\nInitializing controller pak...\n");
#endif
            gControllerPakState = osPfsInitPak(&g_SiMesgQueue, PFS, g_dwMenuChoosingPlayer);
            if (gControllerPakState != 0) {
                if (gControllerPakState == 10) {
                    if (osMotorInit(&g_SiMesgQueue,
                                    &g_aContPfs[g_dwMenuChoosingPlayer],
                                    g_dwMenuChoosingPlayer) == 0) {
                        osMotorStop(&g_aContPfs[g_dwMenuChoosingPlayer]);
                        gControllerPakState = 9999;
                        g_bSaveLoadRumbleSwapPending = 1;
                        menuStep = 15;
                    } else {
                        gControllerPakState = 0;
                        menuStep = 8;
                    }
                } else {
                    g_abRumblePakPresent[g_dwMenuChoosingPlayer] = 0;
                    menuStep = 15;
                }
            } else if (gControllerPakState == 0) {
                g_abRumblePakPresent[g_dwMenuChoosingPlayer] = 0;
                if (g_abSavePakIdKnown[g_dwMenuChoosingPlayer] == 0) {
                    g_abSavePakIdKnown[g_dwMenuChoosingPlayer] = 1;
                    memcpy(g_aabSavePakIds[g_dwMenuChoosingPlayer],
                           g_aPakPfs[g_dwMenuChoosingPlayer].id,
                           32);
                    menuStep = 10;
                } else if (bcmp(g_aabSavePakIds[g_dwMenuChoosingPlayer],
                                g_aPakPfs[g_dwMenuChoosingPlayer].id,
                                32) != 0) {
                    memcpy(g_aabSavePakIds[g_dwMenuChoosingPlayer],
                           g_aPakPfs[g_dwMenuChoosingPlayer].id,
                           32);
                    menuStep = 9;
                } else {
                    menuStep = 10;
                }
            }
        }
        break;
    case 8:
        gControllerPakState = osPfsRepairId(PFS);
        if (gControllerPakState != 0) {
            menuStep = 15;
        } else {
            menuStep = 7;
        }
        break;
    case 9: /* a different pak */
        inputEnabled = 0;
        SetTextStyleNormal();
        SetTextAlignLeft();
#if TGR_REGION_EUR
        SetTextSize(12);
        SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xF5, 0);
        DrawRectBorder(0xAF, 0xB9, 0x122, 0x89, 3, 0, 0, 0x80, 0x80, 0x80);
        DrawText("A NEW CONTROLLER PAK", 95, 108);
        DrawText("WAS INSERTED.", 95, 121);
        DrawText("THIS CONTROLLER PAK", 95, 140);
        DrawText("WILL BE USED.", 95, 153);
#else
        SetTextSize(14);
        SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xF5, 0);
        DrawRectBorder(0x99, 0xB9, 0x14E, 0x89, 3, 0, 0, 0x80, 0x80, 0x80);
        DrawText("%j038B@\xC1\xCF\xC3\xC8(\x88\x8D\x8F\xA1"
                 "9",
                 84,
                 108);
        DrawText("%j2Bq6hc@6R&", 84, 121);
        DrawText("%jqt\xC1\xCF\xC3\xC8(\x88\x8D\x8F\xA1w'", 84, 140);
        DrawText("%j\xDE@6R&", 84, 153);
#endif
        if (PAD->buttons & 0x8030) {
            TransferFlags(PAD, 0x8030);
            menuStep = 10;
        }
        break;
    case 10: /* find the file */
        gControllerPakState = osPfsFindFile(PFS, COMPANY, GAME, name, ext, &noteIndex);
        if (gControllerPakState == 3) {
            DBG(("File system is corrupted, attempting repair...\n"));
            osPfsChecker(PFS);
            gControllerPakState = osPfsFindFile(PFS, COMPANY, GAME, name, ext, &noteIndex);
        }
        if (gControllerPakState == 0) {
            switch (items[menuCursor].action) {
            case 0:
            case 2:
            case 4:
                menuStep = 11;
                break;
            case 1:
            case 3:
                menuStep = 13;
                break;
            case 5:
                menuStep = 14;
                break;
            default:
                menuStep = 1;
                break;
            }
        } else if (gControllerPakState == 5) {
#if TGR_REGION_EUR || TGR_REGION_JPN
            switch (items[menuCursor].action) {
            case 0:
            case 2:
            case 4:
                menuStep = 15;
                break;
            default:
                menuStep = 12;
                break;
            }
#else
            if (items[menuCursor].action == 0 || items[menuCursor].action == 2 ||
                items[menuCursor].action == 4) {
                menuStep = 15;
            } else {
                menuStep = 12;
            }
#endif
        } else {
            menuStep = 15;
        }
        break;
    case 11: /* load */
        if (gControllerPakState == 0) {
#if TGR_REGION_EUR
            switch (items[menuCursor].action) {
            case 0:
                osSyncPrintf("Loading season data...\n");
                break;
            case 2:
                osSyncPrintf("Loading compressed ghost data...\n");
                break;
            case 4:
                osSyncPrintf("Loading Options...\n");
                break;
            }
#endif
            if (items[menuCursor].action != 4) {
                gControllerPakState =
                    osPfsReadWriteFile(PFS, noteIndex, 0, 0, size, g_pPakDataBuffer);
            } else {
                gControllerPakState =
                    osPfsReadWriteFile(PFS, noteIndex, 0, 0, 0x80, g_pPakDataBuffer);
            }
            if (gControllerPakState != 0) {
                menuStep = 15;
#if TGR_REGION_EUR || TGR_REGION_JPN
            } else {
                switch (items[menuCursor].action) {
                case 0:
#else
            } else if (items[menuCursor].action == 0) {
#endif
                    if (g_bSaveLoadEntryPoint == 0) {
                        opts[0] = g_aVehicles[g_dwMenuChoosingPlayer ^ 1].profile->handling;
                        opts[1] = g_aVehicles[g_dwMenuChoosingPlayer ^ 1].profile->transmission;
                        opts[2] = g_aVehicles[g_dwMenuChoosingPlayer ^ 1].profile->tires;
                        opts[3] = g_aVehicles[g_dwMenuChoosingPlayer ^ 1].profile->suspension;
                        opts[4] = g_aVehicles[g_dwMenuChoosingPlayer ^ 1].profile->controllerLayout;
                    }
                    memcpy(g_aVehicles[0].profile, g_pPakDataBuffer, 0x128);
                    memcpy(g_aVehicles[1].profile, g_pPakDataBuffer, 0x128);
                    if (g_bSaveLoadEntryPoint == 0) {
                        g_aVehicles[g_dwMenuChoosingPlayer ^ 1].profile->handling = opts[0];
                        g_aVehicles[g_dwMenuChoosingPlayer ^ 1].profile->transmission = opts[1];
                        g_aVehicles[g_dwMenuChoosingPlayer ^ 1].profile->tires = opts[2];
                        g_aVehicles[g_dwMenuChoosingPlayer ^ 1].profile->suspension = opts[3];
                        g_aVehicles[g_dwMenuChoosingPlayer ^ 1].profile->controllerLayout = opts[4];
                    }
                    DBG(("Done!\n"));
                    if (g_bSaveLoadRumbleSwapPending == 0) {
                        menuStep = 18;
                    } else {
                        menuStep = 16;
                    }
                    dataWasLoaded = 1;
#if TGR_REGION_EUR || TGR_REGION_JPN
                    break;
                case 2:
#else
            } else if (items[menuCursor].action == 2) {
#endif
                    DBG(("Extracting ghost data...\n"));
                    g_dwGhostRecordingSize =
                        DecompressInterleavedRle(g_abGhostRecording, 0xDE5C, g_pPakDataBuffer, 2);
                    g_dwSelectedTrackId = g_abGhostRecording[0];
#if TGR_REGION_EUR || TGR_REGION_JPN
                    g_aVehicles[0].profile->unlockedTracks |= 1 << g_abGhostRecording[0];
#else
                g_aVehicles[0].profile->unlockedTracks |= 1 << g_dwSelectedTrackId;
#endif
                    g_aVehicles[0].profile->unlocks |= 1 << g_abGhostRecording[1];
                    DBG(("Done!\n"));
                    if (g_bSaveLoadRumbleSwapPending == 0) {
                        menuStep = 18;
                    } else {
                        menuStep = 16;
                    }
                    dataWasLoaded = 1;
#if TGR_REGION_EUR || TGR_REGION_JPN
                    break;
                case 4:
#else
            } else if (items[menuCursor].action == 4) {
#endif
                    memcpy(&g_OptionSettings, g_pPakDataBuffer, 12);
                    gControllerPakState =
                        osPfsReadWriteFile(PFS, noteIndex, 0, 0x80, 0x80, g_pPakDataBuffer);
                    if (gControllerPakState == 0) {
                        memcpy(opts, g_pPakDataBuffer, 20);
                        g_aVehicles[g_dwMenuChoosingPlayer].profile->handling = opts[0];
                        g_aVehicles[g_dwMenuChoosingPlayer].profile->transmission = opts[1];
                        g_aVehicles[g_dwMenuChoosingPlayer].profile->tires = opts[2];
                        g_aVehicles[g_dwMenuChoosingPlayer].profile->suspension = opts[3];
                        g_aVehicles[g_dwMenuChoosingPlayer].profile->controllerLayout = opts[4];
                        DBG(("Done!\n"));
                        ApplyVolumeSettings();
                        menuStep = 16;
                    } else {
                        menuStep = 15;
                    }
#if TGR_REGION_EUR || TGR_REGION_JPN
                    break;
#endif
                }
            }
#if TGR_REGION_EUR || TGR_REGION_JPN
        }
#else
    }
    else {
        menuStep = 15;
    }
#endif
        break;
    case 12: /* make room for a new file */
        gControllerPakState = osPfsFreeBlocks(PFS, &freeBytes);
        if (gControllerPakState == 0) {
            if (freeBytes < size) {
                gControllerPakState = 7;
                menuStep = 15;
            } else {
#if TGR_REGION_EUR
                switch (items[menuCursor].action) {
                case 1:
                    osSyncPrintf("Allocating %d bytes for season data...\n", size);
                    break;
                case 3:
                    osSyncPrintf("Allocating %d bytes for ghost data...\n", size);
                    break;
                case 5:
                    osSyncPrintf("Allocating %d bytes for options...\n", size);
                    break;
                }
#endif
                gControllerPakState =
                    osPfsAllocateFile(PFS, COMPANY, GAME, name, ext, size, &noteIndex);
                if (gControllerPakState == 3) {
                    osPfsChecker(PFS);
                    gControllerPakState =
                        osPfsAllocateFile(PFS, COMPANY, GAME, name, ext, size, &noteIndex);
                }
                if (gControllerPakState == 0) {
                    menuStep = 14;
                } else {
                    menuStep = 15;
                }
            }
        } else {
            menuStep = 15;
        }
        break;
    case 6: /* erase both seasons and the options? */
        inputEnabled = 0;
        y = 0x139 - g_texButtonA.width;
#if TGR_REGION_EUR
        DrawRectBorder(0x73, 0xBB, 0x19A, 0x86, 3, 0, 0, 0x80, 0x80, 0x80);
        SetTextAlignLeft();
        SetTextStyleNormal();
        SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xF5, 0);
        SetTextSize(13);
        DrawText("ABOUT TO ERASE SEASON DATA AND", 65, 108);
        DrawText("OPTION DATA STORED ON GAME PAK.", 65, 122);
        DrawText("DO YOU REALLY WANT TO DO THIS?", 65, 136);
        SetTextSize(10);
        DrawText("%wwOK", (g_texButtonA.width + 0x8BU) >> 1, (y + 18) >> 1);
        DrawText("%wwCANCEL", (g_texButtonB.height + 0x19DU) >> 1, (y + 18) >> 1);
        DrawTexturedSprite(&g_texButtonA, 0x85, y);
        DrawTexturedSprite(&g_texButtonB, 0x197, y);
#else
    DrawRectBorder(0x5F, 0xBB, 0x1C2, 0x86, 3, 0, 0, 0x80, 0x80, 0x80);
    SetTextAlignLeft();
    SetTextStyleNormal();
    SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xF5, 0);
    SetTextSize(14);
    DrawText("SEASON DATA %js%e OPTION DATA %j9", 55, 115);
    DrawText("%jRlc\xE0`6R9'u\x7FsPD\xE0"
             "B6R1%%",
             55,
             129);
    SetTextSize(10);
    DrawText("%wwOK", (g_texButtonA.width + 0x77U) >> 1, (y + 18) >> 1);
    DrawText("%wwCANCEL", (g_texButtonB.height + 0x1B1U) >> 1, (y + 18) >> 1);
    DrawTexturedSprite(&g_texButtonA, 0x71, y);
    DrawTexturedSprite(&g_texButtonB, 0x1AB, y);
#endif
        if (PAD->buttons & 0x10) {
            TransferFlags(PAD, 0x10);
            menuStep = 5;
        } else if (PAD->buttons & 0x20) {
            TransferFlags(PAD, 0x20);
            FadeSfxTo(1.0f, 0.2f);
            FadeMusicTo(1.0f, 0.2f);
            menuStep = 1;
        }
        break;
    case 13: /* overwrite? */
        inputEnabled = 0;
        h = 0xBF;
        if (g_bSaveLoadEntryPoint == 0) {
            h = 0xC9;
        }
        y = h - g_texButtonA.width + 0x62;
        DrawRectBorder(0xD5, h, 0xD6, 0x6A, 3, 0, 0, 0x80, 0x80, 0x80);
        SetTextAlignLeft();
        SetTextStyleNormal();
        SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xF5, 0);
#if TGR_REGION_EUR
        SetTextSize(13);
        DrawText("OK TO OVERWRITE", 114, (h + 30) >> 1);
        DrawText("SAVED DATA?", 114, ((h + 30) >> 1) + 14);
#else
    SetTextSize(14);
    DrawText("%j\xDA\xE4"
             "AB6R1%%",
             114,
             ((h + 30) >> 1) + 7);
#endif
        SetTextSize(10);
        DrawText("%wwOK", (g_texButtonA.width + 0xEDU) >> 1, (y + 18) >> 1);
        DrawText("%wwCANCEL", (g_texButtonB.height + 0x13BU) >> 1, (y + 18) >> 1);
        DrawTexturedSprite(&g_texButtonA, 0xE7, y);
        DrawTexturedSprite(&g_texButtonB, 0x135, y);
        if (PAD->buttons & 0x10) {
            TransferFlags(PAD, 0x10);
            if (items[menuCursor].action == 7) {
                menuStep = 4;
                skipEepromProbe = 1;
            } else {
                menuStep = 14;
            }
        } else if (PAD->buttons & 0x20) {
            TransferFlags(PAD, 0x20);
            FadeSfxTo(1.0f, 0.2f);
            FadeMusicTo(1.0f, 0.2f);
            menuStep = 1;
        }
        break;
    case 14: /* save */
#if TGR_REGION_EUR
        switch (items[menuCursor].action) {
        case 1:
            osSyncPrintf("Saving season data...\n");
            memcpy(g_pPakDataBuffer, g_aVehicles[0].profile, 0x128);
            gControllerPakState = osPfsReadWriteFile(PFS, noteIndex, 1, 0, 0x200, g_pPakDataBuffer);
            if (gControllerPakState == 0) {
                osSyncPrintf("Done!\n");
                menuStep = 16;
            } else {
                menuStep = 15;
            }
            break;
        case 3:
            osSyncPrintf("Compressing ghost data...\n");
            newSize = CompressRleChannels(
                g_pPakDataBuffer, 0x3A00, g_abGhostRecording, g_dwGhostRecordingSize, 2);
            osSyncPrintf("new size = %d\n", newSize);
            if (newSize != -1) {
                osSyncPrintf("Saving ghost data to controller pak...\n");
                gControllerPakState =
                    osPfsReadWriteFile(PFS, noteIndex, 1, 0, 0x3A00, g_pPakDataBuffer);
                if (gControllerPakState == 0) {
                    osSyncPrintf("Done!\n");
                    menuStep = 16;
                } else {
                    menuStep = 15;
                }
            } else {
                gControllerPakState = -99999;
                menuStep = 15;
            }
            break;
        case 5:
            osSyncPrintf("Saving options...\n");
            memcpy(g_pPakDataBuffer, &g_OptionSettings, 12);
            gControllerPakState = osPfsReadWriteFile(PFS, noteIndex, 1, 0, 0x80, g_pPakDataBuffer);
            if (gControllerPakState != 0) {
                menuStep = 15;
            } else {
                opts[0] = g_aVehicles[g_dwMenuChoosingPlayer].profile->handling;
                opts[1] = g_aVehicles[g_dwMenuChoosingPlayer].profile->transmission;
                opts[2] = g_aVehicles[g_dwMenuChoosingPlayer].profile->tires;
                opts[3] = g_aVehicles[g_dwMenuChoosingPlayer].profile->suspension;
                opts[4] = g_aVehicles[g_dwMenuChoosingPlayer].profile->controllerLayout;
                memcpy(g_pPakDataBuffer, opts, 20);
                gControllerPakState =
                    osPfsReadWriteFile(PFS, noteIndex, 1, 0x80, 0x80, g_pPakDataBuffer);
                if (gControllerPakState == 0) {
                    osSyncPrintf("Done!\n");
                    menuStep = 16;
                } else {
                    menuStep = 15;
                }
            }
            break;
        }
#elif TGR_REGION_JPN
    switch (items[menuCursor].action) {
    case 1:
        memcpy(g_pPakDataBuffer, g_aVehicles[0].profile, 0x128);
        gControllerPakState = osPfsReadWriteFile(PFS, noteIndex, 1, 0, 0x200, g_pPakDataBuffer);
        if (gControllerPakState == 0) {
            menuStep = 16;
        } else {
            menuStep = 15;
        }
        break;
    case 3:
        if (CompressRleChannels(
                g_pPakDataBuffer, 0x3A00, g_abGhostRecording, g_dwGhostRecordingSize, 2) != -1) {
            gControllerPakState =
                osPfsReadWriteFile(PFS, noteIndex, 1, 0, 0x3A00, g_pPakDataBuffer);
            if (gControllerPakState == 0) {
                menuStep = 16;
            } else {
                menuStep = 15;
            }
        } else {
            gControllerPakState = -99999;
            menuStep = 15;
        }
        break;
    case 5:
        memcpy(g_pPakDataBuffer, &g_OptionSettings, 12);
        gControllerPakState = osPfsReadWriteFile(PFS, noteIndex, 1, 0, 0x80, g_pPakDataBuffer);
        if (gControllerPakState != 0) {
            menuStep = 15;
        } else {
            opts[0] = g_aVehicles[g_dwMenuChoosingPlayer].profile->handling;
            opts[1] = g_aVehicles[g_dwMenuChoosingPlayer].profile->transmission;
            opts[2] = g_aVehicles[g_dwMenuChoosingPlayer].profile->tires;
            opts[3] = g_aVehicles[g_dwMenuChoosingPlayer].profile->suspension;
            opts[4] = g_aVehicles[g_dwMenuChoosingPlayer].profile->controllerLayout;
            memcpy(g_pPakDataBuffer, opts, 20);
            gControllerPakState =
                osPfsReadWriteFile(PFS, noteIndex, 1, 0x80, 0x80, g_pPakDataBuffer);
            if (gControllerPakState == 0) {
                menuStep = 16;
            } else {
                menuStep = 15;
            }
        }
        break;
    }
#else
        if (items[menuCursor].action == 1) {
            memcpy(g_pPakDataBuffer, g_aVehicles[0].profile, 0x128);
            gControllerPakState = osPfsReadWriteFile(PFS, noteIndex, 1, 0, 0x200, g_pPakDataBuffer);
            if (gControllerPakState == 0) {
                menuStep = 16;
            } else {
                menuStep = 15;
            }
        } else if (items[menuCursor].action == 3) {
            if (CompressRleChannels(
                    g_pPakDataBuffer, 0x3A00, g_abGhostRecording, g_dwGhostRecordingSize, 2) !=
                -1) {
                gControllerPakState =
                    osPfsReadWriteFile(PFS, noteIndex, 1, 0, 0x3A00, g_pPakDataBuffer);
                if (gControllerPakState == 0) {
                    menuStep = 16;
                } else {
                    menuStep = 15;
                }
            } else {
                gControllerPakState = -99999;
                menuStep = 15;
            }
        } else if (items[menuCursor].action == 5) {
            memcpy(g_pPakDataBuffer, &g_OptionSettings, 12);
            gControllerPakState = osPfsReadWriteFile(PFS, noteIndex, 1, 0, 0x80, g_pPakDataBuffer);
            if (gControllerPakState != 0) {
                menuStep = 15;
            } else {
                opts[0] = g_aVehicles[g_dwMenuChoosingPlayer].profile->handling;
                opts[1] = g_aVehicles[g_dwMenuChoosingPlayer].profile->transmission;
                opts[2] = g_aVehicles[g_dwMenuChoosingPlayer].profile->tires;
                opts[3] = g_aVehicles[g_dwMenuChoosingPlayer].profile->suspension;
                opts[4] = g_aVehicles[g_dwMenuChoosingPlayer].profile->controllerLayout;
                memcpy(g_pPakDataBuffer, opts, 20);
                gControllerPakState =
                    osPfsReadWriteFile(PFS, noteIndex, 1, 0x80, 0x80, g_pPakDataBuffer);
                if (gControllerPakState == 0) {
                    menuStep = 16;
                } else {
                    menuStep = 15;
                }
            }
        }
#endif
        break;
    case 15: /* an error, or the Rumble Pak is in the way */
        inputEnabled = 0;
        SetTextStyleNormal();
        SetTextAlignLeft();
#if TGR_REGION_EUR
        SetTextSize(12);
        SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xF5, 0);
        switch (gControllerPakState) {
        case 10:
            DrawRectBorder(0xBD, 0xD9, 0x106, 0x4A, 3, 0, 0, 0x80, 0x80, 0x80);
            DrawText("THE CONTROLLER PAK", 102, 124);
            DrawText("IS NONFUNCTIONAL.", 102, 138);
            break;
        case 1:
        case 11:
            DrawRectBorder(0x50, 0xD8, 0x1E0, 0x4C, 3, 0, 0, 0x80, 0x80, 0x80);
            switch (items[menuCursor].action) {
            case 6:
            case 7:
            case 8:
            case 9:
                DrawText("CANNOT ACCESS GAME PAK SAVE AREA.", 48, 124);
                break;
            default:
                DrawText("CONTROLLER PAK NOT FOUND.", 48, 124);
                break;
            }
            switch (items[menuCursor].action) {
            case 0:
            case 2:
            case 4:
            case 6:
            case 8:
                DrawText("DATA CANNOT BE LOADED.", 48, 138);
                break;
            default:
                DrawText("DATA CANNOT BE SAVED.", 48, 138);
                break;
            }
            break;
        case 9999:
            y = 0x146 - g_texButtonA.width;
            DrawRectBorder(0x79, 0xAE, 0x18D, 0xA0, 3, 0, 0, 0x80, 0x80, 0x80);
            DrawText("PLEASE REMOVE THE RUMBLE PAK", 68, 103);
            DrawText("AND INSERT THE CONTROLLER PAK", 68, 116);
            DrawText("INTO THE CONTROLLER.  PRESS", 68, 129);
            DrawText("THE A BUTTON WHEN READY.", 68, 142);
            SetTextAlignLeft();
            SetTextSize(10);
            DrawText("%wwOK", (g_texButtonA.width + 0xE3U) >> 1, (y + 18) >> 1);
            DrawText("%wwCANCEL", (g_texButtonB.width + 0x144U) >> 1, (y + 18) >> 1);
            DrawTexturedSprite(&g_texButtonA, 0xDD, y);
            DrawTexturedSprite(&g_texButtonB, 0x13E, y);
            break;
        case 7:
        case 8:
            DrawRectBorder(0xA5, 0x9F, 0x136, 0xBD, 3, 0, 0, 0x80, 0x80, 0x80);
            switch (items[menuCursor].action) {
            case 1:
                DrawText("INSUFFICIENT FREE PAGES", 90, 95);
                DrawText("OR FREE NOTES IN THE", 90, 108);
                DrawText("CONTROLLER PAK.", 90, 121);
                DrawText("TWO PAGES AND ONE NOTE", 90, 140);
                DrawText("ARE NEEDED TO SAVE THE", 90, 153);
                DrawText("SEASON DATA.", 90, 166);
                break;
            case 3:
                DrawText("INSUFFICIENT FREE PAGES", 90, 95);
                DrawText("OR FREE NOTES IN THE", 90, 108);
                DrawText("CONTROLLER PAK.", 90, 121);
                DrawText("58 PAGES AND ONE NOTE", 90, 140);
                DrawText("ARE NEEDED TO SAVE THE", 90, 153);
                DrawText("GHOST CAR DATA.", 90, 166);
                break;
            case 5:
                DrawText("INSUFFICIENT FREE PAGES", 90, 95);
                DrawText("OR FREE NOTES IN THE", 90, 108);
                DrawText("CONTROLLER PAK.", 90, 121);
                DrawText("ONE PAGE AND ONE NOTE", 90, 140);
                DrawText("ARE NEEDED TO SAVE THE", 90, 153);
                DrawText("OPTIONS.", 90, 166);
                break;
            }
            break;
        case -99999:
            DrawRectBorder(0x75, 0xBD, 0x196, 0x82, 3, 0, 0, 0x80, 0x80, 0x80);
            DrawText("UNABLE TO SAVE GHOST CAR DATA.", 66, 110);
            DrawText("GHOST CAR DATA IS LARGER THAN", 66, 124);
            DrawText("THE MAXIMUM ALLOCATED SPACE", 66, 138);
            DrawText("IN THE CONTROLLER PAK.", 66, 152);
            break;
        case 4:
            DrawRectBorder(0xBF, 0xD9, 0x102, 0x4A, 3, 0, 0, 0x80, 0x80, 0x80);
            DrawText("CONTROLLER ERROR", 103, 124);
            DrawText("HAS BEEN DETECTED!", 103, 138);
            break;
        case 5:
            switch (items[menuCursor].action) {
            case 6:
            case 8:
                DrawRectBorder(0xAD, 0xD9, 0x125, 0x4A, 3, 0, 0, 0x80, 0x80, 0x80);
                DrawText("SAVED DATA NOT FOUND", 94, 124);
                DrawText("IN GAME PAK.", 94, 138);
                break;
            case 7:
            case 9:
                DrawRectBorder(0xAD, 0xD9, 0x125, 0x4A, 3, 0, 0, 0x80, 0x80, 0x80);
                DrawText("ERROR WRITING TO THE", 94, 124);
                DrawText("GAME PAK SAVE AREA.", 94, 138);
                break;
            case 0:
            case 2:
            case 4:
                DrawRectBorder(0xAD, 0xD9, 0x125, 0x4A, 3, 0, 0, 0x80, 0x80, 0x80);
                DrawText("SAVED DATA NOT FOUND", 94, 124);
                DrawText("IN CONTROLLER PAK.", 94, 138);
                break;
            case 1:
            case 3:
            case 5:
                DrawRectBorder(0xB6, 0xD9, 0x114, 0x4A, 3, 0, 0, 0x80, 0x80, 0x80);
                DrawText("UNABLE TO SAVE DATA", 99, 124);
                DrawText("TO CONTROLLER PAK.", 99, 138);
                break;
            }
            break;
        case 2:
            DrawRectBorder(0xAF, 0xBB, 0x122, 0x89, 3, 0, 0, 0x80, 0x80, 0x80);
            DrawText("A NEW CONTROLLER PAK", 95, 109);
            DrawText("WAS INSERTED.", 95, 122);
            DrawText("THIS CONTROLLER PAK", 95, 141);
            DrawText("WILL BE USED.", 95, 154);
            break;
        default:
            DrawRectBorder(0xB1, 0xD9, 0x11E, 0x4A, 3, 0, 0, 0x80, 0x80, 0x80);
            DrawText("CONTROLLER PAK ERROR", 96, 124);
            DrawText("HAS BEEN DETECTED!", 96, 138);
            break;
        }
#else
    SetTextSize(14);
    SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xF5, 0);
    switch (gControllerPakState) {
    case 10:
        DrawRectBorder(0x50, 0xD9, 0x1E0, 0x4A, 3, 0, 0, 0x80, 0x80, 0x80);
        DrawText("%j\xC1\xCF\xC3\xC8(\x88\x8D\x8F\xA1"
                 "9'qg"
                 "hc@6R&",
                 48,
                 124);
        DrawText("%j038B@\xC1\xCF\xC3\xC8(\x88\x8D\x8F\xA1w'2Bc\xDB"
                 "2@&",
                 48,
                 138);
        break;
    case 1:
    case 11:
        switch (items[menuCursor].action) {
        case 6:
        case 7:
        case 8:
        case 9:
            newSize = 0x172;
            break;
        default:
            newSize = 0x1E0;
            break;
        }
        y = (0x280 - newSize) >> 1;
        DrawRectBorder(y, 0xD8, newSize, 0x4C, 3, 0, 0, 0x80, 0x80, 0x80);
        switch (items[menuCursor].action) {
        case 6:
        case 7:
        case 8:
        case 9:
            DrawText("%j\xB9(\xA6\x8D\x8F\xA1"
                     "e\x80\xA1\xB2\xA2kA6b\x7F&",
                     (y + 16) >> 1,
                     124);
            break;
        default:
            DrawText("%j\xC1\xCF\xC3\xC8(\x88\x8D\x8F\xA1"
                     "90H6b\x7F&",
                     (y + 16) >> 1,
                     124);
            break;
        }
        switch (items[menuCursor].action) {
        case 0:
        case 2:
        case 4:
        case 6:
        case 8:
            DrawText("%j\xBB(\x83w\xC8(\xCBkA6b\x7F&", (y + 16) >> 1, 138);
            break;
        default:
            DrawText("%j\xBB(\x83w\xB2(\xACkA6b\x7F&", (y + 16) >> 1, 138);
            break;
        }
        break;
    case 9999:
        y = 0x139 - g_texButtonA.width;
        DrawRectBorder(0xC8, 0xBB, 0xF0, 0x86, 3, 0, 0, 0x80, 0x80, 0x80);
        DrawText("%j\xD2\xD3\x8D\x8F\xA1s", 108, 109);
        DrawText("%j\xC1\xCF\xC3\xC8(\x88\x8D\x8F\xA1w", 108, 122);
        DrawText("%j\xD4\xD5"
                 "Bc\xDB"
                 "2@&",
                 108,
                 135);
        SetTextAlignLeft();
        SetTextSize(10);
        DrawText("%wwOK", (g_texButtonA.width + 0xDEU) >> 1, (y + 18) >> 1);
        DrawText("%wwCANCEL", (g_texButtonB.width + 0x14AU) >> 1, (y + 18) >> 1);
        DrawTexturedSprite(&g_texButtonA, 0xD8, y);
        DrawTexturedSprite(&g_texButtonB, 0x144, y);
        break;
    case 7:
    case 8:
        DrawRectBorder(0x82, 0xB7, 0x17C, 0x8D, 3, 0, 0, 0x80, 0x80, 0x80);
        switch (items[menuCursor].action) {
        case 1:
            DrawText("%j\xC1\xCF\xC3\xC8(\x88\x8D\x8F\xA1"
                     "D",
                     73,
                     107);
            DrawText("%j\xE1"
                     "A\xBD(\x9A"
                     "90H6b\x7F&",
                     73,
                     120);
            DrawText("%j\x92(\xAA\xCF\xBB(\x83w\xB2(\xACR"
                     "XD5",
                     73,
                     139);
            DrawText("2%j\xBD(\x9A\xD6\xD7kR&", 73, 152);
            break;
        case 3:
            DrawText("%j\xC1\xCF\xC3\xC8(\x88\x8D\x8F\xA1"
                     "D",
                     73,
                     107);
            DrawText("%j\xE1"
                     "A\xBD(\x9A"
                     "90H6b\x7F&",
                     73,
                     120);
            DrawText("%j\xC9(\xA2\xC3\x81(\xBB(\x83w\xB2(\xACR"
                     "XD5",
                     73,
                     139);
            DrawText("58%j\xBD(\x9A\xD6\xD7kR&", 73, 152);
            break;
        case 5:
            DrawText("%j\xC1\xCF\xC3\xC8(\x88\x8D\x8F\xA1"
                     "D",
                     73,
                     107);
            DrawText("%j\xE1"
                     "A\xBD(\x9A"
                     "90H6b\x7F&",
                     73,
                     120);
            DrawText("%j\xC0\xAD\x92\xBF\xCF\xBB(\x83w\xB2(\xACR"
                     "XD5",
                     73,
                     139);
            DrawText("1%j\xBD(\x9A\xD6\xD7kR&", 73, 152);
            break;
        }
        break;
    case -99999:
        DrawRectBorder(0xB9, 0xBD, 0x10E, 0x82, 3, 0, 0, 0x80, 0x80, 0x80);
        DrawText("%j\xC9(\xA2\xC3\x81(\xBB(\x83w", 100, 110);
        DrawText("%j\xB2(\xACkA6b\x7F&", 100, 124);
        DrawText("%j\xC1\xCF\xC3\xC8(\x88\x8D\x8F\xA1t", 100, 138);
        DrawText("%j\xE1"
                 "A\xBD(\x9A"
                 "9\xE2H6b\x7F&",
                 100,
                 152);
        break;
    case 4:
        DrawRectBorder(0xBF, 0xD9, 0x102, 0x4A, 3, 0, 0, 0x80, 0x80, 0x80);
        DrawText("%j\xC1\xCF\xC3\xC8(\x88"
                 "D\xB0\x88(9",
                 103,
                 124);
        DrawText("%j\xED\xEE"
                 "B6B3&",
                 103,
                 138);
        break;
    case 5:
        switch (items[menuCursor].action) {
        case 6:
        case 8:
            DrawRectBorder(0xAA, 0xD9, 0x12C, 0x4A, 3, 0, 0, 0x80, 0x80, 0x80);
            DrawText("%j\xB9(\xA6\x8D\x8F\xA1"
                     "D",
                     93,
                     124);
            DrawText("%j\xB2(\xAC\xBB(\x83"
                     "90H6b\x7F&",
                     93,
                     138);
            break;
        case 7:
        case 9:
            DrawRectBorder(0xCD, 0xD9, 0xE6, 0x4A, 3, 0, 0, 0x80, 0x80, 0x80);
            DrawText("%j\xB9(\xA6\x8D\x8F\xA1"
                     "D",
                     110,
                     124);
            DrawText("%j\xE4"
                     "A\xE5"
                     "FkA6b\x7F&",
                     110,
                     138);
            break;
        case 0:
        case 2:
        case 4:
            DrawRectBorder(0xB6, 0xD9, 0x113, 0x4A, 3, 0, 0, 0x80, 0x80, 0x80);
            DrawText("%j\xC1\xCF\xC3\xC8(\x88\x8D\x8F\xA1"
                     "D",
                     99,
                     124);
            DrawText("%j\xB2(\xAC\xBB(\x83"
                     "90H6b\x7F&",
                     99,
                     138);
            break;
        case 1:
        case 3:
        case 5:
            DrawRectBorder(0xB6, 0xD9, 0x114, 0x4A, 3, 0, 0, 0x80, 0x80, 0x80);
            DrawText("%j\xC1\xCF\xC3\xC8(\x88\x8D\x8F\xA1"
                     "D",
                     99,
                     124);
            DrawText("%j\xB2(\xACkA6b\x7F&", 99, 138);
            break;
        }
        break;
    case 2:
        DrawRectBorder(0x99, 0xBB, 0x14E, 0x89, 3, 0, 0, 0x80, 0x80, 0x80);
        DrawText("%j038B@\xC1\xCF\xC3\xC8(\x88\x8D\x8F\xA1"
                 "9",
                 84,
                 109);
        DrawText("%j2Bq6hc@6R&", 84, 122);
        DrawText("%jqt\xC1\xCF\xC3\xC8(\x88\x8D\x8F\xA1w'", 84, 141);
        DrawText("%j\xDE@6R&", 84, 154);
        break;
    default:
        DrawRectBorder(0xB1, 0xD9, 0x11E, 0x4A, 3, 0, 0, 0x80, 0x80, 0x80);
        DrawText("%j\xC1\xCF\xC3\xC8(\x88\x8D\x8F\xA1", 96, 124);
        DrawText("%j9\xD0\xD1kR&", 96, 138);
        break;
    }
#endif
        if (gControllerPakState != 9999) {
            if (PAD->buttons & 0x8030) {
                TransferFlags(PAD, 0x8030);
                gControllerPakState = 0;
                if (gControllerPakState == 2) {
                    menuStep = 7;
                } else {
                    menuStep = 1;
                }
            }
        } else if (PAD->buttons & 0x10) {
            TransferFlags(PAD, 0x10);
            gControllerPakState = 0;
            menuStep = 7;
        } else if (PAD->buttons & 0x20) {
            TransferFlags(PAD, 0x20);
            gControllerPakState = 0;
            FadeSfxTo(1.0f, 0.2f);
            FadeMusicTo(1.0f, 0.2f);
            menuStep = 1;
        }
        break;
    case 16: /* done */
        inputEnabled = 0;
        SetTextAlignCenter();
        SetTextStyleNormal();
#if TGR_REGION_EUR
        SetTextSize(12);
#else
    SetTextSize(14);
#endif
        switch (items[menuCursor].action) {
        case 0:
        case 6:
            if (g_bSaveLoadEntryPoint == 0) {
                h = 0xD9;
            } else {
                h = 0xCE;
            }
            DrawRectBorder(0xE2, h, 0xBC, 0x4A, 3, 0, 0, 0x80, 0x80, 0x80);
#if TGR_REGION_EUR
            DrawText("%ywSEASON DATA", 159, (h + 32) >> 1);
            DrawText("%ywLOADED OK!", 159, ((h + 32) >> 1) + 14);
#else
        DrawText("%yw%j\xC8(\xCB%e OK!", 159, ((h + 32) >> 1) + 7);
#endif
            break;
        case 2:
            if (g_bSaveLoadEntryPoint == 0) {
                h = 0xD9;
            } else {
                h = 0xCE;
            }
            DrawRectBorder(0xE2, h, 0xBC, 0x4A, 3, 0, 0, 0x80, 0x80, 0x80);
#if TGR_REGION_EUR
            DrawText("%ywGHOST DATA", 159, (h + 32) >> 1);
            DrawText("%ywLOADED OK!", 159, ((h + 32) >> 1) + 14);
#else
        DrawText("%yw%j\xC8(\xCB%e OK!", 159, ((h + 32) >> 1) + 7);
#endif
            break;
        case 1:
        case 7:
            if (g_bSaveLoadEntryPoint == 0) {
                h = 0xD9;
            } else {
                h = 0xCE;
            }
            DrawRectBorder(0xE2, h, 0xBC, 0x4A, 3, 0, 0, 0x80, 0x80, 0x80);
#if TGR_REGION_EUR
            DrawText("%ywSEASON DATA", 159, (h + 32) >> 1);
            DrawText("%ywSAVED OK!", 159, ((h + 32) >> 1) + 14);
#else
        DrawText("%yw%j\xB2(\xAC%e OK!", 159, ((h + 32) >> 1) + 7);
#endif
            break;
        case 3:
            if (g_bSaveLoadEntryPoint == 0) {
                h = 0xD9;
            } else {
                h = 0xCE;
            }
            DrawRectBorder(0xD8, h, 0xD0, 0x4A, 3, 0, 0, 0x80, 0x80, 0x80);
#if TGR_REGION_EUR
            DrawText("%ywGHOST CAR DATA", 159, (h + 32) >> 1);
            DrawText("%ywSAVED OK!", 159, ((h + 32) >> 1) + 14);
#else
        DrawText("%yw%j\xB2(\xAC%e OK!", 159, ((h + 32) >> 1) + 7);
#endif
            break;
        case 4:
        case 8:
            DrawRectBorder(0xC3, 0xE7, 0xFA, 0x2E, 3, 0, 0, 0x80, 0x80, 0x80);
#if TGR_REGION_EUR
            DrawText("%ywOPTIONS LOADED OK!", 159, 131);
#else
        DrawText("%yw%j\xC8(\xCB%e OK!", 159, 131);
#endif
            break;
        case 5:
        case 9:
            DrawRectBorder(0xC3, 0xE7, 0xFA, 0x2E, 3, 0, 0, 0x80, 0x80, 0x80);
#if TGR_REGION_EUR
            DrawText("%ywOPTIONS SAVED OK!", 159, 131);
#else
        DrawText("%yw%j\xB2(\xAC%e OK!", 159, 131);
#endif
            break;
        case 10:
            DrawRectBorder(0xC3, 0xE7, 0xFA, 0x2E, 3, 0, 0, 0x80, 0x80, 0x80);
            DrawText("%ywDONE!", 159, 131);
            break;
        default:
            menuStep = 1;
            break;
        }
        if (PAD->buttons & 0x8030) {
            TransferFlags(PAD, 0x8030);
            if (g_bSaveLoadRumbleSwapPending != 0) {
                menuStep = 17;
            } else {
                FadeSfxTo(1.0f, 0.2f);
                FadeMusicTo(1.0f, 0.2f);
                menuStep = 1;
            }
        }
        break;
    case 17: /* put the Rumble Pak back */
        inputEnabled = 0;
        SetTextStyleNormal();
        SetTextAlignLeft();
        SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xF5, 0);
#if TGR_REGION_EUR
        DrawRectBorder(0x73, 0xB2, 0x19A, 0x98, 3, 0, 0, 0x80, 0x80, 0x80);
        SetTextSize(12);
        DrawText("IF THE RUMBLE PAK IS TO BE USED,", 65, 105);
        DrawText("REMOVE THE CONTROLLER PAK AND", 65, 118);
        DrawText("INSERT THE RUMBLE PAK INTO THE", 65, 131);
        DrawText("CONTROLLER.  PRESS THE A BUTTON", 65, 144);
        DrawText("TO CONTINUE.", 65, 157);
#else
    DrawRectBorder(0xAA, 0xCC, 0x12C, 0x64, 3, 0, 0, 0x80, 0x80, 0x80);
    SetTextSize(14);
    DrawText("%j\xD2\xD3\x8D\x8F\xA1w\xDEPsA5'", 93, 118);
    DrawText("%j\xC1\xCF\xC3\xC8(\x88\x8D\x8F\xA1s", 93, 131);
    DrawText("%j\xD2\xD3\x8D\x8F\xA1w\xD4\xD5"
             "Bc\xDB"
             "2@&",
             93,
             144);
#endif
        if (PAD->buttons & 0x10) {
            TransferFlags(PAD, 0x10);
            if (items[menuCursor].action == 0 || items[menuCursor].action == 2) {
                menuStep = 18;
            } else {
                FadeSfxTo(1.0f, 0.2f);
                FadeMusicTo(1.0f, 0.2f);
                menuStep = 1;
            }
            g_bSaveLoadRumbleSwapPending = 0;
            if (osMotorInit(&g_SiMesgQueue,
                            &g_aContPfs[g_dwMenuChoosingPlayer],
                            g_dwMenuChoosingPlayer) == 0) {
                g_abRumblePakPresent[g_dwMenuChoosingPlayer] = 1;
            }
        }
        break;
    case 18:
        SetFadeTarget(0.0f, 0.2f);
        menuStep = 19;
        break;
    case 19:
        FadeSfxTo(1.0f, 0.2f);
        FadeMusicTo(1.0f, 0.2f);
        break;
    }
    {
        Gfx *g = gDisplayListPtr++;
        g->words.w1 = 0;
        g->words.w0 = 0xE7000000;
    }
    ApplyScreenFade();
    FlushDisplayList();
    if (IsFadedToBlack() != 0) {
        g_bSaveLoadMenuInitialized = 0;
        StartHighResFrame();
        ClearScreen(0, 0, 0);
        FlushDisplayList();
        StartHighResFrame();
        ClearScreen(0, 0, 0);
        FlushDisplayList();
        gControllerPakState = 0;
        if ((items[menuCursor].action == 0 || items[menuCursor].action == 6) &&
            dataWasLoaded != 0) {
            dataWasLoaded = 0;
            SetCurrentScreenCallback(StartChampionshipRace);
        } else if (items[menuCursor].action == 2 && dataWasLoaded != 0) {
            dataWasLoaded = 0;
            SetCurrentScreenCallback(StartTimeAttack);
        } else {
            menuCursor = 0;
            if (g_bSaveLoadEntryPoint == 0) {
                SetCurrentScreenCallback(ProcessMainMenu);
            } else if ((mode = g_bSaveLoadEntryPoint) == 1) {
                SetCurrentScreenCallback(ProcessTrackMenu);
            } else if ((mode | 0) == 2) {
                SetCurrentScreenCallback(ProcessMainMenu);
            }
        }
    }
    g_wSerialInterfaceIdle = 1;
}
#endif
#endif /* TGR_REGION != TGR_REGION_US */
