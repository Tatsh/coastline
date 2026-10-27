#include "shadowclip.h"

#include <functional>

#include "platform/win/clipplane.h"

static inline f32 LerpShadowTextureComponent(f32 first, f32 second, f32 fraction) {
    return ((long double)second - first) * fraction + first;
}

ShadowClipVertex *
LerpShadowClipVertex(const ShadowClipVertex *first, const ShadowClipVertex *second, f32 fraction) {
    ShadowClipVertex *result = g_pShadowClipVertexFreeList;

    if (result != nullptr) {
        g_pShadowClipVertexFreeList = result->pNext;
    }
    result->pVertex = &result->vertex;
    Vec3fLerp(&result->pVertex->position,
              &second->pVertex->position,
              &first->pVertex->position,
              fraction);
    result->pVertex->texture.x =
        LerpShadowTextureComponent(first->pVertex->texture.x, second->pVertex->texture.x, fraction);
    result->pVertex->texture.y =
        LerpShadowTextureComponent(first->pVertex->texture.y, second->pVertex->texture.y, fraction);
    Vec3fLerp(
        &result->pVertex->normal, &second->pVertex->normal, &first->pVertex->normal, fraction);
    return result;
}

static void RecycleShadowClipVertex(ShadowClipVertex *vertex) {
    if (std::greater_equal<>{}(vertex, g_aShadowClipVertexPool) &&
        std::less<>{}(vertex, g_aShadowClipVertexPool + SHADOW_CLIP_VERTEX_COUNT)) {
        vertex->pNext = g_pShadowClipVertexFreeList;
        g_pShadowClipVertexFreeList = vertex;
    }
}

void ClipShadowPolygon(ShadowClipPolygon *polygon, ShadowClipDistance distance) {
    ClipLinkedPolygonPlane(
        polygon,
        [distance](const ShadowClipVertex *vertex) { return distance(vertex->pVertex); },
        LerpShadowClipVertex,
        RecycleShadowClipVertex);
}
