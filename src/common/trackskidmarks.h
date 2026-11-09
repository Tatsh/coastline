#ifndef COASTLINE_TRACKSKIDMARKS_H
#define COASTLINE_TRACKSKIDMARKS_H

#include "common/vecmath.h"
#include "common/vehiclefwd.h"

#ifdef __cplusplus
extern "C" {
#endif

/** The number of entries of g_anSkidMarkHead and g_anSkidMarkTail. */
#define SKID_MARK_TRACK_COUNT 32

#if TGR_PLATFORM == TGR_PLATFORM_SDL
/** Ring endpoints for each skid trail in one active engine runtime. */
typedef struct SdlSkidMarkState {
    int heads[SKID_MARK_TRACK_COUNT]; /*!< The first mark of each skid trail. */
    int tails[SKID_MARK_TRACK_COUNT]; /*!< The last mark of each skid trail. */
} SdlSkidMarkState;

/** @return Skid trail endpoints for the active engine runtime. */
SdlSkidMarkState *GetSdlSkidMarkState(void);
#define g_anSkidMarkHead (GetSdlSkidMarkState()->heads)
#define g_anSkidMarkTail (GetSdlSkidMarkState()->tails)
#else
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
#endif /* TGR_PLATFORM == TGR_PLATFORM_SDL */

/** Append vehicle shadows and skid trails to the deferred display list. */
void DrawTrackShadows(Vehicle *vehicles);

#if TGR_PLATFORM == TGR_PLATFORM_SDL
/** The number of segments in each trail's ring. */
#define SKID_MARK_SEGMENT_COUNT 500

/**
 * Lay the skid marks of the wheels of the vehicles of a drawn frame, as DrawTrackShadows() lays
 * them when skid marks are chosen.
 *
 * @param vehicles The vehicles the frame draws.
 */
void RecordFrameSkidMarks(Vehicle *vehicles);

/**
 * Visit the quads of the skid marks in the order DrawTrackShadows() draws them, each the right
 * and the left corner of the older segment and of the newer one. A quad is visited when the top bit
 * of the byte of the visibility mask of the instance under either of its segments is clear, the
 * instance in view and not near the eighth vehicle, until the quads fill the pool of the vertices
 * of the quads of the track every view of the frame draws from.
 *
 * @param visit Called with a stable ring slot, packed endpoint revisions, four corners, and
 * context.
 * @param context Passed to each call.
 * @param mask For each instance of the track, its byte of g_abInstanceVisibilityMask.
 * @param vertices The vertices of the pool the frame has taken, advanced by 4 for each quad.
 */
void VisitSkidMarkQuads(void (*visit)(unsigned slot,
                                      unsigned long long revision,
                                      const Vec3f *oldRight,
                                      const Vec3f *newRight,
                                      const Vec3f *oldLeft,
                                      const Vec3f *newLeft,
                                      void *context),
                        void *context,
                        const unsigned char *mask,
                        int *vertices);
#endif /* TGR_PLATFORM == TGR_PLATFORM_SDL */

#ifdef __cplusplus
}
#endif

#endif
