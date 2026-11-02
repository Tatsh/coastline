#ifndef COASTLINE_CLIP_PLANE_H
#define COASTLINE_CLIP_PLANE_H

#include <concepts>
#include <type_traits>

/**
 * Require a circular vertex list, signed distances, interpolation, and recycling operations.
 * @tparam Polygon A polygon with a head pointer and integer vertex count.
 * @tparam Distance A callable returning a vertex's signed distance from a clipping plane.
 * @tparam Interpolate A callable allocating an interpolated vertex.
 * @tparam Recycle A callable releasing a vertex.
 */
template <typename Polygon, typename Distance, typename Interpolate, typename Recycle>
concept LinkedPolygonClipping = requires(Polygon *polygon,
                                         decltype(polygon->pHead) vertex,
                                         Distance distance,
                                         Interpolate interpolate,
                                         Recycle recycle) {
    requires std::is_pointer_v<decltype(polygon->pHead)>;
    { polygon->nCount } -> std::same_as<int &>;
    { vertex->pNext } -> std::same_as<decltype(polygon->pHead) &>;
    { distance(vertex) } -> std::convertible_to<float>;
    { interpolate(vertex, vertex, 0.0f) } -> std::same_as<decltype(polygon->pHead)>;
    { recycle(vertex) } -> std::same_as<void>;
};

/**
 * Clip a circular vertex list to the non-negative side of a plane.
 * With patches enabled, failed interpolation discards the polygon and recycles every node.
 * @tparam Polygon A type satisfying LinkedPolygonClipping.
 * @tparam Distance The distance callback type.
 * @tparam Interpolate The interpolation callback type.
 * @tparam Recycle The recycling callback type.
 * @param pPolygon The circular polygon, updated in place.
 * @param distance Compute a signed distance for a vertex.
 * @param interpolate Allocate and interpolate an intersection vertex, or return null on failure.
 * @param recycle Release an unused vertex.
 */
template <typename Polygon, typename Distance, typename Interpolate, typename Recycle>
    requires LinkedPolygonClipping<Polygon, Distance, Interpolate, Recycle>
static inline void ClipLinkedPolygonPlane(Polygon *pPolygon,
                                          Distance distance,
                                          Interpolate interpolate,
                                          Recycle recycle) {
#ifdef ENABLE_PATCHES
    if (pPolygon->pHead == nullptr || pPolygon->nCount <= 0) {
        pPolygon->nCount = 0;
        return;
    }
#endif
    auto *pCurrent = pPolygon->pHead;
    auto *pNext = pCurrent->pNext;
    auto *pPrevious = pCurrent;
    decltype(pCurrent) pRemoved = nullptr;
    int nRemaining = pPolygon->nCount;
#ifdef ENABLE_PATCHES
    auto discard = [&] {
        auto *head = pPrevious->pNext;
        auto *vertex = head;
        do {
            auto *next = vertex->pNext;
            recycle(vertex);
            vertex = next;
        } while (vertex != head);
        while (pRemoved) {
            auto *removed = pRemoved;
            pRemoved = removed->pNext;
            recycle(removed);
        }
        pPolygon->pHead = nullptr;
        pPolygon->nCount = 0;
    };
#endif
    while (nRemaining > 0) {
        float fNextDistance = distance(pNext);
        float fCurrentDistance = distance(pCurrent);
        auto *pFollowing = pNext->pNext;
        if (fNextDistance >= 0.0f) {
            if (fCurrentDistance >= 0.0f) {
                pPrevious = pNext;
            } else {
                float fFraction = static_cast<float>(
                    fCurrentDistance /
                    (static_cast<long double>(fCurrentDistance) - fNextDistance));
                auto *pIntersection = interpolate(pCurrent, pNext, fFraction);
#ifdef ENABLE_PATCHES
                if (pIntersection == nullptr) {
                    discard();
                    return;
                }
#endif
                pIntersection->pNext = pPrevious->pNext;
                pPrevious->pNext = pIntersection;
                pPrevious = pNext;
                ++pPolygon->nCount;
            }
        } else {
            if (pPrevious->pNext) {
                pPrevious->pNext = pPrevious->pNext->pNext;
            }
            pNext->pNext = pRemoved;
            pRemoved = pNext;
            if (fCurrentDistance >= 0.0f) {
                float fFraction = static_cast<float>(
                    fNextDistance / (static_cast<long double>(fNextDistance) - fCurrentDistance));
                auto *pIntersection = interpolate(pNext, pCurrent, fFraction);
#ifdef ENABLE_PATCHES
                if (pIntersection == nullptr) {
                    discard();
                    return;
                }
#endif
                pIntersection->pNext = pPrevious->pNext;
                pPrevious->pNext = pIntersection;
                pPrevious = pIntersection;
            } else {
                --pPolygon->nCount;
            }
        }
        pCurrent = pNext;
        pNext = pFollowing;
        if (pPolygon->nCount < 2) {
            break;
        }
        --nRemaining;
    }
    pPolygon->pHead = pNext;
    while (pRemoved) {
        auto *pVertex = pRemoved;
        pRemoved = pRemoved->pNext;
        recycle(pVertex);
    }
}

#endif /* COASTLINE_CLIP_PLANE_H */
