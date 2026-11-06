#include "common/racers.h"

#include "common/racersharing.h"
#include "common/racerslotaccess.h"
#include "common/vehicleaccess.h"
#include "common/vehiclecount.h"

#if TGR_BOSS_RALLY
#include <stdlib.h>

#include "common/settings.h"
#endif
#if TGR_PLATFORM == TGR_PLATFORM_WIN
#define QuickSort qsort
#else
#include "common/util.h"
#endif

void SortRacersByPosition(void) {
    RacerProgress entries[RACER_SLOTS];
#if !TGR_BOSS_RALLY
    s32 i;
    s32 count;

    count = 0;
    for (i = 0; i < g_dwNumVehicles; i++) {
        if (!(g_aRacerSlots[i].status & RACER_FINISHED)) {
            entries[count].slot = i;
            if (g_aRacerSlots[i].vehicle != NULL) {
                entries[count++].progress = g_aRacerSlots[i].vehicle->distance;
            } else {
                entries[count++].progress = g_aRacerSlots[i].distance;
            }
        }
    }
    if (count != 0) {
        QuickSort(entries, count, sizeof(RacerProgress), CompareRacerProgressAsc);
    }
    for (i = 0; i < count; i++) {
        if (g_aRacerSlots[entries[i].slot].vehicle != NULL) {
            g_aRacerSlots[entries[i].slot].vehicle->racePosition = g_dwNumVehicles - i - 1;
        } else {
            g_aRacerSlots[entries[i].slot].racePosition = g_dwNumVehicles - i - 1;
        }
    }
#else
    RacerSlot *slot;
    s32 count = 0;
    s32 i;

#if TGR_BOSS_RALLY && TGR_PLATFORM == TGR_PLATFORM_WIN
    /* The host of a network game decides the order of the race. */
    if (g_dwNetworkPlayMode) {
        for (i = 0; i < g_dwNumActiveVehicles; i++) {
            VEHICLE_SET(&g_aVehicles[i],
                        RacePosition,
                        GetNetworkRaceOrderEntry(VEHICLE_GET(&g_aVehicles[i], GridIndex)));
        }
        return;
    }
#endif /* TGR_BOSS_RALLY && TGR_PLATFORM == TGR_PLATFORM_WIN */
    for (i = 0; i < g_dwNumVehicles; i++) {
        slot = &g_aRacerSlots[i];
        if (RACER_SLOT_GET(slot, Status) & RACER_FINISHED) {
            continue;
        }
        entries[count].slot = i;
        entries[count].progress = RACER_SLOT_GET(slot, Vehicle) != NULL ?
                                      VEHICLE_GET(RACER_SLOT_GET(slot, Vehicle), Distance) :
                                      RACER_SLOT_GET(slot, Distance);
        count++;
    }
    if (count) {
        QuickSort(entries, count, sizeof(RacerProgress), CompareRacerProgressAsc);
    }
    /* Finished racers retain their places. */
    for (i = 0; i < count; i++) {
        slot = &g_aRacerSlots[entries[i].slot];
        if (RACER_SLOT_GET(slot, Vehicle) != NULL) {
            VEHICLE_SET(RACER_SLOT_GET(slot, Vehicle), RacePosition, g_dwNumVehicles - i - 1);
        } else {
            RACER_SLOT_SET(slot, RacePosition, g_dwNumVehicles - i - 1);
        }
    }
#endif
}
