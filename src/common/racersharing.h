#ifndef COASTLINE_RACER_SHARING_H
#define COASTLINE_RACER_SHARING_H

/**
 * @file
 * The sharing of the vehicles of the computer between the racers nearest a player.
 */

#include "common/vehicle.h"

/** The progress of a racer for sorting. */
typedef struct {
    f32 progress; /*!< The sort key, the distance covered or a key derived from it. */
    s32 slot;     /*!< The index of the racer in g_aRacerSlots. */
} RacerProgress;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Order racer sort keys from least to greatest, for QuickSort().
 *
 * @param a The first RacerProgress.
 * @param b The second RacerProgress.
 * @return 1 when the key of @p a is greater, -1 when it is less, and 0 otherwise.
 * @ghidraAddress USA: 0x80229510
 * @ghidraAddress Asia: 0x80229060
 * @ghidraAddress EUR: 0x8022a888
 * @ghidraAddress JPN: 0x8022a040
 * @ghidraAddress BRD3D: 0x10066620
 * @ghidraAddress BRGlide: 0x1005f690
 */
s32 CompareRacerProgressAsc(const void *a, const void *b);

/**
 * Assign computer vehicles to racers near a player. A computer vehicle with no cooldown and a sort
 * key above 80000 returns its state to its racer, and the racers nearest the player take the
 * freed vehicles.
 *
 * @param player The vehicle of the player the racers are sorted around.
 * @ghidraAddress USA: 0x80229700
 * @ghidraAddress Asia: 0x80229250
 * @ghidraAddress EUR: 0x8022aa78
 * @ghidraAddress JPN: 0x8022a230
 */
void ManageAiRacerSlots(Vehicle *player);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_RACER_SHARING_H */
