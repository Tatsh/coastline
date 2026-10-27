#ifndef COASTLINE_RACE_PROGRESS_H
#define COASTLINE_RACE_PROGRESS_H

#include "racers.h"

#ifdef __cplusplus
extern "C" {
#endif

#if TGR_PLATFORM == TGR_PLATFORM_WIN
/** Look up a localised race message. */
const char *GetRaceProgressMessage(s32 id);
/** The empty message shown after a network finish. */
const char *GetNetworkRaceFinishMessage(void);
/**
 * The resource identifiers for the finishing places.
 *
 * @ghidraAddress BRD3D: 0x100b3960
 */
extern s32 g_anRaceProgressStringIds[RACER_SLOTS];
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_RACE_PROGRESS_H */
