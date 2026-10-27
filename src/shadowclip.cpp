#include "shadowclip.h"

#include <functional>

#include "platform/win/clipplane.h"
#include "trackshadow.h"

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

static inline ShadowClipVertex *AllocateProjectedShadowVertex(const ConvertedModelVertex *source,
                                                              ShadowClipVertex *next) {
    ShadowClipVertex *result = g_pShadowClipVertexFreeList;

    if (result != nullptr) {
        g_pShadowClipVertexFreeList = result->pNext;
    }
    result->pVertex = &result->vertex;
    result->pVertex->position = source->position;
    result->pVertex->normal = source->normal;
    ProjectShadowTextureCoordinates(result->pVertex);
    result->pNext = next;
    return result;
}

void ProjectClippedShadowTriangle(const TrackInstance *instance,
                                  ConvertedModelVertex quad[4],
                                  s32 cornerValid[4],
                                  const ConvertedModelVertex *a,
                                  const ConvertedModelVertex *b,
                                  const ConvertedModelVertex *c,
                                  const f32 vehicleMatrix[4][4]) {
    ShadowClipVertex *last = AllocateProjectedShadowVertex(c, nullptr);
    ShadowClipVertex *middle = AllocateProjectedShadowVertex(b, last);
    ShadowClipPolygon polygon = {AllocateProjectedShadowVertex(a, middle), 3};

    last->pNext = polygon.pHead;
    ClipShadowPolygon(&polygon, ShadowTextureDistanceLeft);
    if (polygon.nCount >= 2) {
        ClipShadowPolygon(&polygon, ShadowTextureDistanceRight);
    }
    if (polygon.nCount >= 2) {
        ClipShadowPolygon(&polygon, ShadowTextureDistanceTop);
    }
    if (polygon.nCount >= 2) {
        ClipShadowPolygon(&polygon, ShadowTextureDistanceBottom);
    }
    const bool projectCorners = polygon.nCount >= 2;
    for (s32 index = 0; index < polygon.nCount; ++index) {
        ShadowClipVertex *vertex = polygon.pHead;
        polygon.pHead = vertex->pNext;
        if (projectCorners) {
            UpdateShadowCornerVertex(
                instance, quad, cornerValid, 0, vertex->pVertex, 0.0f, 0.0f, vehicleMatrix);
            UpdateShadowCornerVertex(
                instance, quad, cornerValid, 1, vertex->pVertex, 1024.0f, 0.0f, vehicleMatrix);
            UpdateShadowCornerVertex(
                instance, quad, cornerValid, 2, vertex->pVertex, 0.0f, 1024.0f, vehicleMatrix);
            UpdateShadowCornerVertex(
                instance, quad, cornerValid, 3, vertex->pVertex, 1024.0f, 1024.0f, vehicleMatrix);
        }
        RecycleShadowClipVertex(vertex);
    }
}
