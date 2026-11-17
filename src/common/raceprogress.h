#ifndef COASTLINE_RACE_PROGRESS_H
#define COASTLINE_RACE_PROGRESS_H

/**
 * @file
 * The messages of the progress and the finishing places of a race in the PC release.
 */

#include "common/racers.h"

#if TGR_BOSS_RALLY
#include "common/assetnames.h"
#include "common/strings.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if TGR_BOSS_RALLY
/**
 * Look up a localised race message.
 *
 * @param id The string resource identifier.
 * @return The text of the string resource.
 */
static inline const char *GetRaceProgressMessage(s32 id) {
    return GetStringResource(id);
}
/**
 * Get the empty message shown after a network finish.
 *
 * @return The empty user interface string.
 */
static inline const char *GetNetworkRaceFinishMessage(void) {
    return g_szEmptyUiString;
}
/**
 * The resource identifiers for the finishing places.
 *
 * @ghidraAddress BRD3D: 0x100b3960
 */
extern s32 g_anRaceProgressStringIds[RACER_SLOTS];
#endif /* TGR_BOSS_RALLY */

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_RACE_PROGRESS_H */
