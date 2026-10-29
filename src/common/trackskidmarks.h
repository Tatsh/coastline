#ifndef COASTLINE_TRACKSKIDMARKS_H
#define COASTLINE_TRACKSKIDMARKS_H

#include "common/vehiclefwd.h"

#ifdef __cplusplus
extern "C" {
#endif

/** The number of entries of g_anSkidMarkHead and g_anSkidMarkTail. */
#define SKID_MARK_TRACK_COUNT 32

/**
 * The first skid mark of each track of skid marks.
 *
 * @ghidraAddress BRD3D: 0x10364308
 * @ghidraAddress BRGlide: 0x1035faf0
 */
extern int g_anSkidMarkHead[SKID_MARK_TRACK_COUNT];

/**
 * The last skid mark of each track of skid marks.
 *
 * @ghidraAddress BRD3D: 0x10363f68
 * @ghidraAddress BRGlide: 0x1035f750
 */
extern int g_anSkidMarkTail[SKID_MARK_TRACK_COUNT];

/** Append vehicle shadows and skid trails to the deferred display list. */
void DrawTrackShadows(Vehicle *vehicles);

#ifdef __cplusplus
}
#endif

#endif
