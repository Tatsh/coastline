#ifndef COASTLINE_TRACKSKIDMARKS_H
#define COASTLINE_TRACKSKIDMARKS_H

#include "common/vecmath.h"
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

#if TGR_PLATFORM == TGR_PLATFORM_SDL
/**
 * Lay the skid marks of the wheels of the vehicles of a drawn frame, as DrawTrackShadows() lays
 * them when skid marks are chosen.
 *
 * @param vehicles The vehicles the frame draws.
 */
void RecordFrameSkidMarks(Vehicle *vehicles);

/**
 * Visit the quads of the skid marks in the order DrawTrackShadows() draws them, each the right
 * and the left corner of the older segment and of the newer one.
 *
 * @param visit Called with the four corners of a quad and the context.
 * @param context Passed to each call.
 */
void VisitSkidMarkQuads(void (*visit)(const Vec3f *oldRight,
                                      const Vec3f *newRight,
                                      const Vec3f *oldLeft,
                                      const Vec3f *newLeft,
                                      void *context),
                        void *context);
#endif /* TGR_PLATFORM == TGR_PLATFORM_SDL */

#ifdef __cplusplus
}
#endif

#endif
