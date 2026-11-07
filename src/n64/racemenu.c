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
#if defined(ENABLE_PATCHES) || !defined(__sgi)
    /* An unsigned season and round are never negative. */
    if (g_aVehicles[0].profile->season >= SEASONS) {
#else
    if (g_aVehicles[0].profile->season < 0 || g_aVehicles[0].profile->season >= SEASONS) {
#endif
        goto resetSeason;
    }
#if defined(ENABLE_PATCHES) || !defined(__sgi)
    if (g_aVehicles[0].profile->round >= g_SeasonTable[g_aVehicles[0].profile->season].raceCount) {
#else
    if (g_aVehicles[0].profile->round < 0 ||
        g_aVehicles[0].profile->round >= g_SeasonTable[g_aVehicles[0].profile->season].raceCount) {
#endif
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
#if defined(ENABLE_PATCHES) || !defined(__sgi)
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
#if defined(ENABLE_PATCHES) || !defined(__sgi)
    if (selectedIndex >= 0) {
#else
    if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP || g_dwGameMode == GAME_MODE_ARCADE) {
#endif
        g_apTrackMenuItems[selectedIndex]->name = selectedName;
    }
}
