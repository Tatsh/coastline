#include "options-internal.h"

/* The message that the Controller Pak is not the one the player used before. */
#if TGR_REGION == TGR_REGION_JP
#define DRAW_NEW_PAK_MESSAGE(boxY, textY)                                                          \
    {                                                                                              \
        DRAW_MESSAGE_BOX(0x99, (boxY), 0x14E, 0x89);                                               \
        DrawText("%jあたらしいコントローラパックが", 0x54, (textY));                               \
        DrawText("%jさしこまれています。", 0x54, (textY) + 0xD);                                   \
        DrawText("%jこのコントローラパックを、", 0x54, (textY) + 0x20);                            \
        DrawText("%j使います。", 0x54, (textY) + 0x2D);                                            \
    }
#else
#define DRAW_NEW_PAK_MESSAGE(boxY, textY)                                                          \
    {                                                                                              \
        DRAW_MESSAGE_BOX(0xAF, (boxY), 0x122, 0x89);                                               \
        DrawText("A NEW CONTROLLER PAK", 0x5F, (textY));                                           \
        DrawText("WAS INSERTED.", 0x5F, (textY) + 0xD);                                            \
        DrawText("THIS CONTROLLER PAK", 0x5F, (textY) + 0x20);                                     \
        DrawText("WILL BE USED.", 0x5F, (textY) + 0x2D);                                           \
    }
#endif /* TGR_REGION == TGR_REGION_JP */

void PreparePakDataScreen(void) {
    LoadTextureFromRom(&g_texButtonA);
    LoadTextureFromRom(&g_texButtonB);
#if TGR_REGION != TGR_REGION_JP
    osSyncPrintf("Allocating %d bytes for data_buf...\n", PAK_DATA_BUFFER_SIZE);
#endif
    g_pPakDataBuffer = AllocHeapAligned(PAK_DATA_BUFFER_SIZE);
}

s32 DrawRumblePakPrompt(s32 anyPlayer) {
    s32 player;

    SetTextStyleNormal();
    SetTextAlignLeft();
#if TGR_REGION == TGR_REGION_US || TGR_REGION == TGR_REGION_ASIA
    SetTextSize(12);
    SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xF5, 0x00);
    DRAW_SWAP_FOR_RUMBLE_PAK_MESSAGE(0x73, 0xB4, 0x19A, 0x6A);
#elif TGR_REGION == TGR_REGION_EU
    SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xF5, 0x00);
    DRAW_MESSAGE_BOX(0x73, 0xB4, 0x19A, 0x98);
    SetTextSize(12);
    DRAW_SWAP_FOR_RUMBLE_PAK_TEXT(0x6A);
#else
    SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xF5, 0x00);
    DRAW_MESSAGE_BOX(0xAA, 0xCE, 0x12C, 0x64);
    SetTextSize(14);
    DrawText("%j振動パックを使うときは、", 0x5D, 0x77);
    DrawText("%jコントローラパックと", 0x5D, 0x84);
    DrawText("%j振動パックを交換して下さい。", 0x5D, 0x91);
#endif /* TGR_REGION == TGR_REGION_US */
    for (player = 0; player != 2; player++) {
        if (!anyPlayer && player != g_dwMenuChoosingPlayer) {
            continue;
        }
        if (g_aPlayerInputState[player].buttons & DISMISS_BUTTONS) {
            TransferFlags(&g_aPlayerInputState[player], DISMISS_BUTTONS);
            return TRUE;
        }
    }
    return FALSE;
}

void InitAllRumblePaks(void) {
    s32 player;

    g_wSerialInterfaceIdle = FALSE;
    for (player = 0; player < (s32)g_dwNumLocalPlayers; player++) {
        if (osMotorInit(&g_SiMesgQueue, &g_aContPfs[player], player) == 0) {
            osMotorStop(&g_aContPfs[player]);
            g_abRumblePakPresent[player] = TRUE;
        }
    }
    g_wSerialInterfaceIdle = TRUE;
}

s32 IsRumblePakMissing(void) {
    u8 plugged;
    s32 player;

    g_wSerialInterfaceIdle = FALSE;
    for (player = 0; player < (s32)g_dwNumLocalPlayers; player++) {
        osPfsIsPlug(&g_SiMesgQueue, &plugged);
        if (!(plugged & (1 << player))) {
            goto missing;
        }
        gControllerPakState = osPfsInitPak(&g_SiMesgQueue, &g_aPakPfs[player], player);
        if (gControllerPakState != 0) {
            if (gControllerPakState != PFS_ERR_ID_FATAL) {
                goto missing;
            }
            if (osMotorInit(&g_SiMesgQueue, &g_aContPfs[player], player) != 0) {
                goto missing;
            }
            osMotorStop(&g_aContPfs[player]);
        }
        /* A Controller Pak is not a Rumble Pak. */
        if (gControllerPakState == 0) {
            goto missing;
        }
    }
    g_wSerialInterfaceIdle = TRUE;
    return FALSE;

missing:
    g_wSerialInterfaceIdle = TRUE;
    return TRUE;
}

/* The original reads a byte of its stack that is never set, and other data types set no note. */
s32 ProcessPakDataScreen(s32 dataType, u8 quiet) {
#ifdef __sgi
    static s32 g_dwPakDataNoteIndex;
    static u8 g_bPakDataStep;
    static u8 g_bPakDataWaitFrames;
    /* Preserve the original unused locals to reproduce their stack slots. */
    s32 unusedUpperSlot;
#endif
    s32 top;
#ifdef __sgi
    s32 unusedMiddleSlots[5];
#endif
    s32 buttonY;
#ifdef __sgi
    s32 unusedLowerSlot;
#endif
#if defined(ENABLE_PATCHES) || !defined(__sgi)
    /* Another data type opens no note, and the offset of the prompt adds nothing. */
    u8 *noteName = NULL;
#else
    u8 *noteName;
#endif
    s32 freeBlocks;
    u8 plugged;
#if defined(ENABLE_PATCHES) || !defined(__sgi)
    u8 unsetOffset = 0;
    s32 noteSize = 0;
#else
    u8 unsetOffset;
    s32 noteSize;
#endif
    s32 equipment[EQUIPMENT_FIELDS];
    /* TOP GEAR SEASON, TOP GEAR GHOST and TOP GEAR OPTIONS in the font of the Controller Pak. */
    u8 seasonName[16] = {0x2D,
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
    u8 ghostName[16] = {0x2D,
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
    u8 configurationName[16] = {0x2D,
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
    u8 extension[4] = {0};

    g_wSerialInterfaceIdle = FALSE;
    if (!g_bSaveLoadMenuInitialized) {
        g_bSaveLoadMenuInitialized = TRUE;
        gControllerPakState = 0;
        g_bPakDataStep = PAK_DATA_STEP_START;
        g_bPakDataWaitFrames = 0;
    }
    if (dataType == PAK_DATA_LOAD_SEASON) {
        noteName = seasonName;
        noteSize = SEASON_NOTE_SIZE;
    } else if (dataType == PAK_DATA_LOAD_GHOST) {
        noteName = ghostName;
        noteSize = PAK_DATA_BUFFER_SIZE;
    } else if (dataType == PAK_DATA_LOAD_CONFIGURATION || dataType == PAK_DATA_SAVE_CONFIGURATION) {
        noteName = configurationName;
        noteSize = OPTIONS_NOTE_SIZE;
    }

    switch (g_bPakDataStep) {
    case PAK_DATA_STEP_START:
        if (!quiet) {
            FadeSfxTo(0.0f, FADE_SECONDS);
            FadeMusicTo(0.0f, FADE_SECONDS);
            g_bPakDataStep = PAK_DATA_STEP_WAIT_QUIET;
        } else {
            g_bPakDataStep = PAK_DATA_STEP_MOUNT;
        }
        break;

    case PAK_DATA_STEP_WAIT_QUIET:
        /* The count of frames is not reset, so it waits only the first time. */
        if (HasSfxFadeFinished() && g_bPakDataWaitFrames++ == 3) {
            g_bPakDataStep = PAK_DATA_STEP_MOUNT;
        }
        break;

    case PAK_DATA_STEP_MOUNT:
        osPfsIsPlug(&g_SiMesgQueue, &plugged);
        if (!(plugged & (1 << g_dwMenuChoosingPlayer))) {
            gControllerPakState = PFS_ERR_NOPACK;
            g_bPakDataStep = PAK_DATA_STEP_ERROR;
            break;
        }
        PAK_LOG(("\nInitializing controller pak...\n"));
        gControllerPakState = osPfsInitPak(
            &g_SiMesgQueue, &g_aPakPfs[g_dwMenuChoosingPlayer], g_dwMenuChoosingPlayer);
        if (gControllerPakState != 0) {
            if (gControllerPakState == PFS_ERR_ID_FATAL) {
                if (osMotorInit(&g_SiMesgQueue,
                                &g_aContPfs[g_dwMenuChoosingPlayer],
                                g_dwMenuChoosingPlayer) == 0) {
                    osMotorStop(&g_aContPfs[g_dwMenuChoosingPlayer]);
                    gControllerPakState = PAK_STATE_RUMBLE_PAK;
                    g_bSaveLoadRumbleSwapPending = TRUE;
                    g_bPakDataStep = PAK_DATA_STEP_ERROR;
                } else {
                    gControllerPakState = 0;
                    g_bPakDataStep = PAK_DATA_STEP_REPAIR;
                }
            } else {
                g_abRumblePakPresent[g_dwMenuChoosingPlayer] = FALSE;
                g_bPakDataStep = PAK_DATA_STEP_ERROR;
            }
        } else if (gControllerPakState == 0) {
            g_abRumblePakPresent[g_dwMenuChoosingPlayer] = FALSE;
            if (!g_abSavePakIdKnown[g_dwMenuChoosingPlayer]) {
                g_abSavePakIdKnown[g_dwMenuChoosingPlayer] = TRUE;
                memcpy(g_aabSavePakIds[g_dwMenuChoosingPlayer],
                       g_aPakPfs[g_dwMenuChoosingPlayer].id,
                       sizeof(g_aPakPfs[0].id));
                g_bPakDataStep = PAK_DATA_STEP_FIND;
            } else if (bcmp(g_aabSavePakIds[g_dwMenuChoosingPlayer],
                            g_aPakPfs[g_dwMenuChoosingPlayer].id,
                            sizeof(g_aPakPfs[0].id)) != 0) {
                memcpy(g_aabSavePakIds[g_dwMenuChoosingPlayer],
                       g_aPakPfs[g_dwMenuChoosingPlayer].id,
                       sizeof(g_aPakPfs[0].id));
                g_bPakDataStep = PAK_DATA_STEP_NEW_PAK;
            } else {
                g_bPakDataStep = PAK_DATA_STEP_FIND;
            }
        }
        break;

    case PAK_DATA_STEP_REPAIR:
        gControllerPakState = osPfsRepairId(&g_aPakPfs[g_dwMenuChoosingPlayer]);
        if (gControllerPakState != 0) {
            g_bPakDataStep = PAK_DATA_STEP_ERROR;
        } else {
            g_bPakDataStep = PAK_DATA_STEP_MOUNT;
        }
        break;

    case PAK_DATA_STEP_NEW_PAK:
        if (quiet) {
            g_bPakDataStep = PAK_DATA_STEP_FIND;
            break;
        }
#if TGR_REGION == TGR_REGION_US || TGR_REGION == TGR_REGION_ASIA
        SET_MESSAGE_TEXT();
#else
        SetTextStyleNormal();
        SetTextAlignLeft();
        SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xF5, 0x00);
        SetTextSize(MESSAGE_TEXT_SIZE);
#endif
#if TGR_REGION == TGR_REGION_JP
        DRAW_MESSAGE_BOX(0x99, 0xC3, 0x14E, 0x89);
        DrawText("%jあたらしいコントローラパックが", 0x54, 0x71);
        DrawText("%jさしこまれています。", 0x54, 0x7E);
        DrawText("%jこのコントローラパックを、", 0x54, 0x91);
        DrawText("%j使います。", 0x54, 0x9E);
#else
        DRAW_NEW_PAK_MESSAGE(0xC3, 0x71);
#endif
        if (CHOOSING_INPUT->buttons & DISMISS_BUTTONS) {
            TransferFlags(CHOOSING_INPUT, DISMISS_BUTTONS);
            g_bPakDataStep = PAK_DATA_STEP_FIND;
        }
        break;

    case PAK_DATA_STEP_FIND:
        PAK_LOG(("Finding file...\n"));
        gControllerPakState = osPfsFindFile(&g_aPakPfs[g_dwMenuChoosingPlayer],
                                            COMPANY_CODE,
                                            GAME_CODE,
                                            noteName,
                                            extension,
                                            &g_dwPakDataNoteIndex);
        if (gControllerPakState == PFS_ERR_INCONSISTENT) {
            osPfsChecker(&g_aPakPfs[g_dwMenuChoosingPlayer]);
            gControllerPakState = osPfsFindFile(&g_aPakPfs[g_dwMenuChoosingPlayer],
                                                COMPANY_CODE,
                                                GAME_CODE,
                                                noteName,
                                                extension,
                                                &g_dwPakDataNoteIndex);
        }
        if (gControllerPakState == 0) {
            if (dataType == PAK_DATA_SAVE_CONFIGURATION) {
                g_bPakDataStep = PAK_DATA_STEP_SAVE;
            } else {
                g_bPakDataStep = PAK_DATA_STEP_LOAD;
            }
        } else if (dataType == PAK_DATA_SAVE_CONFIGURATION) {
            g_bPakDataStep = PAK_DATA_STEP_ALLOCATE;
        } else if (quiet) {
            g_bPakDataStep = PAK_DATA_STEP_FINISH;
        } else {
            g_bPakDataStep = PAK_DATA_STEP_ERROR;
        }
        break;

    case PAK_DATA_STEP_LOAD:
#if TGR_REGION != TGR_REGION_JP
        if (dataType == PAK_DATA_LOAD_SEASON) {
            PAK_LOG(("Loading season data...\n"));
        } else if (dataType == PAK_DATA_LOAD_GHOST) {
            PAK_LOG(("Loading ghost data...\n"));
        } else if (dataType == PAK_DATA_LOAD_CONFIGURATION) {
            PAK_LOG(("Loading configuration...\n"));
        }
#endif
        if (dataType != PAK_DATA_LOAD_CONFIGURATION) {
            gControllerPakState = osPfsReadWriteFile(&g_aPakPfs[g_dwMenuChoosingPlayer],
                                                     g_dwPakDataNoteIndex,
                                                     PFS_READ,
                                                     0,
                                                     noteSize,
                                                     g_pPakDataBuffer);
        } else {
            gControllerPakState = osPfsReadWriteFile(&g_aPakPfs[g_dwMenuChoosingPlayer],
                                                     g_dwPakDataNoteIndex,
                                                     PFS_READ,
                                                     0,
                                                     OPTIONS_HALF_SIZE,
                                                     g_pPakDataBuffer);
        }
        if (gControllerPakState == 0) {
            if (dataType == PAK_DATA_LOAD_SEASON) {
                /* Both players take the season, but the other player retains its car equipment. */
                GET_EQUIPMENT(equipment, g_aVehicles[g_dwMenuChoosingPlayer ^ 1].profile);
                memcpy(g_aVehicles[0].profile, g_pPakDataBuffer, sizeof(PlayerProfile));
                memcpy(g_aVehicles[1].profile, g_pPakDataBuffer, sizeof(PlayerProfile));
                SET_EQUIPMENT(g_dwMenuChoosingPlayer ^ 1, equipment);
                PAK_LOG(("Done!\n"));
            } else if (dataType == PAK_DATA_LOAD_GHOST) {
                PAK_LOG(("Extracting ghost data...\n"));
                g_dwGhostRecordingSize = DecompressInterleavedRle(
                    g_abGhostRecording, GHOST_RECORDING_CAPACITY, g_pPakDataBuffer, GHOST_CHANNELS);
                g_aVehicles[0].profile->unlockedTracks |= 1 << g_abGhostRecording[0];
                g_aVehicles[0].profile->unlocks |= 1 << g_abGhostRecording[1];
                PAK_LOG(("Done!\n"));
            } else if (dataType == PAK_DATA_LOAD_CONFIGURATION) {
                memcpy(&g_OptionSettings, g_pPakDataBuffer, sizeof(OptionSettings));
                PAK_LOG(("Loading car equipment settings...\n"));
                gControllerPakState = osPfsReadWriteFile(&g_aPakPfs[g_dwMenuChoosingPlayer],
                                                         g_dwPakDataNoteIndex,
                                                         PFS_READ,
                                                         OPTIONS_HALF_SIZE,
                                                         OPTIONS_HALF_SIZE,
                                                         g_pPakDataBuffer);
                if (gControllerPakState == 0) {
                    memcpy(equipment, g_pPakDataBuffer, sizeof(equipment));
                    SET_EQUIPMENT(g_dwMenuChoosingPlayer, equipment);
                    PAK_LOG(("Done!\n"));
                } else {
                    g_bPakDataStep = PAK_DATA_STEP_ERROR;
                }
            }
            if (g_bPakDataStep != PAK_DATA_STEP_ERROR) {
                if (dataType == PAK_DATA_LOAD_CONFIGURATION) {
                    g_bPakDataStep = PAK_DATA_STEP_DONE;
                } else if (!g_bSaveLoadRumbleSwapPending) {
                    g_bPakDataStep = PAK_DATA_STEP_FINISH;
                } else {
                    g_bPakDataStep = PAK_DATA_STEP_DONE;
                }
            }
        } else {
            g_bPakDataStep = PAK_DATA_STEP_ERROR;
        }
        break;

    case PAK_DATA_STEP_ALLOCATE:
        if ((gControllerPakState =
                 osPfsFreeBlocks(&g_aPakPfs[g_dwMenuChoosingPlayer], &freeBlocks)) == 0) {
            if (freeBlocks < OPTIONS_NOTE_SIZE) {
                gControllerPakState = PFS_DATA_FULL;
                g_bPakDataStep = PAK_DATA_STEP_ERROR;
            } else {
                PAK_LOG(("Allocating %d bytes for configuration...\n", OPTIONS_NOTE_SIZE));
                gControllerPakState = osPfsAllocateFile(&g_aPakPfs[g_dwMenuChoosingPlayer],
                                                        COMPANY_CODE,
                                                        GAME_CODE,
                                                        noteName,
                                                        extension,
                                                        OPTIONS_NOTE_SIZE,
                                                        &g_dwPakDataNoteIndex);
                if (gControllerPakState == PFS_ERR_INCONSISTENT) {
                    osPfsChecker(&g_aPakPfs[g_dwMenuChoosingPlayer]);
                    gControllerPakState = osPfsAllocateFile(&g_aPakPfs[g_dwMenuChoosingPlayer],
                                                            COMPANY_CODE,
                                                            GAME_CODE,
                                                            noteName,
                                                            extension,
                                                            OPTIONS_NOTE_SIZE,
                                                            &g_dwPakDataNoteIndex);
                }
                if (gControllerPakState == 0) {
                    g_bPakDataStep = PAK_DATA_STEP_SAVE;
                } else {
                    g_bPakDataStep = PAK_DATA_STEP_ERROR;
                }
            }
        } else {
            g_bPakDataStep = PAK_DATA_STEP_ERROR;
        }
        break;

    case PAK_DATA_STEP_SAVE:
        PAK_LOG(("Saving configuration...\n"));
        memcpy(g_pPakDataBuffer, &g_OptionSettings, sizeof(OptionSettings));
        gControllerPakState = osPfsReadWriteFile(&g_aPakPfs[g_dwMenuChoosingPlayer],
                                                 g_dwPakDataNoteIndex,
                                                 PFS_WRITE,
                                                 0,
                                                 OPTIONS_HALF_SIZE,
                                                 g_pPakDataBuffer);
        if (gControllerPakState != 0) {
            g_bPakDataStep = PAK_DATA_STEP_ERROR;
        } else {
            GET_EQUIPMENT(equipment, g_aVehicles[g_dwMenuChoosingPlayer].profile);
            memcpy(g_pPakDataBuffer, equipment, sizeof(equipment));
            gControllerPakState = osPfsReadWriteFile(&g_aPakPfs[g_dwMenuChoosingPlayer],
                                                     g_dwPakDataNoteIndex,
                                                     PFS_WRITE,
                                                     OPTIONS_HALF_SIZE,
                                                     OPTIONS_HALF_SIZE,
                                                     g_pPakDataBuffer);
            if (gControllerPakState == 0) {
                PAK_LOG(("Done!\n"));
                g_bPakDataStep = PAK_DATA_STEP_DONE;
            } else {
                g_bPakDataStep = PAK_DATA_STEP_ERROR;
            }
        }
        break;

    case PAK_DATA_STEP_ERROR:
        if (quiet) {
            g_bPakDataStep = PAK_DATA_STEP_FINISH;
            break;
        }
        SET_MESSAGE_TEXT();
        switch (gControllerPakState) {
#if TGR_REGION == TGR_REGION_JP
        case PFS_ERR_ID_FATAL:
            DRAW_SHORT_MESSAGE(0x50,
                               0x1E0,
                               0x30,
                               "%jコントローラパックが、こわれています。",
                               "%jあたらしいコントローラパックを、さして下さい。");
            break;
        case PFS_ERR_NOPACK:
        case PFS_ERR_DEVICE:
            DRAW_MESSAGE_BOX(0x94, 0xD8, 0x158, 0x4C);
            DrawText("%jコントローラパックがありません。", 0x52, 0x7C);
            if (dataType == PAK_DATA_SAVE_CONFIGURATION) {
                DrawText("%jデータをセーブできません。", 0x52, 0x8A);
            } else {
                DrawText("%jデータをロードできません。", 0x52, 0x8A);
            }
            break;
        case PAK_STATE_RUMBLE_PAK: {
            buttonY = 0x13B - g_texButtonA.width;
            DRAW_MESSAGE_BOX(0xC8, 0xBD, 0xF0, 0x86);
            DrawText("%j振動パックと", 0x6C, 0x6E);
            DrawText("%jコントローラパックを", 0x6C, 0x7B);
            DrawText("%j交換して下さい。", 0x6C, 0x88);
            SetTextAlignLeft();
            SetTextSize(10);
            DrawText("%wwOK", (u32)(g_texButtonA.width + 0xDE) >> 1, (buttonY + 0x12) >> 1);
            DrawText("%wwCANCEL", (u32)(g_texButtonB.width + 0x14A) >> 1, (buttonY + 0x12) >> 1);
            DrawTexturedSprite(&g_texButtonA, 0xD8, buttonY);
            DrawTexturedSprite(&g_texButtonB, 0x144, buttonY);
            break;
        }
        case PFS_DATA_FULL:
        case PFS_DIR_FULL:
            DRAW_MESSAGE_BOX(0xA5, 0xBD, 0x136, 0x8D);
            DrawText("%jコントローラパックに", 0x5A, 0x6E);
            DrawText("%j空きページがありません。", 0x5A, 0x7B);
            DrawText("%jオプションデータをセーブするには", 0x5A, 0x8E);
            DrawText("1%jページ必要です。", 0x5A, 0x9B);
            break;
        case PFS_ERR_CONTRFAIL:
            DRAW_SHORT_MESSAGE(0xBF, 0x102, 0x67, "%jコントローラにエラー", "%jが発生しました。");
            break;
        case PFS_ERR_INVALID:
            DRAW_MESSAGE_BOX(0xAF, 0xD9, 0x122, 0x4A);
            if (dataType == PAK_DATA_SAVE_CONFIGURATION) {
                DrawText("%jコントローラパック", 0x5F, 0x7C);
                DrawText("%jが異常です。", 0x5F, 0x8A);
            } else {
                DrawText("%jコントローラパックに", 0x5F, 0x7C);
                DrawText("%jデータがありません。", 0x5F, 0x8A);
            }
            break;
        case PFS_ERR_NEW_PACK:
            DRAW_MESSAGE_BOX(0x99, 0xB9, 0x14E, 0x89);
            DrawText("%jあたらしいコントローラパックが", 0x54, 0x6C);
            DrawText("%jさしこまれています。", 0x54, 0x79);
            DrawText("%jこのコントローラパックを、", 0x54, 0x8C);
            DrawText("%j使います。", 0x54, 0x99);
            break;
        default:
            DRAW_SHORT_MESSAGE(0xAF, 0x122, 0x5F, "%jコントローラパック", "%jが異常です。");
            break;
#else
        case PFS_ERR_ID_FATAL:
            DRAW_SHORT_MESSAGE(0xBD, 0x106, 0x66, "THE CONTROLLER PAK", "IS NONFUNCTIONAL!");
            break;
        case PFS_ERR_NOPACK:
        case PFS_ERR_DEVICE:
            DRAW_MESSAGE_BOX(0x94, 0xD8, 0x158, 0x4C);
            DrawText("CONTROLLER PAK NOT FOUND.", 0x52, 0x7C);
            if (dataType == PAK_DATA_SAVE_CONFIGURATION) {
                DrawText("DATA CANNOT BE SAVED.", 0x52, 0x8A);
            } else {
                DrawText("DATA CANNOT BE LOADED.", 0x52, 0x8A);
            }
            break;
        case PAK_STATE_RUMBLE_PAK:
            buttonY = 0x148 - g_texButtonA.width;
            DRAW_MESSAGE_BOX(0x79, 0xB0, 0x18D, 0xA0);
            DrawText("PLEASE REMOVE THE RUMBLE PAK", 0x44, 0x68);
            DrawText("AND INSERT THE CONTROLLER PAK", 0x44, 0x75);
            DrawText("INTO THE CONTROLLER.  PRESS", 0x44, 0x82);
            DrawText("THE A BUTTON WHEN READY.", 0x44, 0x8F);
            SetTextAlignLeft();
            SetTextSize(10);
            DrawText("%wwOK", (g_texButtonA.width + 0xE3U) >> 1, (buttonY + 18) >> 1);
            DrawText("%wwCANCEL", (g_texButtonB.width + 0x144U) >> 1, (buttonY + 18) >> 1);
            DrawTexturedSprite(&g_texButtonA, 0xDD, buttonY);
            DrawTexturedSprite(&g_texButtonB, 0x13E, buttonY);
            break;
        case PFS_DATA_FULL:
        case PFS_DIR_FULL:
            DRAW_MESSAGE_BOX(0xA5, 0xA5, 0x136, 0xBD);
            DRAW_INSUFFICIENT_PAGES_MESSAGE(0x62, "ONE PAGE AND ONE NOTE", "OPTIONS.");
            break;
        case PFS_ERR_CONTRFAIL:
            DRAW_SHORT_MESSAGE(0xBF, 0x102, 0x67, "CONTROLLER ERROR", "HAS BEEN DETECTED!");
            break;
        case PFS_ERR_INVALID:
            DRAW_MESSAGE_BOX(0xAF, 0xD9, 0x122, 0x4A);
            if (dataType == PAK_DATA_SAVE_CONFIGURATION) {
                DrawText("CONTROLLER PAK ERROR", 0x5F, 0x7C);
                DrawText("HAS BEEN DETECTED!", 0x5F, 0x8A);
            } else {
                DrawText("SAVED DATA NOT FOUND", 0x5F, 0x7C);
                DrawText("IN CONTROLLER PAK.", 0x5F, 0x8A);
            }
            break;
        case PFS_ERR_NEW_PACK:
            DRAW_NEW_PAK_MESSAGE(0xB9, 0x6C);
            break;
        default:
            DRAW_SHORT_MESSAGE(0xAF, 0x122, 0x5F, "CONTROLLER PAK ERROR", "HAS BEEN DETECTED.");
            break;
#endif /* TGR_REGION == TGR_REGION_JP */
        }
        if (gControllerPakState != PAK_STATE_RUMBLE_PAK) {
            if (CHOOSING_INPUT->buttons & DISMISS_BUTTONS) {
                TransferFlags(CHOOSING_INPUT, DISMISS_BUTTONS);
                gControllerPakState = 0;
                /* The state was just cleared, so the screen always finishes. */
                if (gControllerPakState == PFS_ERR_NEW_PACK) {
                    g_bPakDataStep = PAK_DATA_STEP_MOUNT;
                } else {
                    g_bPakDataStep = PAK_DATA_STEP_FINISH;
                }
            }
        } else if (CHOOSING_INPUT->buttons & INPUT_A) {
            TransferFlags(CHOOSING_INPUT, INPUT_A);
            gControllerPakState = 0;
            g_bPakDataStep = PAK_DATA_STEP_MOUNT;
        } else if (CHOOSING_INPUT->buttons & INPUT_B) {
            TransferFlags(CHOOSING_INPUT, INPUT_B);
            g_bPakDataStep = PAK_DATA_STEP_FINISH;
        }
        break;

    case PAK_DATA_STEP_DONE:
        if (quiet) {
            g_bPakDataStep = PAK_DATA_STEP_FINISH;
            break;
        }
        SetTextAlignCenter();
        SetTextStyleNormal();
        SetTextSize(MESSAGE_TEXT_SIZE);
        switch (dataType) {
        case PAK_DATA_LOAD_SEASON:
            if (g_bSaveLoadEntryPoint == SAVE_LOAD_FROM_MAIN_MENU) {
                top = unsetOffset + 0xCB;
            } else {
                top = 0xCE;
            }
#if TGR_REGION == TGR_REGION_JP
            DRAW_MESSAGE_BOX(0xE2, top, 0xBC, 0x4A);
            DrawText("%yw%jロード%e OK!", 0x9F, ((top + 0x20) >> 1) + 7);
#else
            DRAW_DONE_MESSAGE(0xE2, 0xBC, top, "%ywSEASON DATA", "%ywLOADED OK!");
#endif
            break;
        case PAK_DATA_LOAD_GHOST:
            if (g_bSaveLoadEntryPoint == SAVE_LOAD_FROM_MAIN_MENU) {
                top = unsetOffset + 0xCB;
            } else {
                top = 0xCE;
            }
#if TGR_REGION == TGR_REGION_JP
            DRAW_MESSAGE_BOX(0xE2, top, 0xBC, 0x4A);
            DrawText("%yw%jロード%e OK!", 0x9F, ((top + 0x20) >> 1) + 7);
#else
            DRAW_DONE_MESSAGE(0xE2, 0xBC, top, "%ywGHOST DATA", "%ywLOADED OK!");
#endif
            break;
#if TGR_REGION == TGR_REGION_US || TGR_REGION == TGR_REGION_ASIA
        case PAK_DATA_LOAD_CONFIGURATION:
        case PAK_DATA_SAVE_CONFIGURATION:
            DRAW_MESSAGE_BOX(0xD8, 0xD9, 0xD0, 0x4A);
            DrawText("%ywCONFIGURATION", 0x9F, 0x7C);
            if (dataType == PAK_DATA_LOAD_CONFIGURATION) {
                DrawText("%ywLOADED OK!", 0x9F, 0x8A);
            } else {
                DrawText("%ywSAVED OK!", 0x9F, 0x8A);
            }
            break;
#endif
        }
        if (CHOOSING_INPUT->buttons & DISMISS_BUTTONS) {
            TransferFlags(CHOOSING_INPUT, DISMISS_BUTTONS);
            if (dataType == PAK_DATA_LOAD_CONFIGURATION) {
                ApplyVolumeSettings();
            }
            if (g_bSaveLoadRumbleSwapPending) {
                g_bPakDataStep = PAK_DATA_STEP_RUMBLE_PAK;
            } else {
                g_bPakDataStep = PAK_DATA_STEP_FINISH;
            }
        }
        break;

    case PAK_DATA_STEP_RUMBLE_PAK:
        if (DrawRumblePakPrompt(FALSE)) {
            g_bSaveLoadRumbleSwapPending = FALSE;
            if (osMotorInit(&g_SiMesgQueue,
                            &g_aContPfs[g_dwMenuChoosingPlayer],
                            g_dwMenuChoosingPlayer) == 0) {
                g_abRumblePakPresent[g_dwMenuChoosingPlayer] = TRUE;
            }
            g_bPakDataStep = PAK_DATA_STEP_FINISH;
        }
        break;

    case PAK_DATA_STEP_FINISH:
        gControllerPakState = 0;
        g_bSaveLoadMenuInitialized = FALSE;
        if (!quiet) {
            FadeSfxTo(1.0f, FADE_SECONDS);
            FadeMusicTo(1.0f, FADE_SECONDS);
        }
        g_wSerialInterfaceIdle = TRUE;
        return TRUE;
    }

    g_wSerialInterfaceIdle = TRUE;
    return FALSE;
}
