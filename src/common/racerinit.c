#include "common/game.h"
#include "common/racers.h"
#include "common/racerslotaccess.h"
#include "common/screen.h"
#include "common/trackmenuitem.h"
#include "common/trackselect.h"
#include "common/vehicleaccess.h"
#include "common/vehiclecount.h"
#include "common/weather.h"
#if !TGR_PC
#include <PR/os.h>

#include "common/input.h"
#endif

#define ARCADE_VEHICLES 2
#define LEADING_RACERS 13
#define STRAGGLER_SPACING 600
#define STRAGGLER_START 7760
#define RACER_SPACING 550
#define NARROW_RACER_SPACING 520
#if TGR_PC
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
#if TGR_PC
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
    RACER_SLOT_SET(slot, Index, index);
    RACER_SLOT_GET(slot, Color)[0] = g_abPlayerColorsRgb[index][0];
    RACER_SLOT_GET(slot, Color)[1] = g_abPlayerColorsRgb[index][1];
    RACER_SLOT_GET(slot, Color)[2] = g_abPlayerColorsRgb[index][2];
    RACER_SLOT_SET(slot, Status, 0);
    RACER_SLOT_SET(slot, Straggler, index < (s32)g_dwNumLocalPlayers + LEADING_RACERS ? 0 : 1);
    if (RACER_SLOT_GET(slot, Index) < count) {
        RACER_SLOT_SET(slot, Vehicle, &g_aVehicles[RACER_SLOT_GET(slot, Index)]);
        VEHICLE_SET(RACER_SLOT_GET(slot, Vehicle), Racer, slot);
#if !TGR_PC
        return;
#endif
    } else {

        RACER_SLOT_SET(slot, Vehicle, NULL);
        if (RACER_SLOT_GET(slot, Straggler) == 1) {
            RACER_SLOT_SET(
                slot,
                Distance,
                (f32)(STRAGGLER_SPACING * (RACER_SLOT_GET(slot, Index) - (s32)g_dwNumLocalPlayers) -
                      STRAGGLER_START));
        } else if (g_dwSelectedTrackId == TRACK_COASTLINE ||
                   g_dwSelectedTrackId == TRACK_COASTLINE + TRACK_MIRRORED) {
            RACER_SLOT_SET(
                slot, Distance, (f32)(RACER_SLOT_GET(slot, Index) * NARROW_RACER_SPACING));
        } else {
            RACER_SLOT_SET(slot, Distance, (f32)(RACER_SLOT_GET(slot, Index) * RACER_SPACING));
        }
        FindTrackRoutePoint(g_LoadedTrack.rootSection, RACER_SLOT_GET(slot, Distance));
        RACER_SLOT_GET(slot, Position).x = g_vTrackRoutePoint.x;
        RACER_SLOT_GET(slot, Position).y = g_vTrackRoutePoint.y;
        RACER_SLOT_GET(slot, Position).z = g_vTrackRoutePoint.z;
        RACER_SLOT_SET(slot, Section, g_pTrackRouteSection);
        RACER_SLOT_SET(slot, PointIndex, g_dwTrackRoutePointIndex);
        RACER_SLOT_SET(slot, MaxGate, RACER_SLOT_SET(slot, Gate, g_dwTrackRouteGates));
        RACER_SLOT_SET(slot, Lap, RACER_SLOT_SET(slot, DistanceLaps, g_dwTrackRouteLaps));
#if TGR_PC
        NullStub();
#elif TGR_REGION != TGR_REGION_JP
        osSyncPrintf("Initial lap.gate for %d(%d): %d.%d (dist=%f)\n",
                     slot->index,
                     slot->straggler,
                     slot->lap,
                     slot->gate,
                     (double)slot->distance);
#endif /* TGR_PC */
        RACER_SLOT_GET(slot, PreviousPosition).x = RACER_SLOT_GET(slot, Position).x;
        RACER_SLOT_GET(slot, PreviousPosition).y = RACER_SLOT_GET(slot, Position).y;
        RACER_SLOT_GET(slot, PreviousPosition).z = RACER_SLOT_GET(slot, Position).z;
        RACER_SLOT_SET(slot, LapTime, 0.0f);
        RACER_SLOT_SET(slot, TotalTime, 0.0f);
        RACER_SLOT_SET(slot, BestLapTime, 0.0f);
        ARCADE_WEATHER(weather);
        RACER_SLOT_SET(slot,
                       TimeLeft,
                       SELECTED_TRACK_ITEM()
                           ->arcadeTimes[VEHICLE_GET(&(g_aVehicles[0]), TableIndex)][weather]
                           .startTime);
        RACER_SLOT_SET(slot, RacePosition, g_dwNumVehicles - RACER_SLOT_GET(slot, Index) - 1);
    }
#if TGR_PC
    if (RACER_SLOT_GET(slot, Index) < (s32)g_dwNumLocalPlayers) {
        RACER_SLOT_SET(slot, ColorPanels, NULL);
        RACER_SLOT_SET(slot, ColorPanelCount, 0);
    } else {
        MakeEnemyCarColorPanels(slot);
    }
#endif
}
