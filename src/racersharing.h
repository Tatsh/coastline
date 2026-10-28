#ifndef COASTLINE_RACER_SHARING_H
#define COASTLINE_RACER_SHARING_H

#include "vehicle.h"

/* The progress of a racer for sorting. */
typedef struct {
    f32 progress; /*!< The sort key, the distance covered or a key derived from it. */
    s32 slot;     /*!< The index of the racer in g_aRacerSlots. */
} RacerProgress;

#ifdef __cplusplus
extern "C" {
#endif

/** Order racer sort keys from least to greatest.
 * @ghidraAddress USA: 0x80229510
 * @ghidraAddress EUR: 0x8022a888
 * @ghidraAddress JPN: 0x8022a040
 * @ghidraAddress BRD3D: 0x10066620
 * @ghidraAddress BRGlide: 0x1005f690
 */
s32 CompareRacerProgressAsc(const void *a, const void *b);

/** Assign computer vehicles to racers near a player. */
void ManageAiRacerSlots(Vehicle *player);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_RACER_SHARING_H */
