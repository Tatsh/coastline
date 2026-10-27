#include "game.h"
#include "racers.h"
#include "screen.h"
#include "trackmenuitem.h"
#include "trackselect.h"
#include "vehiclecount.h"
#include "weather.h"
#if TGR_PLATFORM != TGR_PLATFORM_WIN
#include <PR/os.h>

#include "input.h"
#endif

#define ARCADE_VEHICLES 2
#define LEADING_RACERS 13
#define STRAGGLER_SPACING 600
#define STRAGGLER_START 7760
#define RACER_SPACING 550
#define NARROW_RACER_SPACING 520
#if TGR_PLATFORM == TGR_PLATFORM_WIN
#define SELECTED_TRACK_ITEM() (g_apTrackMenuItems[g_dwSelectedTrackId])
#else
#define SELECTED_TRACK_ITEM() ((TrackMenuItem *)g_apTrackMenuItems[g_dwSelectedTrackId])
#endif
#define ARCADE_WEATHER(weather)                                                                    \
    weather = g_dwCurrentWeather - 1;                                                              \
    if ((weather) >= ARCADE_WEATHERS || (weather) < 0) {                                           \
        weather = 0;                                                                               \
    }

void InitRacerSlot(RacerSlot *slot) {
    s32 count;
    s32 index;
    s16 weather;

    switch (g_dwGameMode) {
    case GAME_MODE_ARCADE:
        count = ARCADE_VEHICLES;
        break;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    case GAME_MODE_NETWORK:
        count = 9999;
        break;
#endif
    case GAME_MODE_CHAMPIONSHIP:
        count = g_dwNumLocalPlayers;
        break;
    default:
        count = g_dwNumActiveVehicles;
        break;
    }
    index = slot - g_aRacerSlots;
    slot->index = index;
    slot->color[0] = g_abPlayerColorsRgb[index][0];
    slot->color[1] = g_abPlayerColorsRgb[index][1];
    slot->color[2] = g_abPlayerColorsRgb[index][2];
    slot->status = 0;
    slot->straggler = index < (s32)g_dwNumLocalPlayers + LEADING_RACERS ? 0 : 1;
    if (slot->index < count) {
        slot->vehicle = &g_aVehicles[slot->index];
        slot->vehicle->racer = slot;
#if TGR_PLATFORM != TGR_PLATFORM_WIN
        return;
#endif
    } else {

        slot->vehicle = NULL;
        if (slot->straggler == 1) {
            slot->distance = (f32)(STRAGGLER_SPACING * (slot->index - (s32)g_dwNumLocalPlayers) -
                                   STRAGGLER_START);
        } else if (g_dwSelectedTrackId == TRACK_COASTLINE ||
                   g_dwSelectedTrackId == TRACK_COASTLINE + TRACK_MIRRORED) {
            slot->distance = (f32)(slot->index * NARROW_RACER_SPACING);
        } else {
            slot->distance = (f32)(slot->index * RACER_SPACING);
        }
        FindTrackRoutePoint(
#if TGR_PLATFORM == TGR_PLATFORM_WIN
            g_TrackHeader_pSplineModel,
#else
            g_LoadedTrack.rootSection,
#endif
            slot->distance);
        slot->position.x = g_vTrackRoutePoint.x;
        slot->position.y = g_vTrackRoutePoint.y;
        slot->position.z = g_vTrackRoutePoint.z;
        slot->section = g_pTrackRouteSection;
        slot->pointIndex = g_dwTrackRoutePointIndex;
        slot->maxGate = slot->gate = g_dwTrackRouteGates;
        slot->lap = slot->distanceLaps = g_dwTrackRouteLaps;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        NullStub();
#elif TGR_REGION != TGR_REGION_JP
        osSyncPrintf("Initial lap.gate for %d(%d): %d.%d (dist=%f)\n",
                     slot->index,
                     slot->straggler,
                     slot->lap,
                     slot->gate,
                     (double)slot->distance);
#endif
        slot->previousPosition.x = slot->position.x;
        slot->previousPosition.y = slot->position.y;
        slot->previousPosition.z = slot->position.z;
        slot->lapTime = 0.0f;
        slot->totalTime = 0.0f;
        slot->bestLapTime = 0.0f;
        ARCADE_WEATHER(weather);
        slot->timeLeft =
            SELECTED_TRACK_ITEM()->arcadeTimes[g_aVehicles[0].tableIndex][weather].startTime;
        slot->racePosition = g_dwNumVehicles - slot->index - 1;
    }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    if (slot->index < (s32)g_dwNumLocalPlayers) {
        slot->colorPanels = NULL;
        slot->colorPanelCount = 0;
    } else {
        MakeEnemyCarColorPanels(slot);
    }
#endif
}
