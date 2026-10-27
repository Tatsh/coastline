#ifndef COASTLINE_SHADOW_CLIP_H
#define COASTLINE_SHADOW_CLIP_H

#include "convertedvertex.h"

/** A clipping link with a pointer to its attributes and inline vertex storage. */
typedef struct ShadowClipVertex {
    struct ShadowClipVertex *pNext;
    ConvertedModelVertex *pVertex;
    ConvertedModelVertex vertex;
} ShadowClipVertex;

/** A circular list of shadow vertices being clipped to a texture boundary. */
typedef struct ShadowClipPolygon {
    ShadowClipVertex *pHead;
    s32 nCount;
} ShadowClipPolygon;

#ifdef __cplusplus
extern "C" {
#endif

/** The number of entries of g_aShadowClipVertexPool. */
#define SHADOW_CLIP_VERTEX_COUNT 64

/**
 * The pool of the vertices of the clipping of shadows.
 *
 * @ghidraAddress BRD3D: 0x102e54c0
 * @ghidraAddress BRGlide: 0x102e0ca8
 */
extern ShadowClipVertex g_aShadowClipVertexPool[SHADOW_CLIP_VERTEX_COUNT];

/**
 * The first free vertex of g_aShadowClipVertexPool.
 *
 * @ghidraAddress BRD3D: 0x102e5ecc
 * @ghidraAddress BRGlide: 0x102e16b4
 */
extern ShadowClipVertex *g_pShadowClipVertexFreeList;

typedef long double (*ShadowClipDistance)(const ConvertedModelVertex *vertex);

/**
 * Take a clipping node from the free list and interpolate all eight vertex components.
 *
 * @param first The vertex at fraction 0.
 * @param second The vertex at fraction 1.
 * @param fraction The weight of the second vertex.
 * @return The allocated node.
 * @ghidraAddress BRD3D: 0x10010b00
 */
ShadowClipVertex *
LerpShadowClipVertex(const ShadowClipVertex *first, const ShadowClipVertex *second, f32 fraction);

/**
 * Clip a circular polygon against a signed texture-boundary distance.
 *
 * @param polygon The circular polygon to modify.
 * @param distance The signed distance callback.
 * @ghidraAddress BRD3D: 0x100109a0
 */
void ClipShadowPolygon(ShadowClipPolygon *polygon, ShadowClipDistance distance);

#ifdef __cplusplus
}
#endif

#endif
