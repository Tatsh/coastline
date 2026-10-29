#ifndef COASTLINE_PLATFORM_WIN_TRACKSKIDMARKS_H
#define COASTLINE_PLATFORM_WIN_TRACKSKIDMARKS_H

#include "common/vehiclefwd.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Append vehicle shadows and skid trails to the deferred display list. */
void DrawTrackShadows(Vehicle *vehicles);

#ifdef __cplusplus
}
#endif

#endif
