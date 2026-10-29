#ifndef COASTLINE_SHADOWBUFFERS_H
#define COASTLINE_SHADOWBUFFERS_H

#include "common/displaylistops.h"

#if TGR_BOSS_RALLY
#include "common/convertedvertex.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if TGR_BOSS_RALLY
#define SHADOW_COMMANDS 10000
#else
#define SHADOW_COMMANDS 500
#endif
#define SHADOW_VERTICES 1000

/**
 * @ghidraAddress USA: 0x80353580
 * @ghidraAddress EUR: 0x80355a80
 * @ghidraAddress JPN: 0x803546f0
 * @ghidraAddress BRD3D: 0x103643c0
 * @ghidraAddress BRGlide: 0x1035fba8
 */
extern DisplayListCommand g_aShadowDisplayLists[2][SHADOW_COMMANDS];

#if TGR_BOSS_RALLY
/**
 * @ghidraAddress BRD3D: 0x1038bcc0
 * @ghidraAddress BRGlide: 0x103874a8
 */
extern ConvertedModelVertex g_aShadowVertices[2][SHADOW_VERTICES];

#define TRACK_QUAD_VERTICES 8000

/**
 * @ghidraAddress BRD3D: 0x102e5f28
 * @ghidraAddress BRGlide: 0x102e1710
 */
extern ConvertedModelVertex g_aTrackQuadVertices[2][TRACK_QUAD_VERTICES];

/**
 * @ghidraAddress BRD3D: 0x102e5ec4
 * @ghidraAddress BRGlide: 0x102e16ac
 */
extern ConvertedModelVertex *g_pTrackQuadVertexStart;

/**
 * @ghidraAddress BRD3D: 0x10363ff4
 * @ghidraAddress BRGlide: 0x1035f7dc
 */
extern ConvertedModelVertex *g_pTrackQuadVertexWritePtr;
#else
/**
 * @ghidraAddress USA: 0x803554c0
 * @ghidraAddress EUR: 0x803579c0
 * @ghidraAddress JPN: 0x80356630
 */
extern Vtx g_aShadowVertices[2][SHADOW_VERTICES];
#endif /* TGR_BOSS_RALLY */

/**
 * Start the display lists and vertices of the shadows of the frame.
 *
 * @ghidraAddress USA: 0x80233fdc
 * @ghidraAddress EUR: 0x802354ec
 * @ghidraAddress JPN: 0x802352ec
 * @ghidraAddress BRD3D: 0x1000f5c0
 * @ghidraAddress BRGlide: 0x1000cb20
 */
void ResetShadowBuffers(void);

/**
 * The next command of the shadows, and their first this frame.
 *
 * @ghidraAddress USA: 0x8028c75c
 * @ghidraAddress EUR: 0x8028eebc
 * @ghidraAddress JPN: 0x8028e90c
 * @ghidraAddress BRD3D: 0x10363ff0
 * @ghidraAddress BRGlide: 0x1035f7d8
 */
extern DisplayListCommand *g_pShadowDisplayListWritePtr;

/**
 * @ghidraAddress USA: 0x8028c760
 * @ghidraAddress EUR: 0x8028eec0
 * @ghidraAddress JPN: 0x8028e910
 * @ghidraAddress BRD3D: 0x102e5ec8
 * @ghidraAddress BRGlide: 0x102e16b0
 */
extern DisplayListCommand *g_pShadowDisplayListStart;

/**
 * The next vertex of the shadows, and their first this frame.
 *
 * @ghidraAddress USA: 0x8028c764
 * @ghidraAddress EUR: 0x8028eec4
 * @ghidraAddress JPN: 0x8028e914
 * @ghidraAddress BRD3D: 0x10364304
 * @ghidraAddress BRGlide: 0x1035faec
 */
#if TGR_BOSS_RALLY
extern ConvertedModelVertex *g_pShadowVertexWritePtr;
#else
extern Vtx *g_pShadowVertexWritePtr;
#endif

/**
 * @ghidraAddress USA: 0x8028c768
 * @ghidraAddress EUR: 0x8028eec8
 * @ghidraAddress JPN: 0x8028e918
 * @ghidraAddress BRD3D: 0x103643bc
 * @ghidraAddress BRGlide: 0x1035fba4
 */
#if TGR_BOSS_RALLY
extern ConvertedModelVertex *g_pShadowVertexStart;
#else
extern Vtx *g_pShadowVertexStart;
#endif

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_SHADOWBUFFERS_H */
