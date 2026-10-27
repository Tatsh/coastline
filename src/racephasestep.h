#ifndef COASTLINE_RACE_PHASE_STEP_H
#define COASTLINE_RACE_PHASE_STEP_H

#include "fade.h"
#include "frametime.h"
#include "game.h"
#include "instantreplay.h"
#include "race.h"
#include "racephaseadvance.h"
#include "racerslotaccess.h"
#include "timing.h"
#include "vehiclecount.h"

/* Step the phases of the race. */
static inline void StepRacePhases(void) {
    s32 i;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    long double timer;
#endif

    /* The phases share the countdown of the timer and the step to the next phase. */
    if (g_dwRacePhase < RACE_PHASE_GO) {
        HoldBeforeStart();
    tick:
        if (!g_dwRacePauseState) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
            timer = (long double)g_fRacePhaseTimer - g_fFrameDeltaTime;
            g_fRacePhaseTimer = (f32)timer;
            if (!(timer >= 0.0L)) {
#else
            g_fRacePhaseTimer -= g_fFrameDeltaTime;
            if (g_fRacePhaseTimer < 0.0f) {
#endif
            advance:
                AdvanceRacePhase();
            }
        }
    } else if (g_dwRacePhase == RACE_PHASE_GO) {
        g_dwStartLightsAnimation = 1;
        g_bStartLightsVisible = TRUE;
        for (i = 0; i < g_dwNumVehicles; i++) {
            RACER_SLOT_SET(
                &g_aRacerSlots[i], Status, RACER_SLOT_GET(&g_aRacerSlots[i], Status) & ~RACER_HELD);
        }
        /* The lights rise away faster and faster through the phase. */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        {
            const long double share =
                ((long double)g_aRacePhaseTable[g_dwRacePhaseIndex].seconds - g_fRacePhaseTimer) /
                g_aRacePhaseTable[g_dwRacePhaseIndex].seconds;
            g_fStartLightsDrop = (f32)((share * share) * 15.0L);
        }
#else
        g_fStartLightsDrop = (g_aRacePhaseTable[g_dwRacePhaseIndex].seconds - g_fRacePhaseTimer) /
                             g_aRacePhaseTable[g_dwRacePhaseIndex].seconds;
        g_fStartLightsDrop = g_fStartLightsDrop * (15.0f * g_fStartLightsDrop);
#endif
        goto tick;
    } else if (g_dwRacePhase == RACE_PHASE_RACING) {
        if (RunRacingPhase()) {
            goto advance;
        }
    } else if (g_dwRacePhase == RACE_PHASE_FINISHED) {
        if ((g_dwGameMode == GAME_MODE_TIME_ATTACK || g_dwGameMode == GAME_MODE_ARCADE ||
             g_dwGameMode == GAME_MODE_CHAMPIONSHIP
#if TGR_PLATFORM == TGR_PLATFORM_WIN
             || g_dwGameMode == GAME_MODE_NETWORK
#endif
             ) &&
            !gInstantReplayActive) {
            SaveRaceResultsToSlots();
        }
        goto advance;
    } else if (g_dwRacePhase == RACE_PHASE_AFTER) {
        goto tick;
    } else if (g_dwRacePhase == RACE_PHASE_LEAVE) {
        if (!g_dwRaceExitState) {
            FadeSfxTo(0.0f, FADE_SECONDS);
            if (!g_bReplayAfterRace) {
                FadeMusicTo(0.0f, FADE_SECONDS);
            }
            SetFadeTarget(0.0f, FADE_SECONDS);
            g_dwRaceExitState = TRUE;
        }
    }
}

#endif
