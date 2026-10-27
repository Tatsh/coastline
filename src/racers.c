#include "racers.h"

#include <PR/os.h>
#include <PR/os_libc.h>

#include "audio.h"
#include "camera.h"
#include "carselect.h"
#include "game.h"
#include "input.h"
#include "mathutil.h"
#include "race.h"
#include "racemenu.h"
#include "raceprogress.h"
#include "racermotion.h"
#include "racersharing.h"
#include "render.h"
#include "system.h"
#include "trackselect.h"
#include "util.h"
#include "vehiclerender.h"
#include "weather.h"

/* The racers after the players that are not stragglers, and where the racers without vehicles
 * start: stragglers 600 apart from 7760 behind, the others 550 apart, or 520 on the coastline. */
#define LEADING_RACERS 13
#define STRAGGLER_SPACING 600
#define STRAGGLER_START 7760
#define RACER_SPACING 550
#define NARROW_RACER_SPACING 520

/* The arcade gives two racers vehicles; the kinds of racer share vehicles separately. */
#define ARCADE_VEHICLES 2

/* Count the gate after the ones counted if the segment crosses it, and a lap at the first. */
#define COUNT_ROUTE_GATE(next, from, to)                                                           \
    if (g_LoadedTrack.gateCount) {                                                                 \
        next = (g_dwTrackRouteGates + 1) % g_LoadedTrack.gateCount;                                \
        if (CROSSES_GATE(&g_LoadedTrack.gates[next], from, to)) {                                  \
            g_dwTrackRouteGates++;                                                                 \
            if (&g_LoadedTrack.gates[next] == g_LoadedTrack.gates) {                               \
                g_dwTrackRouteLaps++;                                                              \
            }                                                                                      \
        }                                                                                          \
    }

/* Skip the alternate branches from a section on. */
#define SKIP_ALTERNATE_SECTIONS(section)                                                           \
    while ((section) != NULL && ((section)->flags & TRACK_SECTION_ALTERNATE)) {                    \
        section = (section)->next;                                                                 \
    }

void SortRacersByPosition(void) {
    RacerProgress entries[RACER_SLOTS];
    RacerSlot *slot;
    s32 count = 0;
    s32 i;

    for (i = 0; i < g_dwNumVehicles; i++) {
        slot = &g_aRacerSlots[i];
        if (slot->status & RACER_FINISHED) {
            continue;
        }
        entries[count].slot = i;
        entries[count].progress = slot->vehicle != NULL ? slot->vehicle->distance : slot->distance;
        count++;
    }
    if (count) {
        QuickSort(entries, count, sizeof(RacerProgress), CompareRacerProgressAsc);
    }
    /* The finished racers keep the leading places. */
    for (i = 0; i < count; i++) {
        slot = &g_aRacerSlots[entries[i].slot];
        if (slot->vehicle != NULL) {
            slot->vehicle->racePosition = g_dwNumVehicles - i - 1;
        } else {
            slot->racePosition = g_dwNumVehicles - i - 1;
        }
    }
}
