#ifndef COASTLINE_RACE_HOLD_H
#define COASTLINE_RACE_HOLD_H

#include "game.h"
#include "race.h"
#include "racerslotaccess.h"
#include "soundeffect.h"
#include "trackmenuitem.h"
#include "trackselect.h"
#include "vehicleaccess.h"
#include "vehiclecount.h"
#include "weather.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include "platform/win/boot/strings.h"
#include "platform/win/directplay.h"
#include "platform/win/input.h"
#include "platform/win/settings.h"
#endif

/* Hold the vehicles and run the start lights before the race starts. */
static inline void HoldBeforeStart(void) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    const TrackMenuItem *track;
#else
    TrackMenuItem *track;
#endif
    Vehicle *vehicle;
    s16 weather;
    s32 i;

#if TGR_PLATFORM == TGR_PLATFORM_WIN
    if (g_bNetRaceStartReached) {
        const s32 localPlayers = (s32)g_dwNumLocalPlayers;
        for (i = 0; i < localPlayers; i++) {
            VEHICLE_SET(&g_aVehicles[i], SubMessage, NULL);
            VEHICLE_SET(&g_aVehicles[i], SubMessageTime, 0.01f);
        }
    } else {
        const unsigned messageId = g_dwNetworkPlayMode && g_bNetReadyToStartSent ? 0xed : 0xee;
        for (i = 0; i < (s32)g_dwNumLocalPlayers; i++) {
            VEHICLE_SET(&g_aVehicles[i], SubMessage, GetStringResource(messageId));
            VEHICLE_SET(&g_aVehicles[i], SubMessageTime, 0.01f);
        }
    }
#endif
    g_bStartLightsVisible = TRUE;
    for (i = 0; i < g_dwNumVehicles; i++) {
        RACER_SLOT_SET(
            &g_aRacerSlots[i], Status, RACER_SLOT_GET(&g_aRacerSlots[i], Status) | RACER_HELD);
        vehicle = RACER_SLOT_GET(&g_aRacerSlots[i], Vehicle);
        if (vehicle == NULL || VEHICLE_GET(vehicle, Index) >= (s32)g_dwNumLocalPlayers) {
            continue;
        }
        if (g_dwGameMode == GAME_MODE_ARCADE
#if TGR_PLATFORM == TGR_PLATFORM_WIN
            || g_dwGameMode == GAME_MODE_NETWORK
#endif
        ) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
            weather = (s16)((u32)g_dwCurrentWeather - 1);
#else
            weather = g_dwCurrentWeather - 1;
#endif
            if (weather >= ARCADE_WEATHERS || weather < 0) {
                weather = 0;
            }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
            track = g_apTrackMenuItems[g_dwSelectedTrackId];
#else
            /* The descriptor of a track starts with its menu item. */
            track = (TrackMenuItem *)g_apTrackMenuItems[g_dwSelectedTrackId];
#endif
            VEHICLE_SET(vehicle,
                        TimeLeft,
                        track->arcadeTimes[VEHICLE_GET(vehicle, TableIndex)][weather].startTime);
        }
        VEHICLE_SET(vehicle, MessageTime, 1.0f);
        /* The countdown runs once for each local player, so two players step it twice a frame. */
        switch (g_dwRacePhase) {
        case RACE_PHASE_HOLD:
            g_dwStartLightsAnimation = -1;
            g_dwStartLightsStep = 0;
            break;
        case RACE_PHASE_WAIT:
            /* The original tests for the phase and does nothing in it. */
            break;
        case RACE_PHASE_COUNTDOWN:
            g_dwStartLightsAnimation = 1;
            if (g_fRacePhaseTimer < g_afStartLightThresholds[g_dwStartLightsStep]) {
                g_dwStartLightsStep++;
                if (g_dwStartLightsStep == 4) {
                    PlayGoSound();
                } else {
                    PlayBeepSound();
                }
            }
            break;
        }
    }
    g_fStartLightsDrop = 0.0f;
}

#endif
