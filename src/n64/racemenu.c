#include "racemenu.h"

#include <PR/gu.h>
#include <PR/os.h>
#include <PR/os_libc.h>

#include "carselect.h"
#include "common/audio.h"
#include "common/audiomix.h"
#include "common/fade.h"
#include "common/game.h"
#include "common/heap.h"
#include "common/input.h"
#include "common/race.h"
#include "common/render.h"
#include "common/replay.h"
#include "common/system.h"
#include "common/text.h"
#include "common/timing.h"
#include "common/trackselect.h"
#include "common/util.h"
#include "common/weather.h"
#include "mainmenu.h"
#include "options.h"
#include "playerselect.h"
#include "weatherselect.h"

/* The sounds of moving the cursor and of choosing, on the cartridge. */
#ifdef COASTLINE_ROM_POINTERS
#define ROM_SCROLL_SOUND _scrollSoundSegmentRomStart
#define ROM_SELECT_SOUND _selectSoundSegmentRomStart
#define ROM_MODEL_SOURCE(offset) ((char *)(offset))
#else
#define ROM_SCROLL_SOUND ((u32)_scrollSoundSegmentRomStart)
#define ROM_SELECT_SOUND ((u32)_selectSoundSegmentRomStart)
#define ROM_MODEL_SOURCE(offset) (offset)
#endif

#if TGR_REGION == TGR_REGION_JP
#define TRACK_MENU_TITLE "COURSE SELECT"
#else
#define TRACK_MENU_TITLE "TRACK SELECT"
#endif

/* The silence after each sound, in bytes. */
#define SOUND_TAIL_BYTES 0x100

/* How long the round under way blinks in the records, and how long of that it shows yellow. */
#define BLINK_PERIOD 0.75f
#define BLINK_ON 0.4f

/* The degrees of a radian. */
#define RAD_TO_DEG 57.29577637f

/* The scale of a model on the stage of a screen. */
#define STAGE_MODEL_SCALE 0.03125f

/* The points of the beam of the spotlight, and its radius. */
#define SPOTLIGHT_POINTS 31
#define SPOTLIGHT_RADIUS 64.0f

/* The laps of an arcade race. */
#define MIN_LAPS 1
#define MAX_LAPS 5

/* The tracks whose names are mirrored ones, past the first five. */
#define MIRRORED_TRACKS 5

/*
 * Choose a car of the season of a profile for each of the two slots, and load them at once. The
 * original reads the season from the profile for each slot.
 */
#define LOAD_SEASON_CARS(profile, slot)                                                            \
    for (slot = 0; slot < 2; slot++) {                                                             \
        LoadVehicleModel(                                                                          \
            &g_aVehicleModelSlots[slot], slot, g_SeasonTable[(profile)->season].cars[slot], slot); \
        while (PollVehicleModelDma(&g_aVehicleModelSlots[slot]) != 0) {                            \
        }                                                                                          \
    }

/* Draw the buttons at the bottom of a screen and their names. */
#define DRAW_FOOTER_BUTTONS(leftX, rightX, leftText, leftTextX, rightText, rightTextX)             \
    SetTextStyleNormal();                                                                          \
    SetTextAlignLeft();                                                                            \
    DrawTexturedRect(                                                                              \
        &g_ArrowLeftTextureDesc, leftX, 216, 12, 12, 0, 0, 0, 0xFF, 0, 0, 0, 0xFF, 0);             \
    DrawTexturedRect(                                                                              \
        &g_ArrowRightTextureDesc, rightX, 216, 12, 12, 0, 0, 0, 0xFF, 0, 0, 0, 0xFF, 0);           \
    SetTextSize(11);                                                                               \
    DrawText(leftText, leftTextX, g_dwScreenHeight * 19 / 20 - 3);                                 \
    DrawText(rightText, rightTextX, g_dwScreenHeight * 19 / 20 - 3)

s32 IsTrackUnlocked(s32 track) {
    if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP) {
        if (g_bSeasonFirstRace) {
            return track == (s32)g_dwSelectedTrackId;
        }
        return track == (s32)g_dwSelectedTrackId || track >= g_dwNumTracks;
    }
    if (g_dwGameMode == GAME_MODE_TIME_ATTACK &&
        (track == g_dwNumTracks || track == g_dwNumTracks + 2)) {
        return TRUE;
    }
    return (g_aVehicles[0].profile->unlockedTracks & (1 << track)) && IsTrackIndexValid(track);
}

/* The original tests the unsigned season and round for being negative. */
void ValidateChampionshipProgress(void) {
    if (g_aVehicles[0].profile->championshipFlags < 0) {
        g_aVehicles[0].profile->championshipFlags = 0;
    resetSeason:
        g_aVehicles[0].profile->season = 0;
    resetRound:
        g_aVehicles[0].profile->round = 0;
        g_aVehicles[0].profile->seasonPoints[g_aVehicles[0].profile->season] = 0;
    }
#ifdef ENABLE_PATCHES
    /* An unsigned season and round are never negative. */
    if (g_aVehicles[0].profile->season >= SEASONS) {
#else
    if (g_aVehicles[0].profile->season < 0 || g_aVehicles[0].profile->season >= SEASONS) {
#endif /* ENABLE_PATCHES */
        goto resetSeason;
    }
#ifdef ENABLE_PATCHES
    if (g_aVehicles[0].profile->round >= g_SeasonTable[g_aVehicles[0].profile->season].raceCount) {
#else
    if (g_aVehicles[0].profile->round < 0 ||
        g_aVehicles[0].profile->round >= g_SeasonTable[g_aVehicles[0].profile->season].raceCount) {
#endif /* ENABLE_PATCHES */
        goto resetRound;
    }
}

void TrackMenuPostDraw(s32 item) {
    s32 needed, slot;

#if TGR_REGION != TGR_REGION_JP
    if (g_dwGameMode == GAME_MODE_ARCADE) {
        SetTextSize(10);
        SetTextStyleNormal();
        SetTextAlignCenter();
        DrawText("%wwPush up or down to adjust number of laps", 160, 90);
    }
#endif
    if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP && item < g_dwNumTracks) {
        SetTextSize(10);
        SetTextStyleNormal();
        SetTextAlignCenter();
        needed = g_SeasonTable[g_aVehicles[0].profile->season].pointsNeeded -
                 g_aVehicles[0].profile->seasonPoints[g_aVehicles[0].profile->season];
        if (needed <= 0) {
#if TGR_REGION == TGR_REGION_JP
            sprintf(g_szTrackSelectPointsTextBuf, "%%wwYOU ARE READY TO NEXT SEASON");
#else
            sprintf(g_szTrackSelectPointsTextBuf,
                    "%%wwYou have enough points to advance to next season");
#endif
        } else {
#if TGR_REGION == TGR_REGION_JP
            DrawText("%ww1st=9, 2nd=6, 3rd=5, 4th=3, 5th=2, 6th=1", 160, 100);
#else
            DrawText("%wwFirst=9, Second=6, Third=5, Fourth=3, Fifth=2, Sixth=1", 160, 100);
#endif
            sprintf(g_szTrackSelectPointsTextBuf,
                    "%%ww%d point%s needed to advance to next season",
                    needed,
                    needed == 1 ? "" : "s");
        }
        DrawText(g_szTrackSelectPointsTextBuf, 160, 90);
    }
    if (!g_bTrackMenuPakScreen) {
        return;
    }
    if (ProcessPakDataScreen(g_dwGameMode == GAME_MODE_TIME_ATTACK, 0)) {
        g_bTrackMenuPakScreen = FALSE;
        if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP) {
            ValidateChampionshipProgress();
            StartChampionshipRace();
            LOAD_SEASON_CARS(g_aVehicles[0].profile, slot);
            g_bSeasonFirstRace = TRUE;
        } else if (g_dwGameMode == GAME_MODE_TIME_ATTACK) {
            StartTimeAttack();
            /* The loaded ghost picks its track when that track is unlocked. */
            if (g_aVehicles[0].profile->unlockedTracks & (1 << g_abGhostRecording[0])) {
                g_dwSelectedTrackId = g_abGhostRecording[0];
            }
        }
        SetScreenPendingSelection(g_dwSelectedTrackId);
    } else {
        /* The menu takes no input while the screen of the controller pak runs. */
        g_aPlayerInputState[0].buttons = 0;
        g_aPlayerInputState[0].stickX = 0.0f;
    }
}

void SeasonRecordsPreDrawCallback(void) {
    static float g_fSeasonRecordsBlinkTime = 0.0f;
#ifdef __GNUC__
    static char g_szTrackSelectRecordTextBuf[80] __attribute__((section(".bss.season_records")));
#else
    static char g_szTrackSelectRecordTextBuf[80];
#endif
    /* The unused buffer preserves IDO's stack frame. */
    char unusedFrameBuffer[32] TGR_MATCHING_UNUSED;
    int raceIndex;
    int track;
    int currentRace;
    int seasonIndex;
    unsigned int championshipFlags;
    int shift;
    int y;

    y = 95;
    for (g_fSeasonRecordsBlinkTime += g_fFrameDeltaTime; g_fSeasonRecordsBlinkTime > 0.75f;
         g_fSeasonRecordsBlinkTime -= 0.75f) {
    }
    currentRace = g_aVehicles[0].profile->round;
    seasonIndex = g_aVehicles[0].profile->season;
    championshipFlags = g_aVehicles[0].profile->championshipFlags;
    SetTextStyleNormal();
    SetTextAlignCenter();
    SetTextSize(20);
    sprintf(g_szTrackSelectRecordTextBuf, "%%ry%s:", (char *)g_SeasonTable[seasonIndex].subtitle);
    DrawText(g_szTrackSelectRecordTextBuf, g_dwScreenWidth / 2, g_dwScreenHeight * 20 / 64);
    shift = 0;
    for (raceIndex = 0; raceIndex < g_SeasonTable[seasonIndex].raceCount; raceIndex++) {
        if (g_SeasonTable[seasonIndex].races[raceIndex][0] < 5) {
            if (championshipFlags & 1) {
                shift = g_dwScreenWidth * 3 / 64;
                break;
            }
        } else if (!(championshipFlags & 1)) {
            shift = g_dwScreenWidth * 3 / 64;
            break;
        }
    }
    SetTextSize(8);
    SetTextAlignRight();
    DrawText("Race Record", g_dwScreenWidth * 23 / 32 + shift, 0x56);
    DrawText("Lap Record", g_dwScreenWidth * 28 / 32 + shift + 2, 0x56);
    SetTextSize(10);
    for (raceIndex = 0; raceIndex < g_SeasonTable[seasonIndex].raceCount; raceIndex++) {
        track = g_SeasonTable[seasonIndex].races[raceIndex][0];
        if (championshipFlags & 1) {
            if (track < 5) {
                track += 5;
            } else {
                track -= 5;
            }
        }
        SetTextAlignLeft();
        if (raceIndex == currentRace && g_fSeasonRecordsBlinkTime < 0.4f) {
            SetTextCustomColors(0xFF, 0xF0, 0x50, 0xFF, 0xF0, 0x50);
        } else {
            SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);
        }
        sprintf(g_szTrackSelectRecordTextBuf, "Race %d", raceIndex + 1);
        DrawText(g_szTrackSelectRecordTextBuf, g_dwScreenWidth * 4 / 32 - shift, y);
#if TGR_REGION == TGR_REGION_JP
        sprintf(g_szTrackSelectRecordTextBuf,
                "%%j%c",
                g_SeasonTable[seasonIndex].races[raceIndex][1] + 0x29);
        SetTextSize(13);
        DrawText(g_szTrackSelectRecordTextBuf, g_dwScreenWidth * 8 / 32 - shift + 4, y + 1);
        SetTextSize(10);
        DrawText(g_apTrackMenuItems[track]->name, g_dwScreenWidth * 8 / 32 - shift + 16, y);
#else
        sprintf(g_szTrackSelectRecordTextBuf,
                "%s/%s",
                g_apTrackMenuItems[track]->name,
                g_apWeatherMenuItems[g_SeasonTable[seasonIndex].races[raceIndex][1]]->name);
        DrawText(g_szTrackSelectRecordTextBuf, g_dwScreenWidth * 8 / 32 - shift + 4, y);
#endif
        SetTextAlignRight();
        if (g_aVehicles[0].profile->raceRecords[track] != 0) {
            FormatLapTime(g_szTrackSelectRecordTextBuf, g_aVehicles[0].profile->raceRecords[track]);
        } else {
            sprintf(g_szTrackSelectRecordTextBuf, "--    ");
        }
        DrawText(g_szTrackSelectRecordTextBuf, g_dwScreenWidth * 23 / 32 + shift - 1, y);
        if (g_aVehicles[0].profile->lapRecords[track] != 0) {
            FormatLapTime(g_szTrackSelectRecordTextBuf, g_aVehicles[0].profile->lapRecords[track]);
        } else {
            sprintf(g_szTrackSelectRecordTextBuf, "--    ");
        }
        DrawText(g_szTrackSelectRecordTextBuf, g_dwScreenWidth * 28 / 32 + shift, y);
        y += 10;
    }
    SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);
    SetTextAlignCenter();
    y = 0x94;
    DrawText("Cars For", g_dwScreenWidth >> 1, y);
    y += 10;
    DrawText("This Season:", g_dwScreenWidth >> 1, y);
    g_dwWeatherActive = 0;
    g_dwNightMode = 0;
    g_dwRainMode = 0;
    g_dwSnowMode = 0;
    SetupFog();
    SetupWeatherLights();
    g_aViewportRects[0].vehicleIndex = 0;
    g_aViewportRects[1].vehicleIndex = 1;
    RenderCameraView(0,
                     0,
                     0,
                     0,
                     g_dwScreenHeight * 7 / 16,
                     g_dwScreenWidth * 5 / 8,
                     g_dwScreenHeight * 5 / 8,
                     0.0f,
                     1,
                     0,
                     0,
                     0.0f,
                     1.5707964f);
    RenderCameraView(0,
                     0,
                     1,
                     g_dwScreenWidth * 3 / 8,
                     g_dwScreenHeight * 7 / 16,
                     g_dwScreenWidth * 5 / 8,
                     g_dwScreenHeight * 5 / 8,
                     0.0f,
                     1,
                     1,
                     1,
                     0.0f,
                     0.0f);
    SetScissorRect(0, 0, g_dwScreenWidth, g_dwScreenHeight);
}

/* The original sets the named item only in the modes that restore its name at the end. */
void ProcessTrackMenu(void) {
    static s32 menuInitialized;
    static s32 menuCount;
    static s32 menuSelection;
    static s32 menuChampionship;
    static char menuText[80];
    /* IDO reserves this unused buffer in the original stack frame. */
    char unusedFrameBuffer[100] TGR_MATCHING_UNUSED;
    MenuItem **selectedSlot;
    MenuItem *selectedItem;
#ifdef ENABLE_PATCHES
    const char *selectedName = NULL;
    int selectedIndex = -1;
#else
    const char *selectedName;
    int selectedIndex;
#endif
    int i;

    if (menuInitialized <= 0) {
        ResetAndInitDisplay(g_apTrackMenuItems, &menuCount);
        for (i = 0; i < g_dwNumTracks; i++) {
            if (g_dwGameMode == GAME_MODE_ARCADE) {
                g_apTrackMenuItems[i]->flags |= MENU_ITEM_ADJUSTABLE;
            } else {
                g_apTrackMenuItems[i]->flags &= ~MENU_ITEM_ADJUSTABLE;
            }
        }
        PreparePakDataScreen();
        ValidateChampionshipProgress();
        for (i = 0; i < 2; i++) {
            InitVehicleColor(&g_aVehicles[i]);
            g_aVehicles[i].driver = UpdatePhysicsStep;
            InstallVehicleModel(
                &g_aVehicles[i], i, g_SeasonTable[g_aVehicles[0].profile->season].cars[i]);
            g_aVehicles[i].groundInstanceCount = 0;
            g_aVehicles[i].nearbyInstanceCount = 0;
            g_aVehicles[i].frame.forward.x = 1.0f;
            g_aVehicles[i].frame.forward.y = 0.0f;
            g_aVehicles[i].frame.forward.z = 0.0f;
            g_aVehicles[i].frame.side.x = 0.0f;
            g_aVehicles[i].frame.side.y = 1.0f;
            g_aVehicles[i].frame.side.z = 0.0f;
            g_aVehicles[i].frame.up.x = 0.0f;
            g_aVehicles[i].frame.up.y = 0.0f;
            g_aVehicles[i].frame.up.z = 1.0f;
            g_aVehicles[i].frame.position.x = 0.0f;
            g_aVehicles[i].frame.position.y = 0.0f;
            g_aVehicles[i].frame.position.z = 0.254f;
        }
        g_aVehicles[0].cameras[VEHICLE_CAMERA_TRACKSIDE].position.x = 10.0f;
        g_aVehicles[0].cameras[VEHICLE_CAMERA_TRACKSIDE].position.y = 10.0f;
        g_aVehicles[0].cameras[VEHICLE_CAMERA_TRACKSIDE].position.z = 5.0f;
        Vec3fScale(&g_aVehicles[0].cameras[VEHICLE_CAMERA_TRACKSIDE].forward,
                   &g_aVehicles[0].cameras[VEHICLE_CAMERA_TRACKSIDE].position,
                   -1.0f);
        Vec3fNormalize(&g_aVehicles[0].cameras[VEHICLE_CAMERA_TRACKSIDE].forward);
        g_aVehicles[0].cameras[VEHICLE_CAMERA_TRACKSIDE].up.x = 0.0f;
        g_aVehicles[0].cameras[VEHICLE_CAMERA_TRACKSIDE].up.y = 0.0f;
        g_aVehicles[0].cameras[VEHICLE_CAMERA_TRACKSIDE].up.z = 1.0f;
        Vec3fCross(&g_aVehicles[0].cameras[VEHICLE_CAMERA_TRACKSIDE].side,
                   &g_aVehicles[0].cameras[VEHICLE_CAMERA_TRACKSIDE].up,
                   &g_aVehicles[0].cameras[VEHICLE_CAMERA_TRACKSIDE].forward);
        Vec3fCross(&g_aVehicles[0].cameras[VEHICLE_CAMERA_TRACKSIDE].up,
                   &g_aVehicles[0].cameras[VEHICLE_CAMERA_TRACKSIDE].forward,
                   &g_aVehicles[0].cameras[VEHICLE_CAMERA_TRACKSIDE].side);
        memcpy(&g_aVehicles[1].cameras[VEHICLE_CAMERA_TRACKSIDE],
               &g_aVehicles[0].cameras[VEHICLE_CAMERA_TRACKSIDE],
               sizeof(CameraFrame));
        g_dwVehicleViewModeIndex = 0;
        menuSelection = g_dwSelectedTrackId;
        while (!IsTrackUnlocked(menuSelection)) {
            menuSelection = (menuSelection + 1) % g_dwNumTracks;
        }
        g_nScreenLastActiveIndex = menuSelection;
        g_pfnScreenPostDrawCallback = TrackMenuPostDraw;
        menuChampionship = g_dwGameMode == GAME_MODE_CHAMPIONSHIP;
        g_bSeasonFirstRace = menuChampionship && g_dwContinueFromResults == 0;
        g_dwContinueFromResults = 0;
        if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP) {
            g_apTrackMenuItems[g_dwNumTracks]->name = "Load Season Data";
            g_apTrackMenuItems[g_dwNumTracks + 1]->name = "Reset To First Round";
            g_apTrackMenuItems[g_dwNumTracks + 2]->name = "Reset To First Season";
            g_apTrackMenuItems[g_dwNumTracks + 3]->name = "Reset To First Year";
        } else {
            g_apTrackMenuItems[g_dwNumTracks]->name = "Load Ghost Car Data";
            g_apTrackMenuItems[g_dwNumTracks + 2]->name = "Reset Ghost Car";
        }
        menuInitialized = 1;
    }
    if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP) {
        selectedIndex = g_dwSelectedTrackId;
        selectedSlot = &g_apTrackMenuItems[selectedIndex];
        selectedItem = *selectedSlot;
        selectedName = (*selectedSlot)->name;
        if (g_bSeasonFirstRace) {
            g_pfnScreenPreDrawCallback = SeasonRecordsPreDrawCallback;
            selectedItem->flags |= MENU_ITEM_IMMEDIATE;
        } else {
            g_pfnScreenPreDrawCallback = NULL;
#if TGR_REGION == TGR_REGION_JP
            sprintf(menuText,
                    "Race %d: %s %%j%c%%e",
                    g_aVehicles[0].profile->round + 1,
                    selectedName,
                    g_dwCurrentWeather + FIRST_WEATHER_GLYPH);
#else
            sprintf(menuText,
                    "Race %d: %s/%s",
                    g_aVehicles[0].profile->round + 1,
                    selectedName,
                    g_apWeatherMenuItems[g_dwCurrentWeather]->name);
#endif
            (*selectedSlot)->name = menuText;
        }
        i = ProcessScreenState(g_SeasonTable[g_aVehicles[0].profile->season].name,
                               menuCount,
                               g_apTrackMenuItems,
                               &menuSelection,
                               IsTrackUnlocked,
                               0,
                               0,
                               0,
                               0,
                               0x78,
                               0x82);
        if (g_bSeasonFirstRace) {
            (*selectedSlot)->flags &= ~MENU_ITEM_IMMEDIATE;
        }
    } else {
        if (g_dwGameMode == GAME_MODE_ARCADE) {
            selectedIndex = g_nScreenLastActiveIndex;
            selectedSlot = &g_apTrackMenuItems[selectedIndex];
            selectedName = (*selectedSlot)->name;
            sprintf(menuText, "%s: %d laps", selectedName, g_dwTotalLaps);
            (*selectedSlot)->name = menuText;
        }
#if TGR_REGION == TGR_REGION_JP
        i = ProcessScreenState("COURSE SELECT",
                               menuCount,
                               g_apTrackMenuItems,
                               &menuSelection,
                               IsTrackUnlocked,
                               0,
                               0,
                               0,
                               0x80,
                               0,
                               0x80);
#else
        i = ProcessScreenState("TRACK SELECT",
                               menuCount,
                               g_apTrackMenuItems,
                               &menuSelection,
                               IsTrackUnlocked,
                               0,
                               0,
                               0,
                               0x80,
                               0,
                               0x80);
#endif
    }
    switch (i) {
    case MENU_RESULT_UP:
        if (g_dwGameMode == GAME_MODE_ARCADE && g_dwTotalLaps < MAX_LAPS) {
            g_dwTotalLaps++;
        }
        break;
    case MENU_RESULT_DOWN:
        if (g_dwGameMode == GAME_MODE_ARCADE && g_dwTotalLaps > MIN_LAPS) {
            g_dwTotalLaps--;
        }
        break;
    case MENU_RESULT_CHOSEN:
        if (menuSelection == g_dwNumTracks) {
            g_bTrackMenuPakScreen = TRUE;
        } else if (menuSelection == g_dwNumTracks + 1) {
            if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP) {
                goto reset_race;
            }
        } else if (menuSelection == g_dwNumTracks + 2) {
            if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP) {
                goto reset_round;
            }
            if (g_dwGameMode == GAME_MODE_TIME_ATTACK) {
                g_dwGhostRecordingSize = 8;
                StartTimeAttack();
                SetScreenPendingSelection(menuSelection = g_dwSelectedTrackId);
            }
        } else if (menuSelection == g_dwNumTracks + 3) {
            if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP) {
                g_aVehicles[0].profile->championshipFlags = 0;
            reset_round:
                g_aVehicles[0].profile->season = 0;
            reset_race:
                g_aVehicles[0].profile->round = 0;
                g_aVehicles[0].profile->seasonPoints[g_aVehicles[0].profile->season] = 0;
                StartChampionshipRace();
                SetScreenPendingSelection(menuSelection = g_dwSelectedTrackId);
                for (i = 0; i < 2; i++) {
                    LoadVehicleModel(&g_aVehicleModelSlots[i],
                                     i,
                                     g_SeasonTable[g_aVehicles[0].profile->season].cars[i],
                                     i);
                    while (PollVehicleModelDma(&g_aVehicleModelSlots[i])) {
                    }
                }
                g_bSeasonFirstRace = 1;
            }
        } else if (g_bSeasonFirstRace) {
            g_bSeasonFirstRace = 0;
        } else {
            g_dwSelectedTrackId = menuSelection;
            if (g_dwGameMode == GAME_MODE_TIME_ATTACK || g_dwGameMode == GAME_MODE_CHAMPIONSHIP) {
                SetCurrentScreenCallback(ProcessCarSelectScreen);
            } else {
                SetCurrentScreenCallback(ProcessWeatherSelect);
            }
            menuInitialized = -1;
        }
        break;
    case MENU_RESULT_CANCELLED:
        if (g_dwGameMode == GAME_MODE_TIME_ATTACK || g_dwGameMode == GAME_MODE_CHAMPIONSHIP) {
            SetCurrentScreenCallback(ProcessMainMenu);
        } else {
            SetCurrentScreenCallback(ProcessPlayerCountSelect);
        }
        menuInitialized = 0;
        break;
    }
#ifdef ENABLE_PATCHES
    if (selectedIndex >= 0) {
#else
    if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP || g_dwGameMode == GAME_MODE_ARCADE) {
#endif
        g_apTrackMenuItems[selectedIndex]->name = selectedName;
    }
}

void DrawRotatedModel(Model *model, f32 x, f32 y, f32 z, f32 angle) {
    if (g_dwTrackMirrored != g_dwViewportMirrored) {
        guRotateF(g_afMvpMatrix, -angle * RAD_TO_DEG, 0.0f, 0.0f, 1.0f);
        guScaleF(g_afScratchMatrix, -STAGE_MODEL_SCALE, STAGE_MODEL_SCALE, STAGE_MODEL_SCALE);
    } else {
        guRotateF(g_afMvpMatrix, angle * RAD_TO_DEG, 0.0f, 0.0f, 1.0f);
        guScaleF(g_afScratchMatrix, STAGE_MODEL_SCALE, STAGE_MODEL_SCALE, STAGE_MODEL_SCALE);
    }
    guMtxCatF(g_afMvpMatrix, g_afScratchMatrix, g_afMvpMatrix);
    guRotateF(g_afScratchMatrix, -90.0f, 1.0f, 0.0f, 0.0f);
    guMtxCatF(g_afMvpMatrix, g_afScratchMatrix, g_afMvpMatrix);
    guTranslateF(g_afScratchMatrix, x, y, z);
    guMtxCatF(g_afMvpMatrix, g_afScratchMatrix, g_afMvpMatrix);
    DrawModelList(model, g_afMvpMatrix);
}

void DrawModelAt(Model *model, f32 x, f32 y, f32 z, f32 angle, f32 scale) {
    if (g_dwTrackMirrored != g_dwViewportMirrored) {
        guRotateF(g_afMvpMatrix, -angle * RAD_TO_DEG, 0.0f, 0.0f, 1.0f);
        guScaleF(g_afScratchMatrix, -scale, scale, scale);
    } else {
        guRotateF(g_afMvpMatrix, angle * RAD_TO_DEG, 0.0f, 0.0f, 1.0f);
        guScaleF(g_afScratchMatrix, scale, scale, scale);
    }
    guMtxCatF(g_afMvpMatrix, g_afScratchMatrix, g_afMvpMatrix);
    guRotateF(g_afScratchMatrix, -90.0f, 1.0f, 0.0f, 0.0f);
    guMtxCatF(g_afMvpMatrix, g_afScratchMatrix, g_afMvpMatrix);
    guTranslateF(g_afScratchMatrix, x, y, z);
    guMtxCatF(g_afMvpMatrix, g_afScratchMatrix, g_afMvpMatrix);
    DrawModelList(model, g_afMvpMatrix);
}

void DrawMenuSpotlight(void) {
    f32 angle;
    s32 i;

    /* Nothing sets the flag, so the vertices are built again every frame. */
    if (!g_bMenuSpotlightBuilt) {
        /* A ring under the stage, then its source above it in the middle. */
        for (i = 0; i < SPOTLIGHT_POINTS; i++) {
            angle = (f32)i * 0.2094395161f;
            g_aMenuSpotlightVertices[i].v.ob[0] =
                (s16)(s32)(sinf(angle) * SPOTLIGHT_RADIUS) + g_dwScreenWidth / 2;
            g_aMenuSpotlightVertices[i].v.ob[1] = g_dwScreenHeight * 6 / 16;
            g_aMenuSpotlightVertices[i].v.ob[2] = (s32)(cosf(angle) * SPOTLIGHT_RADIUS);
            g_aMenuSpotlightVertices[i].v.cn[0] = 0xFF;
            g_aMenuSpotlightVertices[i].v.cn[1] = 0xFF;
            g_aMenuSpotlightVertices[i].v.cn[2] = 0xFF;
            g_aMenuSpotlightVertices[i].v.cn[3] = 0xFF;
        }
        g_aMenuSpotlightVertices[0].v.ob[0] = g_dwScreenWidth / 2;
        g_aMenuSpotlightVertices[0].v.ob[1] = g_dwScreenHeight * 5 / 16;
        g_aMenuSpotlightVertices[0].v.ob[2] = 0;
    }
    gSPDisplayList(gDisplayListPtr++, g_aMenuSpotlightDisplayList);
}

void DrawMenuChoiceMarker(void) {
    guScaleF(g_afMvpMatrix, 0.26f, 0.26f, 0.26f);
    guRotateF(g_afScratchMatrix, -90.0f, 1.0f, 0.0f, 0.0f);
    guMtxCatF(g_afMvpMatrix, g_afScratchMatrix, g_afMvpMatrix);
    guTranslateF(g_afScratchMatrix, 160.0f, (f32)(g_dwScreenHeight * 6 / 16 - 4), 0.0f);
    guMtxCatF(g_afMvpMatrix, g_afScratchMatrix, g_afMvpMatrix);
    guMtxF2L(g_afMvpMatrix, &g_MenuChoiceMarkerMtx);
    gSPDisplayList(gDisplayListPtr++, g_aMenuChoiceMarkerDisplayList);
}

void DrawMenuFooterButtons(void) {
    DRAW_FOOTER_BUTTONS(100, 160, "%wwSelect", 115, "%wwGo Back", 175);
}

void DrawPauseMenuButtons(void) {
    DRAW_FOOTER_BUTTONS(102, 178, "%wwContinue", 118, "%wwExit", 193);
}

void SetScreenPendingSelection(s32 item) {
    g_nScreenPendingSelection = item;
}

/*
 * Load the textures and the sounds of the screens, and silence the end of each sound, counting
 * with i.
 */
s32 ProcessScreenState(const char *title,
                       s32 itemCount,
                       MenuItem **items,
                       s32 *selection,
                       MenuItemFilter isSelectable,
                       s32 topRed,
                       s32 topGreen,
                       s32 topBlue,
                       s32 bottomRed,
                       s32 bottomGreen,
                       s32 bottomBlue) {
#ifdef __sgi
    /* IDO places local menu state at the original BSS addresses. */
    static int g_nPrevScreenSelection;
    static float g_fScreenSlideOffset;
    static float g_fScreenRotationAngle;
    static int g_bScreenItemChosen;
    static s8 *g_pScreenScrollSoundBuf;
    static int g_dwScreenScrollSoundSize;
    static s8 *g_pScreenSelectSoundBuf;
    static int g_dwScreenSelectSoundSize;
    static float g_fScreenIdleTime;
    static int g_nScreenCursor;
#endif
    float off;
    int i;
    int result;

    result = 0;
    if (g_bScreenStateInitFlag == 0) {
        SetFadeTarget(1.0f, 0.2f);
        FadeMusicTo(1.0f, 0.2f);
        FadeSfxTo(1.0f, 0.2f);
        g_nScreenCursor = *selection;
        g_fScreenRotationAngle = 4.712389f;
        g_fScreenSlideOffset = 0.0f;
        LoadAndDecompressAsset(&g_ArrowLeftTextureDesc, AllocHeapAligned);
        LoadAndDecompressAsset(&g_ArrowRightTextureDesc, AllocHeapAligned);
        LoadAndDecompressAsset(&g_BgTextureDesc, AllocHeapAligned);
        g_dwScreenScrollSoundSize = DmaReadAssetSize(ROM_SCROLL_SOUND);
        g_pScreenScrollSoundBuf =
            AllocHeapAligned(g_dwScreenScrollSoundSize + g_dwScreenScrollSoundSize);
        DecompressFromRom(g_pScreenScrollSoundBuf, ROM_SCROLL_SOUND, 0);
        g_dwScreenSelectSoundSize = DmaReadAssetSize(ROM_SELECT_SOUND);
        g_pScreenSelectSoundBuf =
            AllocHeapAligned(g_dwScreenSelectSoundSize + g_dwScreenSelectSoundSize);
        DecompressFromRom(g_pScreenSelectSoundBuf, ROM_SELECT_SOUND, 0);
        for (i = 0; i != 0x100; i++) {
            g_pScreenScrollSoundBuf[g_dwScreenScrollSoundSize + i] = 0;
            g_pScreenSelectSoundBuf[g_dwScreenSelectSoundSize + i] = 0;
        }
        if (g_pMenuTitleImage != 0) {
            LoadAndDecompressAsset(g_pMenuTitleImage, AllocHeapAligned);
        }
        g_fScreenIdleTime = 0.0f;
        g_nScreenPendingSelection = -1;
        g_bScreenStateInitFlag = 1;
    }
    if (g_nScreenPendingSelection >= 0) {
        short v;

        g_nPrevScreenSelection = g_nScreenCursor;
        g_nScreenCursor = g_nScreenPendingSelection;
        if (g_nScreenCursor != g_nPrevScreenSelection) {
            g_fScreenSlideOffset = -1.0f;
            v = FindFreeSoundChannel();
            if (v != -1) {
                InitSoundChannel(v, g_pScreenScrollSoundBuf, g_dwScreenScrollSoundSize, 0);
            }
        }
        g_nScreenPendingSelection = -1;
    }
    UpdateFrameTiming();
    UpdateFadeTransitions();
    StartHighResFrame();
    AddTimingMarker(0, 0, 0, 200, 0xFF);
    ClearDepthBuffer();
    DrawTexturedRect(&g_BgTextureDesc,
                     0,
                     0,
                     320,
                     240,
                     topRed,
                     topGreen,
                     topBlue,
                     0xFF,
                     bottomRed,
                     bottomGreen,
                     bottomBlue,
                     0xFF,
                     4);
    g_fScreenRotationAngle += g_fFrameDeltaTime + g_fFrameDeltaTime;
    while (g_fScreenRotationAngle > 6.2831855f) {
        g_fScreenRotationAngle -= 6.2831855f;
    }
    if (g_fScreenSlideOffset < 0.0f) {
        g_fScreenSlideOffset += g_fFrameDeltaTime + g_fFrameDeltaTime;
        if (g_fScreenSlideOffset > 0.0f) {
            g_fScreenSlideOffset = 0.0f;
        }
    } else if (g_fScreenSlideOffset > 0.0f) {
        g_fScreenSlideOffset -= g_fFrameDeltaTime + g_fFrameDeltaTime;
        if (g_fScreenSlideOffset < 0.0f) {
            g_fScreenSlideOffset = 0.0f;
        }
    }
    if (g_fScreenSlideOffset < 0.0f) {
        off = g_fScreenSlideOffset * g_fScreenSlideOffset * -5.0f;
    } else {
        off = g_fScreenSlideOffset * g_fScreenSlideOffset * 5.0f;
    }
    SetupMenuProjection(g_dwScreenWidth, g_dwScreenHeight);
    SetViewport(0, 0, g_dwScreenWidth, g_dwScreenHeight, 1);
    gSPNumLights(gDisplayListPtr++, 1);
    gSPLight(gDisplayListPtr++, &g_MenuDirectionalLight, 1);
    gSPLight(gDisplayListPtr++, &g_MenuAmbientLight, 2);
    gSPClipRatio(gDisplayListPtr++, 6);
    if (g_pMenuTitleImage != 0) {
        DrawTexturedRect(g_pMenuTitleImage,
                         g_dwMenuTitleImageX,
                         g_dwMenuTitleImageY,
                         g_dwMenuTitleImageWidth,
                         g_dwMenuTitleImageHeight,
                         0,
                         0,
                         0,
                         0xFF,
                         0xFF,
                         0xFF,
                         0xFF,
                         0xFF,
                         2);
    }
    {
        char buf[80];
        short v;

        SetTextSize(30);
        SetTextAlignCenter();
        sprintf(buf, "%%ry%s", title);
        DrawText(buf, g_dwScreenWidth / 2, g_dwScreenHeight / 6 - 2);
        if (g_pfnScreenPreDrawCallback != 0) {
            g_pfnScreenPreDrawCallback();
        } else {
            Mtx *mtx = (Mtx *)AllocDisplayListBlock();
            int row;

            guRotateF(g_afMvpMatrix, 12.0f, 1.0f, 0.0f, 0.0f);
            guMtxF2L(g_afMvpMatrix, mtx);
            gSPMatrix(gDisplayListPtr++, mtx, G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
            DrawMenuSpotlight();
            row = g_nScreenCursor;
            do {
                row = (row + itemCount - 1) % itemCount;
            } while (isSelectable != 0 && isSelectable(row) == 0);
            if (row != g_nScreenCursor) {
                DrawMenuChoiceMarker();
            }
            if (items[g_nScreenCursor]->flags & 4) {
                int shown;

                if (g_fScreenSlideOffset > 0.75f || g_fScreenSlideOffset < -0.75f) {
                    shown = g_nPrevScreenSelection;
                } else {
                    shown = g_nScreenCursor;
                }
                AdvanceModelAnimations(items[shown]->model);
                if (items[shown]->flags & 0x10) {
                    g_dwViewportMirrored = 1;
                }
                DrawRotatedModel(items[shown]->model,
                                 g_dwScreenWidth / 2,
                                 (g_dwScreenHeight * 6) / 16,
                                 0.0f,
                                 -1.5707964f - g_fScreenSlideOffset * 6.2831855f);
                g_dwViewportMirrored = 0;
                if (g_fScreenSlideOffset == 0) {
                    goto label;
                }
            } else if (off != 0) {
                if (items[g_nScreenCursor]->model != 0) {
                    AdvanceModelAnimations(items[g_nScreenCursor]->model);
                    if (items[g_nScreenCursor]->flags & 0x10) {
                        g_dwViewportMirrored = 1;
                    }
                    DrawRotatedModel(items[g_nScreenCursor]->model,
                                     g_dwScreenWidth / 2 - off * g_dwScreenWidth / 6.0f,
                                     (g_dwScreenHeight * 6) / 16,
                                     0.0f,
                                     g_fScreenRotationAngle);
                    g_dwViewportMirrored = 0;
                } else {
                    sprintf(buf, "%%ww%s", items[g_nScreenCursor]->name);
                    SetTextStyleNormal();
                    SetTextAlignCenter();
                    DrawText(buf,
                             g_dwScreenWidth / 2 - (int)(off * g_dwScreenWidth / 6.0f),
                             (g_dwScreenHeight * 9) / 16);
                }
                if (items[g_nPrevScreenSelection]->model != 0) {
                    AdvanceModelAnimations(items[g_nPrevScreenSelection]->model);
                    if (items[g_nPrevScreenSelection]->flags & 0x10) {
                        g_dwViewportMirrored = 1;
                    }
                    DrawRotatedModel(items[g_nPrevScreenSelection]->model,
                                     g_dwScreenWidth / 2 - ((off < 0.0f ? 5.0f : -5.0f) + off) *
                                                               g_dwScreenWidth / 6.0f,
                                     (g_dwScreenHeight * 6) / 16,
                                     0.0f,
                                     g_fScreenRotationAngle);
                    g_dwViewportMirrored = 0;
                } else {
                    sprintf(buf, "%%ww%s", items[g_nPrevScreenSelection]->name);
                    SetTextStyleNormal();
                    SetTextAlignCenter();
                    DrawText(buf,
                             g_dwScreenWidth / 2 - (int)(((off < 0.0f ? 5.0f : -5.0f) + off) *
                                                         g_dwScreenWidth / 6.0f),
                             (g_dwScreenHeight * 9) / 16);
                }
            } else {
                SetTextStyleNormal();
                SetTextAlignCenter();
                if (items[g_nScreenCursor]->model != 0) {
                    AdvanceModelAnimations(items[g_nScreenCursor]->model);
                    if (items[g_nScreenCursor]->flags & 0x10) {
                        g_dwViewportMirrored = 1;
                    }
                    DrawRotatedModel(items[g_nScreenCursor]->model,
                                     g_dwScreenWidth / 2,
                                     (g_dwScreenHeight * 6) / 16,
                                     0.0f,
                                     g_fScreenRotationAngle);
                    g_dwViewportMirrored = 0;
                } else {
                    sprintf(buf, "%%ww%s", items[g_nScreenCursor]->name);
                    DrawText(buf, g_dwScreenWidth / 2, (g_dwScreenHeight * 9) / 16);
                }
            label:
                SetTextSize(20);
                sprintf(buf, "%%ry%s", items[g_nScreenCursor]->name);
                DrawText(buf, g_dwScreenWidth / 2, (g_dwScreenHeight * 20) / 64);
                if (g_pfnScreenPostDrawCallback != 0) {
                    g_pfnScreenPostDrawCallback(g_nScreenCursor);
                }
            }
            gSPPopMatrix(gDisplayListPtr++, 0);
        }
        DrawMenuFooterButtons();
        ApplyScreenFade();
        if (g_dwHudDebugLevel >= 2) {
            SetViewport(0, 0, g_dwScreenWidth, g_dwScreenHeight, 1);
            DrawTimingBars();
        }
        FlushDisplayList();
        if (IsFadingOut() != 0) {
            if (IsFadeComplete() != 0) {
                int ch;

                g_bScreenStateInitFlag = 0;
                *selection = g_nScreenCursor;
                ClearFramebuffersForWideMode();
                for (ch = 0; ch < 6; ch++) {
                    g_aSoundEffectChannels[ch].volume = 0;
                    g_aSoundEffectChannels[ch].mixVolume = 0;
                    g_aSoundEffectChannels[ch].step = 0;
                    g_aSoundEffectChannels[ch].sample = g_SilenceSampleData;
                }
                if (g_bScreenItemChosen != 0) {
                    return 1;
                }
                return 2;
            }
        } else {
            for (i = 0; i < (s32)g_dwNumLocalPlayers; i++) {
                if (g_dwScreenPlayerIdx != -1 && ((1 << i) & g_dwMenuInputPlayerMask) == 0) {
                    continue;
                }
                UpdateStickRepeat(g_aVehicles[i].input);
                if (g_aVehicles[i].input->buttons != 0) {
                    g_fScreenIdleTime = 0.0f;
                }
                if (g_fScreenSlideOffset == 0) {
                    if (g_aVehicles[i].input->buttons & 4) {
                        TransferFlags(g_aVehicles[i].input, 10);
                        g_aVehicles[i].input->buttons &= ~10;
                        g_nPrevScreenSelection = g_nScreenCursor;
                        do {
                            g_nScreenCursor = (g_nScreenCursor + itemCount - 1) % itemCount;
                        } while (isSelectable != 0 && isSelectable(g_nScreenCursor) == 0);
                        if (g_nScreenCursor != g_nPrevScreenSelection) {
                            g_fScreenSlideOffset = 1.0f;
                            goto move;
                        }
                    }
                    if (g_aVehicles[i].input->buttons & 1) {
                        TransferFlags(g_aVehicles[i].input, 10);
                        g_aVehicles[i].input->buttons &= ~10;
                        g_nPrevScreenSelection = g_nScreenCursor;
                        do {
                            g_nScreenCursor = (g_nScreenCursor + 1) % itemCount;
                        } while (isSelectable != 0 && isSelectable(g_nScreenCursor) == 0);
                        if (g_nScreenCursor != g_nPrevScreenSelection) {
                            g_fScreenSlideOffset = -1.0f;
                        move:
                            v = FindFreeSoundChannel();
                            if (v != -1) {
                                InitSoundChannel(
                                    v, g_pScreenScrollSoundBuf, g_dwScreenScrollSoundSize, 0);
                            }
                        }
                    }
                    if (items[g_nScreenCursor]->flags & 1) {
                        if (g_aVehicles[i].input->buttons & 8) {
                            *selection = g_nScreenCursor;
                            TransferFlags(g_aVehicles[i].input, 8);
                            result = 3;
                            goto choose;
                        }
                        if (g_aVehicles[i].input->buttons & 2) {
                            *selection = g_nScreenCursor;
                            TransferFlags(g_aVehicles[i].input, 2);
                            result = 4;
                            goto choose;
                        }
                    }
                    if (g_aVehicles[i].input->buttons & 0x20) {
                        TransferFlags(g_aVehicles[i].input, 0x20);
                        g_bScreenItemChosen = 0;
                        SetFadeTarget(0.0f, 0.2f);
                        FadeSfxTo(0.0f, 0.2f);
                        goto choose;
                    }
                }
                if (!(items[g_nScreenCursor]->flags & 8) &&
                    !(items[g_nScreenCursor]->flags & 0x40) &&
                    (g_aVehicles[i].input->buttons & 0xC010)) {
                    TransferFlags(g_aVehicles[i].input, 0xC010);
                    if (i == 0 || !(items[g_nScreenCursor]->flags & 0x20)) {
                        g_dwMenuChoosingPlayer = i;
                        if ((items[g_nScreenCursor]->flags & 2) == 0) {
                            g_bScreenItemChosen = 1;
                            FadeSfxTo(0.0f, 0.2f);
                            SetFadeTarget(0.0f, 0.2f);
                        } else {
                            *selection = g_nScreenCursor;
                            result = 1;
                        }
                    choose:
                        v = FindFreeSoundChannel();
                        if (v != -1) {
                            InitSoundChannel(
                                v, g_pScreenSelectSoundBuf, g_dwScreenSelectSoundSize, 0);
                        }
                    }
                }
            }
        }
        if (g_fScreenIdleTimeout != 0 && result == 0) {
            g_fScreenIdleTime = g_fScreenIdleTime + g_fFrameDeltaTime;
            if (g_fScreenIdleTimeout <= g_fScreenIdleTime && IsFadingOut() == 0) {
                g_bScreenItemChosen = 0;
                SetFadeTarget(0.0f, 0.2f);
                FadeSfxTo(0.0f, 0.2f);
            }
        }
    }
    g_nScreenLastActiveIndex = g_nScreenCursor;
    return result;
}
void LoadMenuItemModels(MenuItem **items, s32 *itemCount) {
    s32 i;

    /* Two frames of black in the mode of the screen. */
    g_dwKeepScreenBlanked = TRUE;
    if (g_dwScreenScaleShift) {
        BLANK_SCREEN();
        StartHighResFrame();
        FlushDisplayList();
        BLANK_SCREEN();
        StartHighResFrame();
        FlushDisplayList();
    } else {
        BLANK_SCREEN();
        StartLowResFrame();
        FlushDisplayList();
        BLANK_SCREEN();
        StartLowResFrame();
        FlushDisplayList();
    }
    g_dwKeepScreenBlanked = FALSE;

    /* Items with the same model on the cartridge share it. */
    for (*itemCount = 0; items[*itemCount] != NULL; (*itemCount)++) {
        if (items[*itemCount]->romStart == 0) {
            continue;
        }
        for (i = 0; i < *itemCount; i++) {
            if (items[*itemCount]->romStart == items[i]->romStart) {
                items[*itemCount]->model = items[i]->model;
                break;
            }
        }
        if (i == *itemCount) {
            items[*itemCount]->model =
                AllocHeapAligned(DmaReadAssetSize(ROM_MODEL_SOURCE(items[*itemCount]->romStart)));
            DecompressModel(items[*itemCount]->model,
                            ROM_MODEL_SOURCE(items[*itemCount]->romStart));
        }
    }
}

void ResetAndInitDisplay(MenuItem **items, s32 *itemCount) {
    ResetHeap();
    g_fScreenIdleTimeout = 0.0f;
    g_pfnScreenPostDrawCallback = NULL;
    g_pfnScreenPreDrawCallback = NULL;
    LoadMenuItemModels(items, itemCount);
}
