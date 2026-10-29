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
#define ROM_SCROLL_SOUND 0x1BF480
#define ROM_SELECT_SOUND 0x1BFEC0

#if TGR_REGION == TGR_REGION_JP
#define TRACK_MENU_TITLE "COURSE SELECT"
#else
#define TRACK_MENU_TITLE "TRACK SELECT"
#endif

/* The silence after each sound, in bytes. */
#define SOUND_TAIL_BYTES 0x100

/* The time a fade of a screen in or out takes, as its rate. */
#define SCREEN_FADE_RATE 0.2f

/* How long the round under way blinks in the records, and how long of that it shows yellow. */
#define BLINK_PERIOD 0.75f
#define BLINK_ON 0.4f

/* How far the items slide before the model under the cursor changes to the next one. */
#define SPIN_SWAP_SLIDE 0.75f

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

/* Play a sound of the screen on a free channel. */
#define PLAY_SCREEN_SOUND(sample, length)                                                          \
    {                                                                                              \
        s16 channel = FindFreeSoundChannel();                                                      \
                                                                                                   \
        if (channel != -1) {                                                                       \
            InitSoundChannel(channel, sample, length, 0);                                          \
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
    Season *season;
    s32 round, seasonIndex, flags, track, race, shift, y, i;

    g_fSeasonRecordsBlinkTime += g_fFrameDeltaTime;
    while (BLINK_PERIOD < g_fSeasonRecordsBlinkTime) {
        g_fSeasonRecordsBlinkTime -= BLINK_PERIOD;
    }
    round = g_aVehicles[0].profile->round;
    seasonIndex = g_aVehicles[0].profile->season;
    flags = g_aVehicles[0].profile->championshipFlags;
    SetTextStyleNormal();
    SetTextAlignCenter();
    SetTextSize(20);
    season = &g_SeasonTable[seasonIndex];
    sprintf(g_szTrackSelectRecordTextBuf, "%%ry%s:", season->subtitle);
    DrawText(g_szTrackSelectRecordTextBuf, g_dwScreenWidth / 2, g_dwScreenHeight * 20 / 64);

    /* The columns move right when a name is of the longer, mirrored tracks. */
    shift = 0;
    for (i = 0; i < season->raceCount; i++) {
        if (season->races[i][0] < MIRRORED_TRACKS) {
            if (flags & 1) {
                shift = g_dwScreenWidth * 3 / 64;
                break;
            }
        } else if (!(flags & 1)) {
            shift = g_dwScreenWidth * 3 / 64;
            break;
        }
    }

    SetTextSize(8);
    SetTextAlignRight();
    DrawText("Race Record", g_dwScreenWidth * 23 / 32 + shift, 86);
    DrawText("Lap Record", g_dwScreenWidth * 28 / 32 + shift + 2, 86);
    SetTextSize(10);
    y = 95;
    for (race = 0; race < season->raceCount; race++, y += 10) {
        track = season->races[race][0];
        if (flags & 1) {
            track = track < MIRRORED_TRACKS ? track + MIRRORED_TRACKS : track - MIRRORED_TRACKS;
        }
        SetTextAlignLeft();
        if (race == round && g_fSeasonRecordsBlinkTime < BLINK_ON) {
            SetTextCustomColors(0xFF, 0xF0, 0x50, 0xFF, 0xF0, 0x50);
        } else {
            SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);
        }
        sprintf(g_szTrackSelectRecordTextBuf, "Race %d", race + 1);
        DrawText(g_szTrackSelectRecordTextBuf, g_dwScreenWidth * 4 / 32 - shift, y);
        sprintf(g_szTrackSelectRecordTextBuf,
                "%s/%s",
                g_apTrackMenuItems[track]->name,
                g_apWeatherMenuItems[season->races[race][1]]->name);
        DrawText(g_szTrackSelectRecordTextBuf, g_dwScreenWidth * 8 / 32 - shift + 4, y);
        SetTextAlignRight();
        if (g_aVehicles[0].profile->raceRecords[track] == 0.0f) {
            sprintf(g_szTrackSelectRecordTextBuf, "--    ");
        } else {
            FormatLapTime(g_szTrackSelectRecordTextBuf, g_aVehicles[0].profile->raceRecords[track]);
        }
        DrawText(g_szTrackSelectRecordTextBuf, g_dwScreenWidth * 23 / 32 + shift - 1, y);
        if (g_aVehicles[0].profile->lapRecords[track] == 0.0f) {
            sprintf(g_szTrackSelectRecordTextBuf, "--    ");
        } else {
            FormatLapTime(g_szTrackSelectRecordTextBuf, g_aVehicles[0].profile->lapRecords[track]);
        }
        DrawText(g_szTrackSelectRecordTextBuf, g_dwScreenWidth * 28 / 32 + shift, y);
    }

    SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);
    SetTextAlignCenter();
    DrawText("Cars For", g_dwScreenWidth >> 1, 148);
    DrawText("This Season:", g_dwScreenWidth >> 1, 158);

    /* The two cars of the season, in clear weather, side by side. */
    g_dwWeatherActive = FALSE;
    g_dwNightMode = FALSE;
    g_dwRainMode = FALSE;
    g_dwSnowMode = FALSE;
    SetupFog();
    SetupWeatherLights();
    g_aViewportRects[0].vehicleIndex = 0;
    g_aViewportRects[1].vehicleIndex = 1;
    RenderCameraView(NULL,
                     NULL,
                     0,
                     0,
                     g_dwScreenHeight * 7 / 16,
                     g_dwScreenWidth * 5 / 8,
                     g_dwScreenHeight * 5 / 8,
                     0.0f,
                     TRUE,
                     0,
                     0,
                     0.0f,
                     1.570796371f);
    RenderCameraView(NULL,
                     NULL,
                     1,
                     g_dwScreenWidth * 3 / 8,
                     g_dwScreenHeight * 7 / 16,
                     g_dwScreenWidth * 5 / 8,
                     g_dwScreenHeight * 5 / 8,
                     0.0f,
                     TRUE,
                     1,
                     1,
                     0.0f,
                     0.0f);
    SetScissorRect(0, 0, g_dwScreenWidth, g_dwScreenHeight);
}

/* The original sets the named item only in the modes that restore its name at the end. */
void ProcessTrackMenu(void) {
    CameraFrame *camera;
    Vehicle *vehicle;
#ifdef ENABLE_PATCHES
    /* No item is named until a mode names one. */
    const char *name = NULL;
    s32 named = -1, result, i, selection;
#else
    const char *name;
    s32 named, result, i, selection;
#endif /* ENABLE_PATCHES */

    if (g_dwTrackMenuInitState <= 0) {
        ResetAndInitDisplay(g_apTrackMenuItems, &g_dwTrackMenuItemCount);
        for (i = 0; i < g_dwNumTracks; i++) {
            if (g_dwGameMode == GAME_MODE_ARCADE) {
                g_apTrackMenuItems[i]->flags |= MENU_ITEM_ADJUSTABLE;
            } else {
                g_apTrackMenuItems[i]->flags &= ~MENU_ITEM_ADJUSTABLE;
            }
        }
        PreparePakDataScreen();
        ValidateChampionshipProgress();

        /* The two cars of the season stand at the origin. */
        for (i = 0, vehicle = g_aVehicles; i < 2; i++, vehicle++) {
            InitVehicleColor(vehicle);
            vehicle->driver = UpdatePhysicsStep;
            InstallVehicleModel(vehicle, i, g_SeasonTable[g_aVehicles[0].profile->season].cars[i]);
            vehicle->groundInstanceCount = 0;
            vehicle->nearbyInstanceCount = 0;
            vehicle->frame.forward.x = 1.0f;
            vehicle->frame.forward.y = 0.0f;
            vehicle->frame.forward.z = 0.0f;
            vehicle->frame.side.x = 0.0f;
            vehicle->frame.side.y = 1.0f;
            vehicle->frame.side.z = 0.0f;
            vehicle->frame.up.x = 0.0f;
            vehicle->frame.up.y = 0.0f;
            vehicle->frame.up.z = 1.0f;
            vehicle->frame.position.x = 0.0f;
            vehicle->frame.position.y = 0.0f;
            vehicle->frame.position.z = 0.254f;
        }

        /* Their trackside camera looks at the origin from above a corner. */
        camera = &g_aVehicles[0].cameras[VEHICLE_CAMERA_TRACKSIDE];
        camera->position.x = 10.0f;
        camera->position.y = 10.0f;
        camera->position.z = 5.0f;
        Vec3fScale(&camera->forward, &camera->position, -1.0f);
        Vec3fNormalize(&camera->forward);
        camera->up.x = 0.0f;
        camera->up.y = 0.0f;
        camera->up.z = 1.0f;
        Vec3fCross(&camera->side, &camera->up, &camera->forward);
        Vec3fCross(&camera->up, &camera->forward, &camera->side);
        memcpy(&g_aVehicles[1].cameras[VEHICLE_CAMERA_TRACKSIDE], camera, sizeof(CameraFrame));

        g_dwVehicleViewModeIndex = 0;
        g_dwTrackMenuSelection = g_dwSelectedTrackId;
        while (!IsTrackUnlocked(g_dwTrackMenuSelection)) {
            g_dwTrackMenuSelection = (g_dwTrackMenuSelection + 1) % g_dwNumTracks;
        }
        g_nScreenLastActiveIndex = g_dwTrackMenuSelection;
        g_pfnScreenPostDrawCallback = TrackMenuPostDraw;
        g_bTrackMenuChampionship = g_dwGameMode == GAME_MODE_CHAMPIONSHIP;
        g_bSeasonFirstRace = g_bTrackMenuChampionship && !g_dwContinueFromResults;
        g_dwContinueFromResults = FALSE;
        if (g_bTrackMenuChampionship) {
            g_apTrackMenuItems[g_dwNumTracks]->name = "Load Season Data";
            g_apTrackMenuItems[g_dwNumTracks + 1]->name = "Reset To First Round";
            g_apTrackMenuItems[g_dwNumTracks + 2]->name = "Reset To First Season";
            g_apTrackMenuItems[g_dwNumTracks + 3]->name = "Reset To First Year";
        } else {
            g_apTrackMenuItems[g_dwNumTracks]->name = "Load Ghost Car Data";
            g_apTrackMenuItems[g_dwNumTracks + 2]->name = "Reset Ghost Car";
        }
        g_dwTrackMenuInitState = 1;
    }

    /* The item of the track of the next race, or of the arcade, names it for the frame. */
    if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP) {
        named = g_dwSelectedTrackId;
        name = g_apTrackMenuItems[named]->name;
        if (g_bSeasonFirstRace) {
            g_pfnScreenPreDrawCallback = SeasonRecordsPreDrawCallback;
            g_apTrackMenuItems[named]->flags |= MENU_ITEM_IMMEDIATE;
        } else {
            g_pfnScreenPreDrawCallback = NULL;
#if TGR_REGION == TGR_REGION_JP
            sprintf(g_szTrackMenuTextBuf,
                    "Race %d: %s %%j%c%%e",
                    g_aVehicles[0].profile->round + 1,
                    name,
                    g_dwCurrentWeather + FIRST_WEATHER_GLYPH);
#else
            sprintf(g_szTrackMenuTextBuf,
                    "Race %d: %s/%s",
                    g_aVehicles[0].profile->round + 1,
                    name,
                    g_apWeatherMenuItems[g_dwCurrentWeather]->name);
#endif /* TGR_REGION == TGR_REGION_JP */
            g_apTrackMenuItems[named]->name = g_szTrackMenuTextBuf;
        }
        result = ProcessScreenState(g_SeasonTable[g_aVehicles[0].profile->season].name,
                                    g_dwTrackMenuItemCount,
                                    g_apTrackMenuItems,
                                    &g_dwTrackMenuSelection,
                                    IsTrackUnlocked,
                                    0x00,
                                    0x00,
                                    0x00,
                                    0x00,
                                    0x78,
                                    0x82);
        if (g_bSeasonFirstRace) {
            g_apTrackMenuItems[named]->flags &= ~MENU_ITEM_IMMEDIATE;
        }
    } else {
        if (g_dwGameMode == GAME_MODE_ARCADE) {
            named = g_nScreenLastActiveIndex;
            name = g_apTrackMenuItems[named]->name;
            sprintf(g_szTrackMenuTextBuf, "%s: %d laps", name, g_dwTotalLaps);
            g_apTrackMenuItems[named]->name = g_szTrackMenuTextBuf;
        }
        result = ProcessScreenState(TRACK_MENU_TITLE,
                                    g_dwTrackMenuItemCount,
                                    g_apTrackMenuItems,
                                    &g_dwTrackMenuSelection,
                                    IsTrackUnlocked,
                                    0x00,
                                    0x00,
                                    0x00,
                                    0x80,
                                    0x00,
                                    0x80);
    }

    switch (result) {
    case MENU_RESULT_CHOSEN:
        selection = g_dwTrackMenuSelection;
        if (selection == g_dwNumTracks) {
            g_bTrackMenuPakScreen = TRUE;
            break;
        }

        /* Reset to the first round, season, or year, each reset including the ones before it. */
        if (selection == g_dwNumTracks + 1) {
            g_dwTrackMenuSelection = selection;
            if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP) {
                goto resetRound;
            }
            break;
        }
        if (selection == g_dwNumTracks + 2) {
            g_dwTrackMenuSelection = selection;
            if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP) {
                goto resetSeason;
            }
            if (g_dwGameMode == GAME_MODE_TIME_ATTACK) {
                /* Reset the ghost car. */
                g_dwGhostRecordingSize = 8;
                StartTimeAttack();
                g_dwTrackMenuSelection = g_dwSelectedTrackId;
                SetScreenPendingSelection(g_dwSelectedTrackId);
            }
            break;
        }
        if (selection == g_dwNumTracks + 3) {
            g_dwTrackMenuSelection = selection;
            if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP) {
                g_aVehicles[0].profile->championshipFlags = 0;
            resetSeason:
                g_aVehicles[0].profile->season = 0;
            resetRound:
                g_aVehicles[0].profile->round = 0;
                g_aVehicles[0].profile->seasonPoints[g_aVehicles[0].profile->season] = 0;
                StartChampionshipRace();
                g_dwTrackMenuSelection = g_dwSelectedTrackId;
                SetScreenPendingSelection(g_dwSelectedTrackId);
                LOAD_SEASON_CARS(g_aVehicles[0].profile, i);
                g_bSeasonFirstRace = TRUE;
            }
            break;
        }
        if (g_bSeasonFirstRace) {
            /* The records shown, the next race is chosen. */
            g_dwTrackMenuSelection = selection;
            g_bSeasonFirstRace = FALSE;
            break;
        }
        g_dwSelectedTrackId = selection;
        g_dwTrackMenuSelection = selection;
        if (g_dwGameMode == GAME_MODE_TIME_ATTACK || g_dwGameMode == GAME_MODE_CHAMPIONSHIP) {
            SetCurrentScreenCallback(ProcessCarSelectScreen);
        } else {
            SetCurrentScreenCallback(ProcessWeatherSelect);
        }
        g_dwTrackMenuInitState = -1;
        break;
    case MENU_RESULT_CANCELLED:
        if (g_dwGameMode == GAME_MODE_TIME_ATTACK || g_dwGameMode == GAME_MODE_CHAMPIONSHIP) {
            SetCurrentScreenCallback(ProcessMainMenu);
        } else {
            SetCurrentScreenCallback(ProcessPlayerCountSelect);
        }
        g_dwTrackMenuInitState = 0;
        break;
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
    }
#ifdef ENABLE_PATCHES
    if (named >= 0) {
#else
    if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP || g_dwGameMode == GAME_MODE_ARCADE) {
#endif /* ENABLE_PATCHES */
        g_apTrackMenuItems[named]->name = name;
    }
}

void DrawRotatedModel(Model *model, f32 x, f32 y, f32 z, f32 angle) {
    if (g_dwViewportMirrored != g_dwTrackMirrored) {
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
    if (g_dwViewportMirrored != g_dwTrackMirrored) {
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
#define LOAD_SCREEN_ASSETS(i)                                                                      \
    LoadAndDecompressAsset(&g_ArrowLeftTextureDesc, AllocHeapAligned);                             \
    LoadAndDecompressAsset(&g_ArrowRightTextureDesc, AllocHeapAligned);                            \
    LoadAndDecompressAsset(&g_BgTextureDesc, AllocHeapAligned);                                    \
    g_dwScreenScrollSoundSize = DmaReadAssetSize(ROM_SCROLL_SOUND);                                \
    g_pScreenScrollSoundBuf =                                                                      \
        AllocHeapAligned(g_dwScreenScrollSoundSize + g_dwScreenScrollSoundSize);                   \
    DecompressFromRom(g_pScreenScrollSoundBuf, ROM_SCROLL_SOUND, NULL);                            \
    g_dwScreenSelectSoundSize = DmaReadAssetSize(ROM_SELECT_SOUND);                                \
    g_pScreenSelectSoundBuf =                                                                      \
        AllocHeapAligned(g_dwScreenSelectSoundSize + g_dwScreenSelectSoundSize);                   \
    DecompressFromRom(g_pScreenSelectSoundBuf, ROM_SELECT_SOUND, NULL);                            \
    for (i = 0; i < SOUND_TAIL_BYTES; i++) {                                                       \
        g_pScreenScrollSoundBuf[g_dwScreenScrollSoundSize + i] = 0;                                \
        g_pScreenSelectSoundBuf[g_dwScreenSelectSoundSize + i] = 0;                                \
    }

/*
 * Draw an item that slides with the cursor: its model turning, or its name. The item is an
 * expression the original indexes afresh after each call.
 */
#define DRAW_SLIDING_ITEM(item, offset, text)                                                      \
    if ((item)->model != NULL) {                                                                   \
        AdvanceModelAnimations((item)->model);                                                     \
        if ((item)->flags & MENU_ITEM_MIRRORED) {                                                  \
            g_dwViewportMirrored = TRUE;                                                           \
        }                                                                                          \
        DrawRotatedModel((item)->model,                                                            \
                         (f32)(g_dwScreenWidth / 2) - (offset) * (f32)g_dwScreenWidth / 6.0f,      \
                         (f32)(g_dwScreenHeight * 6 / 16),                                         \
                         0.0f,                                                                     \
                         g_fScreenRotationAngle);                                                  \
        g_dwViewportMirrored = FALSE;                                                              \
    } else {                                                                                       \
        sprintf(text, "%%ww%s", (item)->name);                                                     \
        SetTextStyleNormal();                                                                      \
        SetTextAlignCenter();                                                                      \
        DrawText(text,                                                                             \
                 g_dwScreenWidth / 2 - (s32)((offset) * (f32)g_dwScreenWidth / 6.0f),              \
                 g_dwScreenHeight * 9 / 16);                                                       \
    }

/* Fade the screen and the sound effects out. */
#define FADE_SCREEN_OUT()                                                                          \
    SetFadeTarget(0.0f, SCREEN_FADE_RATE);                                                         \
    FadeSfxTo(0.0f, SCREEN_FADE_RATE)

/* The input of a player, which the original reads from the vehicle at every use. */
#define PLAYER_INPUT(player) (g_aVehicles[player].input)

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
    char text[112];
    MenuItem *item;
    Mtx *block;
    f32 step, slide, ease;
    s32 result, shown, other, player, i;
    u32 buttons;

    result = 0;
    if (!g_bScreenStateInitFlag) {
        SetFadeTarget(1.0f, SCREEN_FADE_RATE);
        FadeMusicTo(1.0f, SCREEN_FADE_RATE);
        FadeSfxTo(1.0f, SCREEN_FADE_RATE);
        g_nScreenCursor = *selection;
        g_fScreenRotationAngle = 4.712388992f;
        g_fScreenSlideOffset = 0.0f;
        LOAD_SCREEN_ASSETS(i);
        if (g_pMenuTitleImage != NULL) {
            LoadAndDecompressAsset(g_pMenuTitleImage, AllocHeapAligned);
        }
        g_fScreenIdleTime = 0.0f;
        g_nScreenPendingSelection = -1;
        g_bScreenStateInitFlag = TRUE;
    }
    if (g_nScreenPendingSelection >= 0) {
        g_nPrevScreenSelection = g_nScreenCursor;
        g_nScreenCursor = g_nScreenPendingSelection;
        if (g_nScreenPendingSelection != g_nPrevScreenSelection) {
            g_fScreenSlideOffset = -1.0f;
            PLAY_SCREEN_SOUND(g_pScreenScrollSoundBuf, g_dwScreenScrollSoundSize);
        }
        g_nScreenPendingSelection = -1;
    }

    UpdateFrameTiming();
    UpdateFadeTransitions();
    StartHighResFrame();
    AddTimingMarker(0, 0, 0, 0xC8, 0xFF);
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
                     TEXRECT_BLEND_SHADED_OPAQUE);

    /* The model under the cursor turns twice a second about, and the items slide to rest. */
    step = g_fFrameDeltaTime + g_fFrameDeltaTime;
    g_fScreenRotationAngle += step;
    while (6.283185482f < g_fScreenRotationAngle) {
        g_fScreenRotationAngle -= 6.283185482f;
    }
    slide = g_fScreenSlideOffset;
    if (slide < 0.0f) {
        slide += step;
        if (0.0f < slide) {
            slide = 0.0f;
        }
    } else if (0.0f < slide) {
        slide -= step;
        if (slide < 0.0f) {
            slide = 0.0f;
        }
    }
    g_fScreenSlideOffset = slide;
    ease = slide < 0.0f ? slide * slide * -5.0f : slide * slide * 5.0f;

    SetupMenuProjection((f32)g_dwScreenWidth, (f32)g_dwScreenHeight);
    SetViewport(0, 0, g_dwScreenWidth, g_dwScreenHeight, TRUE);
    gSPNumLights(gDisplayListPtr++, NUMLIGHTS_1);
    gSPLight(gDisplayListPtr++, &g_MenuDirectionalLight, 1);
    gSPLight(gDisplayListPtr++, &g_MenuAmbientLight, 2);
    gSPClipRatio(gDisplayListPtr++, FRUSTRATIO_6);
    if (g_pMenuTitleImage != NULL) {
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
                         TEXRECT_BLEND_TEXTURE_2);
    }
    SetTextSize(30);
    SetTextAlignCenter();
    sprintf(text, "%%ry%s", title);
    DrawText(text, g_dwScreenWidth / 2, g_dwScreenHeight / 6 - 2);

    if (g_pfnScreenPreDrawCallback != NULL) {
        g_pfnScreenPreDrawCallback();
    } else {
        block = (Mtx *)AllocDisplayListBlock();
        guRotateF(g_afMvpMatrix, 12.0f, 1.0f, 0.0f, 0.0f);
        guMtxF2L(g_afMvpMatrix, block);
        gSPMatrix(gDisplayListPtr++, block, G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
        DrawMenuSpotlight();

        /* The marker shows while there is another item to move to. */
        other = g_nScreenCursor;
        do {
            other = (other + itemCount - 1) % itemCount;
        } while (isSelectable != NULL && !isSelectable(other));
        if (other != g_nScreenCursor) {
            DrawMenuChoiceMarker();
        }

        item = items[g_nScreenCursor];
        if (item->flags & MENU_ITEM_SPINNING) {
            /* The model spins round as the items slide, changing halfway. */
            if (SPIN_SWAP_SLIDE < g_fScreenSlideOffset || g_fScreenSlideOffset < -SPIN_SWAP_SLIDE) {
                shown = g_nPrevScreenSelection;
            } else {
                shown = g_nScreenCursor;
            }
            AdvanceModelAnimations(items[shown]->model);
            if (items[shown]->flags & MENU_ITEM_MIRRORED) {
                g_dwViewportMirrored = TRUE;
            }
            DrawRotatedModel(items[shown]->model,
                             (f32)(g_dwScreenWidth / 2),
                             (f32)(g_dwScreenHeight * 6 / 16),
                             0.0f,
                             -1.570796371f - g_fScreenSlideOffset * 6.283185482f);
            g_dwViewportMirrored = FALSE;
            if (g_fScreenSlideOffset != 0.0f) {
                goto popMatrix;
            }
        } else if (ease != 0.0f) {
            /* The item slides in, and the one the cursor left slides out. */
            DRAW_SLIDING_ITEM(items[g_nScreenCursor], ease, text);
            DRAW_SLIDING_ITEM(
                items[g_nPrevScreenSelection], (ease < 0.0f ? 5.0f : -5.0f) + ease, text);
            goto popMatrix;
        } else {
            SetTextStyleNormal();
            SetTextAlignCenter();
            item = items[g_nScreenCursor];
            if (item->model != NULL) {
                AdvanceModelAnimations(item->model);
                if (items[g_nScreenCursor]->flags & MENU_ITEM_MIRRORED) {
                    g_dwViewportMirrored = TRUE;
                }
                DrawRotatedModel(items[g_nScreenCursor]->model,
                                 (f32)(g_dwScreenWidth / 2),
                                 (f32)(g_dwScreenHeight * 6 / 16),
                                 0.0f,
                                 g_fScreenRotationAngle);
                g_dwViewportMirrored = FALSE;
            } else {
                sprintf(text, "%%ww%s", item->name);
                DrawText(text, g_dwScreenWidth / 2, g_dwScreenHeight * 9 / 16);
            }
        }
        SetTextSize(20);
        sprintf(text, "%%ry%s", items[g_nScreenCursor]->name);
        DrawText(text, g_dwScreenWidth / 2, g_dwScreenHeight * 20 / 64);
        if (g_pfnScreenPostDrawCallback != NULL) {
            g_pfnScreenPostDrawCallback(g_nScreenCursor);
        }
    popMatrix:
        gSPPopMatrix(gDisplayListPtr++, G_MTX_MODELVIEW);
    }

    DrawMenuFooterButtons();
    ApplyScreenFade();
    if (g_dwHudDebugLevel >= 2) {
        SetViewport(0, 0, g_dwScreenWidth, g_dwScreenHeight, TRUE);
        DrawTimingBars();
    }
    FlushDisplayList();

    if (IsFadingOut()) {
        if (IsFadeComplete()) {
            /* The screen is done: silence its sounds and leave. */
            g_bScreenStateInitFlag = FALSE;
            *selection = g_nScreenCursor;
            ClearFramebuffersForWideMode();
            for (i = 0; i < SOUND_EFFECT_CHANNELS; i++) {
                g_aSoundEffectChannels[i].volume = 0;
                g_aSoundEffectChannels[i].mixVolume = 0;
                g_aSoundEffectChannels[i].step = 0;
                g_aSoundEffectChannels[i].sample = g_SilenceSampleData;
            }
            return g_bScreenItemChosen ? MENU_RESULT_CHOSEN : MENU_RESULT_CANCELLED;
        }
    } else {
        for (player = 0; player < (s32)g_dwNumLocalPlayers; player++) {
            if (g_dwScreenPlayerIdx != -1 && !((1 << player) & g_dwMenuInputPlayerMask)) {
                continue;
            }
            UpdateStickRepeat(PLAYER_INPUT(player));
            buttons = PLAYER_INPUT(player)->buttons;
            if (buttons != 0) {
                g_fScreenIdleTime = 0.0f;
            }
            if (g_fScreenSlideOffset == 0.0f) {
                /*
                 * Left and right move the cursor past the items that may not be chosen, and the
                 * presses of up and down are taken too. Right is not tried once left has moved.
                 */
                if (buttons & INPUT_LEFT) {
                    TransferFlags(PLAYER_INPUT(player), INPUT_UP | INPUT_DOWN);
                    PLAYER_INPUT(player)->buttons &= ~(INPUT_UP | INPUT_DOWN);
                    g_nPrevScreenSelection = g_nScreenCursor;
                    do {
                        g_nScreenCursor = (g_nScreenCursor + itemCount - 1) % itemCount;
                    } while (isSelectable != NULL && !isSelectable(g_nScreenCursor));
                    if (g_nScreenCursor != g_nPrevScreenSelection) {
                        g_fScreenSlideOffset = 1.0f;
                        goto scrolled;
                    }
                }
                if (PLAYER_INPUT(player)->buttons & INPUT_RIGHT) {
                    TransferFlags(PLAYER_INPUT(player), INPUT_UP | INPUT_DOWN);
                    PLAYER_INPUT(player)->buttons &= ~(INPUT_UP | INPUT_DOWN);
                    g_nPrevScreenSelection = g_nScreenCursor;
                    do {
                        g_nScreenCursor = (g_nScreenCursor + 1) % itemCount;
                    } while (isSelectable != NULL && !isSelectable(g_nScreenCursor));
                    if (g_nScreenCursor != g_nPrevScreenSelection) {
                        g_fScreenSlideOffset = -1.0f;
                    scrolled:
                        PLAY_SCREEN_SOUND(g_pScreenScrollSoundBuf, g_dwScreenScrollSoundSize);
                    }
                }
                buttons = PLAYER_INPUT(player)->buttons;
                if (items[g_nScreenCursor]->flags & MENU_ITEM_ADJUSTABLE) {
                    if (buttons & INPUT_UP) {
                        *selection = g_nScreenCursor;
                        result = MENU_RESULT_UP;
                        TransferFlags(PLAYER_INPUT(player), INPUT_UP);
                        goto selected;
                    }
                    if (buttons & INPUT_DOWN) {
                        *selection = g_nScreenCursor;
                        result = MENU_RESULT_DOWN;
                        TransferFlags(PLAYER_INPUT(player), INPUT_DOWN);
                        goto selected;
                    }
                }
                if (buttons & INPUT_B) {
                    TransferFlags(PLAYER_INPUT(player), INPUT_B);
                    g_bScreenItemChosen = FALSE;
                    FADE_SCREEN_OUT();
                    goto selected;
                }
            }
            item = items[g_nScreenCursor];
            if ((item->flags & MENU_ITEM_UNAVAILABLE) || (item->flags & MENU_ITEM_LOCKED) ||
                !(buttons & (INPUT_A | INPUT_START | INPUT_Z))) {
                continue;
            }
            TransferFlags(PLAYER_INPUT(player), INPUT_A | INPUT_START | INPUT_Z);
            if (player != 0 && (items[g_nScreenCursor]->flags & MENU_ITEM_FIRST_PLAYER)) {
                continue;
            }
            g_dwMenuChoosingPlayer = player;
            if (items[g_nScreenCursor]->flags & MENU_ITEM_IMMEDIATE) {
                *selection = g_nScreenCursor;
                result = MENU_RESULT_CHOSEN;
            } else {
                g_bScreenItemChosen = TRUE;
                FadeSfxTo(0.0f, SCREEN_FADE_RATE);
                SetFadeTarget(0.0f, SCREEN_FADE_RATE);
            }
        selected:
            PLAY_SCREEN_SOUND(g_pScreenSelectSoundBuf, g_dwScreenSelectSoundSize);
        }
    }

    /* A screen left alone long enough leaves by itself. */
    if (g_fScreenIdleTimeout != 0.0f && result == 0) {
        g_fScreenIdleTime += g_fFrameDeltaTime;
        if (g_fScreenIdleTimeout <= g_fScreenIdleTime && !IsFadingOut()) {
            g_bScreenItemChosen = FALSE;
            FADE_SCREEN_OUT();
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
                AllocHeapAligned(DmaReadAssetSize(items[*itemCount]->romStart));
            DecompressModel(items[*itemCount]->model, items[*itemCount]->romStart);
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
