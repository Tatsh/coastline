#ifndef COASTLINE_RACE_PHASE_ADVANCE_H
#define COASTLINE_RACE_PHASE_ADVANCE_H

#include "race.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include "platform/win/directplay.h"
#endif

/* Go on to the next phase of the race. */
static inline void AdvanceRacePhase(void) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    if (!g_bNetRaceStartReached) {
        return;
    }
#endif
    g_dwRacePhaseIndex++;
    g_dwRacePhase = g_aRacePhaseTable[g_dwRacePhaseIndex].phase;
    g_fRacePhaseTimer = g_aRacePhaseTable[g_dwRacePhaseIndex].seconds;
}

#endif
