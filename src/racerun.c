#include "audio.h"
#include "camera.h"
#include "effects.h"
#include "fade.h"
#include "frameclear.h"
#include "frametime.h"
#include "game.h"
#include "hud.h"
#include "input.h"
#include "physics.h"
#include "race.h"
#include "racecontrols.h"
#include "racefinish.h"
#include "raceload.h"
#include "racephases.h"
#include "racerender.h"
#include "racespecials.h"
#include "render.h"
#include "replay.h"
#include "sky.h"
#include "soundsource.h"
#include "system.h"
#include "timing.h"
#include "track.h"
#include "vehicle.h"
#include "vehiclecount.h"
#include "weather.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include "win32/directplay.h"
#include "win32/frametimer.h"
#include "win32/ghost.h"
#include "win32/hud.h"
#include "win32/input.h"
#include "win32/racepacing.h"
#include "win32/racesound.h"
#include "win32/settings.h"
#include "win32/snapshot.h"
#include "win32/sound.h"
#else
#include "audiomix.h"
#include "shadowbuffers.h"
#include "splash.h"
#include "text.h"
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

#define FADE_SECONDS 0.2f
#define DEMO_FADE_BYTES 30

#if TGR_PLATFORM != TGR_PLATFORM_WIN
/* Fade the screen and the sounds out to leave the race, once. */
static void LeaveRace(void) {
    if (!g_dwRaceExitState) {
        g_dwRaceExitState = TRUE;
        g_bReplayAfterRace = FALSE;
        FadeSfxTo(0.0f, FADE_SECONDS);
        SetFadeTarget(0.0f, FADE_SECONDS);
    }
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

#endif /* TGR_PLATFORM != TGR_PLATFORM_WIN */

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#define RACE_VEHICLE_CALL(vehicle, method) ((vehicle)->method())
#define RACE_SLOT_CALL(slot, method) ((slot)->method())
#define RACE_TIMING_MARKER(a, b, c, d, e) NullStub()
void RunRaceGameScreen(void) {
#else
#define RACE_VEHICLE_CALL(vehicle, method) method(vehicle)
#define RACE_SLOT_CALL(slot, method) method(slot)
#define RACE_TIMING_MARKER(a, b, c, d, e) AddTimingMarker(a, b, c, d, e)
void ProcessRacingScreen(void) {
#endif
    s32 paused, i;

#if TGR_PLATFORM == TGR_PLATFORM_WIN
    UpdateRaceFrameStatistics();
#endif
    if (!g_dwRaceInitialized) {
        InitRace();
    }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    else if (g_dwNetworkPlayMode) {
        s32 slot = PopDepartedPlayerSlot();
        if (slot >= 0 && !gInstantReplayActive) {
            RemoveNetworkPlayerById(slot);
        }
    }
#endif
    g_wRaceScreenLoopExited = FALSE;
#if TGR_PLATFORM != TGR_PLATFORM_WIN && TGR_REGION != TGR_REGION_US
    RestoreReplayKeyframeState();
#endif
    if (g_dwRacePhase == RACE_PHASE_RACING) {
        for (i = 0; i < (s32)g_dwNumLocalPlayers; i++) {
            RACE_VEHICLE_CALL(&g_aVehicles[i], SaveLastLapSnapshot);
        }
    }
    if (g_dwGameMode != GAME_MODE_DEMO
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        && !g_dwRacePauseState
#endif
    ) {
        g_bTrackCollisionFrameToggle ^= 1;
    }
#if TGR_PLATFORM != TGR_PLATFORM_WIN
    if (gInstantReplayActive && g_dwRacePhase == RACE_PHASE_RACING) {
        CheckInstantReplayEnd();
    }
#endif

    UpdateFrameTiming();
    g_dwStartLightsAnimation = 0;
    paused = g_dwRacePauseState;
    g_bStartLightsVisible = FALSE;
    StepRacePhases();

    RACE_TIMING_MARKER(0, 0, 0, 0xC8, 0xFF);
    ClearPerFrameVariables();
    for (i = 0; i < g_dwNumActiveVehicles; i++) {
        RACE_VEHICLE_CALL(&g_aVehicles[i], ClearCameraChanged);
    }
    for (i = 0; i < g_dwNumVehicles; i++) {
        RACE_SLOT_CALL(&g_aRacerSlots[i], UpdateRacerSlotMotion);
    }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    if (g_dwNetworkPlayMode || gInstantReplayActive) {
        for (i = 0; i < g_dwNumVehicles; i++) {
            RACE_SLOT_CALL(&g_aRacerSlots[i], UpdateRemoteVehicleFrame);
        }
    }
#endif
    for (i = 0; i < g_dwNumVehicles; i++) {
        RACE_SLOT_CALL(&g_aRacerSlots[i], UpdateRacerSlotRaceState);
    }
    if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP) {
        for (i = 0; i < (s32)g_dwNumLocalPlayers; i++) {
            RACE_VEHICLE_CALL(&g_aVehicles[i], ManageAiRacerSlots);
        }
    }
    SortRacersByPosition();
    if (!g_dwRacePauseState
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        && gInstantReplayActive != 2
#endif
    ) {
        RACE_TIMING_MARKER(0, 0x80, 0x80, 0, 0xFF);
        UpdateAllVehicleEffects();
        RACE_TIMING_MARKER(0, 0, 0xFF, 0xFF, 0xFF);
        UpdatePrecipitationParticles();
        UpdateTrackSpecials();
    }

    for (i = 0; i < g_dwNumActiveVehicles; i++) {
        RACE_VEHICLE_CALL(&g_aVehicles[i], UpdateVehicleSounds);
    }
    ResetSoundSourceTracking();
    for (i = 0; i < g_dwNumViewports; i++) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        CameraFrame *camera = g_aVehicles[g_aViewportRects[i].vehicleIndex].GetActiveCamera();
        if (g_dwAirplaneInstance && g_bAirplaneFlying) {
            UpdateAirplaneSound(g_dwAirplaneInstance,
                                &g_LoadedTrack.instances[g_dwAirplaneInstance].rows[3].vector,
                                camera);
        }
        if (camera != NULL) {
            s32 j;
            for (j = 0; j < g_dwWaterfallCount; j++) {
                UpdateWaterfallSound(
                    g_dwAirplaneInstance,
                    &g_LoadedTrack.instances[g_adwWaterfallInstances[j]].rows[3].vector,
                    camera);
            }
        }
        NullStub();
#else
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
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
    }
    UpdateAmbientSoundChannel();

#if TGR_PLATFORM == TGR_PLATFORM_WIN
    UpdateRaceAnimations();
    g_aFrameSnapshots[AcquireNextFrameSnapshotSlot()].Capture();
    PaceRaceFrames();
#else
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
#endif /* TGR_REGION != TGR_REGION_US */

#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

    if (g_dwRacePauseState) {
        paused = ProcessPauseMenu(paused);
    } else {
        paused = ProcessRaceButtons(paused);
    }
    if (g_dwGameMode == GAME_MODE_DEMO && g_dwRaceSessionType == RACE_SESSION_SCROLLING_CREDITS) {
        ScrollTextUp();
#if TGR_PLATFORM != TGR_PLATFORM_WIN
        DrawScrollingText();
    } else if (g_dwGameMode == GAME_MODE_DEMO &&
               g_dwRaceSessionType == RACE_SESSION_CREDIT_CAPTIONS &&
               g_aCreditCaptions[g_dwRaceCaptionIndex].title != NULL) {
        DrawCreditCaption();
    }
#else
    }
    if (g_dwGameMode == GAME_MODE_DEMO && g_dwRaceSessionType == RACE_SESSION_CREDIT_CAPTIONS &&
        g_aCreditCaptions[g_dwRaceCaptionIndex].title != NULL) {
        long double elapsed = (long double)g_fRaceCaptionTimer + g_fFrameDeltaTime;
        g_fRaceCaptionTimer = (f32)elapsed;
        if (elapsed > g_aCreditCaptions[g_dwRaceCaptionIndex].duration) {
            g_fRaceCaptionTimer = 0.0f;
            g_dwRaceCaptionIndex = (s32)((u32)g_dwRaceCaptionIndex + 1u);
        }
    }
#endif /* TGR_PLATFORM != TGR_PLATFORM_WIN */
#if TGR_PLATFORM != TGR_PLATFORM_WIN
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
#endif /* TGR_REGION != TGR_REGION_US */
    FlushDisplayList();
#endif /* TGR_PLATFORM != TGR_PLATFORM_WIN */
    if (IsFadingOut() && IsFadeComplete()) {
        FinishRace();
    }

    if (g_dwRacePauseState != (u32)paused) {
        if (paused) {
            SaveClockForPause();
#if TGR_PLATFORM == TGR_PLATFORM_WIN
            PauseMusicPlayback();
            MuteAllActiveSoundChannels();
            MuteVehicleSoundVoices();
#endif
        } else {
            RestoreClockAfterPause();
#if TGR_PLATFORM == TGR_PLATFORM_WIN
            if (g_bBgmVolume) {
                UnpauseMusicPlayback();
                DispatchMusicVolumeChange(g_bBgmVolume);
            }
#endif
        }
        g_dwRacePauseState = paused;
    }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    if (g_dwGameMode == GAME_MODE_NETWORK) {
        ProcessPendingNetworkNotifications();
    }
    {
        const Vehicle *vehicle = &g_aVehicles[0];
        const RigidBody *body = &vehicle->GetBody();
        int enabled = body->wheels[0]->contactFrames || body->wheels[1]->contactFrames;
        UpdateForceFeedbackForce(body->skidding, enabled, vehicle->GetTireType());
    }
    if (g_bSoundEnabledIni && !g_dwRacePauseState) {
        ResumeLoopingSoundChannels();
    }
    if (gInstantReplayActive) {
        UpdateReplayScrubControls();
    } else {
        AdvanceGhostRecordFrame();
    }
    TickDebugOverlayMessageTimer();
    AdvanceFrameTickCounter();
    if (!g_bNetRaceStartReached) {
        if (g_dwNetworkPlayMode) {
            CheckNetworkRaceStartTime();
        }
        ResetFrameTimer();
    }
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
}
