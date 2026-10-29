#ifndef COASTLINE_RACE_PROGRESS_H
#define COASTLINE_RACE_PROGRESS_H

#include "common/racers.h"

#if TGR_BOSS_RALLY
#include "common/assetnames.h"
#include "common/strings.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if TGR_BOSS_RALLY
/** Look up a localised race message. */
static inline const char *GetRaceProgressMessage(s32 id) {
    return GetStringResource(id);
}
/** The empty message shown after a network finish. */
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
