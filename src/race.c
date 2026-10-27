#include "race.h"

#include <PR/gu.h>
#include <PR/os.h>
#include <PR/os_libc.h>

#include "audio.h"
#include "audiomix.h"
#include "camera.h"
#include "carselect.h"
#include "driving.h"
#include "effects.h"
#include "fade.h"
#include "game.h"
#include "hud.h"
#include "input.h"
#include "mainmenu.h"
#include "options.h"
#include "physics.h"
#include "racecontrols.h"
#include "racefinish.h"
#include "raceinit.h"
#include "raceload.h"
#include "racemenu.h"
#include "racephases.h"
#include "racerender.h"
#include "racespecials.h"
#include "render.h"
#include "replay.h"
#include "rumble.h"
#include "sky.h"
#include "splash.h"
#include "system.h"
#include "text.h"
#include "timing.h"
#include "track.h"
#include "trackrender.h"
#include "trackselect.h"
#include "vecmath.h"
#include "vehicle.h"
#include "vehiclerender.h"
#include "visgrid.h"
#include "weather.h"

/* The time the sounds and the screen take to fade, in seconds. */
#define FADE_SECONDS 0.2f

/* The time the races the computer drives last, in seconds. */
#if TGR_REGION == TGR_REGION_US
#define ATTRACT_RACE_SECONDS 39.6f
#define ENDING_RACE_SECONDS 140.0f
#else
/* The attract race of the later releases lasts as long as its recording. */
#define CREDITS_RACE_SECONDS 42.0f
#define ENDING_RACE_SECONDS 170.0f
#endif
#define AFTER_SEASON_RACE_SECONDS 16.0f

/* The sound effect channels a race uses. */
#define RACE_SOUND_CHANNELS 6

/* The x of the middle of the screen. */
#define SCREEN_CENTER_X 160

/* The bytes of the end of the recording of the first demonstration race it fades out over. */
#define DEMO_FADE_BYTES 30

/* Fade the screen and the sounds out to leave the race, once. */
static void LeaveRace(void) {
    if (!g_dwRaceExitState) {
        g_dwRaceExitState = TRUE;
        g_bReplayAfterRace = FALSE;
        FadeSfxTo(0.0f, FADE_SECONDS);
        SetFadeTarget(0.0f, FADE_SECONDS);
    }
}

void LoadAndPlayTrackBgm(void) {
    ResetMusicPlaybackState();
    DecompressFromRom(g_abMenuMusicModule, g_adwTrackMusicRom[g_dwSelectedTrackId], NULL);
    InitMusicPlayback(g_abMenuMusicModule, g_abMusicModuleWorkBuffer);
}

void StartAttractDemo(void) {
    g_dwGameMode = GAME_MODE_DEMO;
    g_dwRaceSessionType = RACE_SESSION_ATTRACT;
    SetCurrentScreenCallback(ProcessRacingScreen);
}

void StartCreditsRace(void) {
    g_dwGameMode = GAME_MODE_DEMO;
    if (g_dwMultiplayerMode) {
        g_dwRaceSessionType = RACE_SESSION_SCROLLING_CREDITS;
    } else {
        g_dwRaceSessionType = RACE_SESSION_CREDIT_CAPTIONS;
    }
    SetCurrentScreenCallback(ProcessRacingScreen);
}

/* End the instant replay once the recordings it plays run out. */
static void CheckInstantReplayEnd(void) {
    PlayerInputState *input;
    s32 other;

    if (g_dwGameMode != GAME_MODE_ARCADE) {
        input = g_aVehicles[0].input;
        if (input->playbackLimit == input->playbackPosition) {
            LeaveRace();
        }
        return;
    }
    if (g_aViewportRects[0].vehicleIndex == g_dwBestReplayPlayer) {
        input = g_aVehicles[g_dwBestReplayPlayer].input;
        if (input->playbackLimit != input->playbackPosition) {
            return;
        }
        if (g_dwNumLocalPlayers == 1) {
            LeaveRace();
            return;
        }
        /* Once the best recording ends, the view turns to the other player's. */
        if (g_aVehicles[g_dwBestReplayPlayer].input->playbackLimit + 0xF0 !=
            g_aVehicles[g_dwBestReplayPlayer == 0].input->playbackPosition) {
            return;
        }
        g_aViewportRects[1].vehicleIndex = g_dwBestReplayPlayer;
        g_aViewportRects[0].vehicleIndex = g_aViewportRects[1].vehicleIndex == 0;
    }
    other = g_dwBestReplayPlayer == 0;
    if (g_aVehicles[other].input->playbackLimit == g_aVehicles[other].input->playbackPosition) {
        LeaveRace();
    }
}

void ProcessRacingScreen(void) {
    s32 paused, i;

    if (!g_dwRaceInitialized) {
        InitRace();
    }
    g_wRaceScreenLoopExited = FALSE;
#if TGR_REGION != TGR_REGION_US
    RestoreReplayKeyframeState();
#endif
    if (g_dwRacePhase == RACE_PHASE_RACING) {
        for (i = 0; i < (s32)g_dwNumLocalPlayers; i++) {
            SaveLastLapSnapshot(&g_aVehicles[i]);
        }
    }
    if (g_dwGameMode != GAME_MODE_DEMO) {
        g_bTrackCollisionFrameToggle ^= 1;
    }
    if (gInstantReplayActive && g_dwRacePhase == RACE_PHASE_RACING) {
        CheckInstantReplayEnd();
    }

    UpdateFrameTiming();
    g_dwStartLightsAnimation = 0;
    paused = g_dwRacePauseState;
    g_bStartLightsVisible = FALSE;
    StepRacePhases();

    AddTimingMarker(0, 0, 0, 0xC8, 0xFF);
    ClearPerFrameVariables();
    for (i = 0; i < g_dwNumActiveVehicles; i++) {
        ClearCameraChanged(&g_aVehicles[i]);
    }
    for (i = 0; i < g_dwNumVehicles; i++) {
        UpdateRacerSlotMotion(&g_aRacerSlots[i]);
    }
    for (i = 0; i < g_dwNumVehicles; i++) {
        UpdateRacerSlotRaceState(&g_aRacerSlots[i]);
    }
    if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP) {
        for (i = 0; i < (s32)g_dwNumLocalPlayers; i++) {
            ManageAiRacerSlots(&g_aVehicles[i]);
        }
    }
    SortRacersByPosition();
    if (!g_dwRacePauseState) {
        AddTimingMarker(0, 0x80, 0x80, 0, 0xFF);
        UpdateAllVehicleEffects();
        AddTimingMarker(0, 0, 0xFF, 0xFF, 0xFF);
        UpdatePrecipitationParticles();
        UpdateTrackSpecials();
    }

    for (i = 0; i < g_dwNumActiveVehicles; i++) {
        UpdateVehicleSounds(&g_aVehicles[i]);
    }
    ResetSoundSourceTracking();
    for (i = 0; i < g_dwNumViewports; i++) {
        g_pActiveCameraVehicle = &g_aVehicles[g_aViewportRects[i].vehicleIndex];
        g_pActiveCamera = g_pActiveCameraVehicle->activeCamera;
        if (g_dwAirplaneInstance && g_bAirplaneFlying) {
            UpdateAirplaneSound(g_dwAirplaneInstance,
                                (Vec3f *)g_LoadedTrack.instances[g_dwAirplaneInstance].matrix[3],
                                g_pActiveCamera);
        }
        if (g_pActiveCamera != NULL) {
            s32 j;

            for (j = 0; j < g_dwWaterfallCount; j++) {
                /* The waterfalls are heard under the source of the airplane. */
                UpdateWaterfallSound(
                    g_dwAirplaneInstance,
                    (Vec3f *)g_LoadedTrack.instances[g_adwWaterfallInstances[j]].matrix[3],
                    g_pActiveCamera);
                if (g_pActiveCamera == NULL) {
                    break;
                }
            }
        }
        StubSoundUpdate(g_pActiveCamera);
    }
    UpdateAmbientSoundChannel();

    UpdateFadeTransitions();
    StartLowResFrame();
    AddTimingMarker(0, 0, 0x82, 0, 0xFF);
    ResetShadowBuffers();
    ClearDepthBuffer();
    if (g_dwScreenClearFrames || g_dwSkyboxDisabled) {
        ClearScreen(0, 0, 0);
        if (g_dwScreenClearFrames) {
            g_dwScreenClearFrames--;
        }
    } else if (g_aViewportRects[0].width < 0x130) {
        /* Black out the screen right of a narrow view. */
        DrawFilledRect(g_aViewportRects[0].width + g_aViewportRects[0].x,
                       8,
                       0x130 - g_aViewportRects[0].width,
                       0xE0,
                       0,
                       0,
                       0);
    }
    if (g_dwNumViewports != g_dwPrevViewportCount) {
        g_dwScreenClearFrames = 2;
        g_dwPrevViewportCount = g_dwNumViewports;
    }
    for (i = 0; i < g_dwNumViewports; i++) {
        RenderRaceView(i);
    }
    StubEmpty();
    SetScissorRect(0, 0, g_dwScreenWidth, g_dwScreenHeight);
#if TGR_REGION != TGR_REGION_US
    /* The cheat finishes the race, the first player first and the second second. */
    if (g_dwCheatFinishRacePending) {
        g_aVehicles[0].racePosition = 0;
        g_aVehicles[0].lap = g_dwTotalLaps;
        g_aRacerSlots[0].status &= RACER_HELD;
        g_aRacerSlots[0].status |= RACER_FINISHED;
        g_aVehicles[1].racePosition = 1;
        g_aVehicles[1].lap = g_dwTotalLaps;
        g_aRacerSlots[1].status &= RACER_HELD;
        g_aRacerSlots[1].status |= RACER_FINISHED;
        g_bReplayAfterRace = FALSE;
        if (g_dwRacePhase < RACE_PHASE_RACING) {
            g_dwRacePhase = RACE_PHASE_RACING;
        }
        g_dwCheatFinishRacePending = FALSE;
    }
#endif

    if (g_dwRacePauseState) {
        paused = ProcessPauseMenu(paused);
    } else {
        paused = ProcessRaceButtons(paused);
    }
    if (g_dwGameMode == GAME_MODE_DEMO && g_dwRaceSessionType == RACE_SESSION_SCROLLING_CREDITS) {
        ScrollTextUp();
        DrawScrollingText();
    } else if (g_dwGameMode == GAME_MODE_DEMO &&
               g_dwRaceSessionType == RACE_SESSION_CREDIT_CAPTIONS &&
               g_aCreditCaptions[g_dwRaceCaptionIndex].title != NULL) {
        DrawCreditCaption();
    }
    ApplyScreenFade();
    if (g_dwHudDebugLevel >= 2) {
        SetViewport(0, 0, g_dwScreenWidth, g_dwScreenHeight, TRUE);
        DrawTimingBars();
    }
    AddTimingMarker(0, 0xFF, 0xE0, 0, 0xFF);
    StreamAnimatedTrackTextures();
    AddTimingMarker(0, 0, 0x82, 0, 0xFF);
    SetNextScreenCallback(UpdateRaceAnimations);
#if TGR_REGION != TGR_REGION_US
    /* The first demonstration race fades to black over the end of its recording. */
    if (g_dwGameMode == GAME_MODE_DEMO && !g_dwRaceSessionType &&
        g_aVehicles[0].input->playbackPosition >=
            g_aVehicles[0].input->playbackLimit - DEMO_FADE_BYTES) {
#if TGR_REGION == TGR_REGION_JP
        /* The Japanese release clears to black at once. */
        ClearScreen(0, 0, 0);
#else
        DrawScreenFadeOverlay(
            (f32)(g_aVehicles[0].input->playbackLimit - g_aVehicles[0].input->playbackPosition) /
            DEMO_FADE_BYTES);
#endif
    }
#endif
    FlushDisplayList();
    if (IsFadingOut() && IsFadeComplete()) {
        FinishRace();
    }

    if (g_dwRacePauseState != (u32)paused) {
        if (paused) {
            SaveClockForPause();
        } else {
            RestoreClockAfterPause();
        }
        g_dwRacePauseState = paused;
    }
}

/* Add a line of the bonus of a finished championship, after a blank line when others precede it. */
#define ADD_BONUS_LINE(line)                                                                       \
    {                                                                                              \
        if (count) {                                                                               \
            g_apszSeasonResultMsgLines[count++] = "";                                              \
        }                                                                                          \
        g_apszSeasonResultMsgLines[count++] = line;                                                \
    }

void ProcessSeasonResults(void) {
    s32 count, i, race, base;

    count = 0;
    if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP && g_dwSavedResultsValid) {
        for (i = 0; i < (s32)g_dwNumLocalPlayers; i++) {
            g_aVehicles[i].profile->round++;
            if (g_aVehicles[i].profile->round ==
                g_SeasonTable[g_aVehicles[i].profile->season].raceCount) {
                g_aVehicles[i].profile->round = 0;
                if (g_aVehicles[i].profile->seasonPoints[g_aVehicles[i].profile->season] >=
                    g_SeasonTable[g_aVehicles[i].profile->season].pointsNeeded) {
                    g_aVehicles[i].profile->unlocks |=
                        g_SeasonTable[g_aVehicles[i].profile->season].unlocks;
                    g_aVehicles[i].profile->season++;
                    if (g_aVehicles[i].profile->season == SEASONS) {
                        g_apszSeasonResultMsgLines[count++] = "ALL SEASONS COMPLETED!";
                        /* Winning every race of the last season adds the strip mine. */
                        for (race = 0; race < g_SeasonTable[SEASONS - 1].raceCount; race++) {
                            if (g_aVehicles[i].profile->seasonPlaces[SEASONS - 1][race] != 0) {
                                goto cars;
                            }
                        }
                        if (g_aVehicles[i].profile->championshipFlags & 1) {
                            if (!(g_aVehicles[i].profile->unlockedTracks &
                                  (1 << TRACK_MIRROR_STRIP_MINE))) {
                                ADD_BONUS_LINE("You Won all races this season!");
                                g_apszSeasonResultMsgLines[count++] =
                                    "Bonus: Mirror Mine Track Added!";
                                g_aVehicles[i].profile->unlockedTracks |=
                                    1 << TRACK_MIRROR_STRIP_MINE;
                            }
                        } else if (!(g_aVehicles[i].profile->unlockedTracks &
                                     (1 << TRACK_STRIP_MINE))) {
                            ADD_BONUS_LINE("You Won all races this season!");
                            g_apszSeasonResultMsgLines[count++] = "Bonus: Mine Track Added!";
                            g_aVehicles[i].profile->unlockedTracks |= 1 << TRACK_STRIP_MINE;
                        }
                    cars:
                        /* Each championship of the years unlocks a car. */
                        if (g_aVehicles[i].profile->championshipFlags == 0) {
                            ADD_BONUS_LINE("Bonus: Milk Truck Added!");
                            g_aVehicles[i].profile->unlocks |= 0x400;
                        } else if (g_aVehicles[i].profile->championshipFlags == 1) {
#if TGR_REGION == TGR_REGION_JP
                            ADD_BONUS_LINE("Bonus: Type-MN Car Added!");
#else
                            ADD_BONUS_LINE("Bonus: Helmet Car Added!");
#endif
                            g_aVehicles[i].profile->unlocks |= 0x200;
                        } else if (g_aVehicles[i].profile->championshipFlags == 2) {
                            ADD_BONUS_LINE("Bonus: Cupra Car Added!");
                            g_aVehicles[i].profile->unlocks |= 0x800;
                        } else if (g_aVehicles[i].profile->championshipFlags == 3) {
                            ADD_BONUS_LINE("Bonus: Beach Ball Car Added!");
                            g_aVehicles[i].profile->unlocks |= 0x1000;
                        } else if (g_aVehicles[i].profile->championshipFlags == 4) {
                            ADD_BONUS_LINE("Bonus: Mirrored Cars Added!");
                            g_aVehicles[i].profile->unlocks |= 0x8000;
                        }
                        g_aVehicles[i].profile->season = 0;
                        g_aVehicles[i].profile->championshipFlags++;
                    } else {
#if TGR_REGION == TGR_REGION_JP
                        g_apszSeasonResultMsgLines[count++] = "SEASON COMPLETE";
#else
                        g_apszSeasonResultMsgLines[count++] = "SEASON COMPLETED!";
                        g_apszSeasonResultMsgLines[count++] = "";
                        g_apszSeasonResultMsgLines[count++] = "PROGRESS TO THE NEXT SEASON";
#endif
                    }
                } else {
#if TGR_REGION == TGR_REGION_JP
                    g_apszSeasonResultMsgLines[count++] = "SEASON TRY AGAIN";
#else
                    g_apszSeasonResultMsgLines[count++] = "Sorry, you don't have enough points to";
                    g_apszSeasonResultMsgLines[count++] =
                        "pass this season.  You'll have to try again.";
#endif
                }
                g_aVehicles[i].profile->seasonPoints[g_aVehicles[i].profile->season] = 0;
            }
            /* The track and the weather of the next race become available. */
            base = g_aVehicles[i].profile->championshipFlags & 1 ? TRACK_MIRRORED : 0;
            g_aVehicles[i].profile->unlockedTracks |=
                1 << (g_SeasonTable[g_aVehicles[i].profile->season]
                          .races[g_aVehicles[i].profile->round][0] +
                      base);
            g_aVehicles[i].profile->unlockedWeathers |=
                1 << g_SeasonTable[g_aVehicles[i].profile->season]
                         .races[g_aVehicles[i].profile->round][1];
        }
    }
    g_dwSeasonResultMsgLineCount = count;
}
