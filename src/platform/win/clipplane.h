#ifndef COASTLINE_WIN_CLIP_PLANE_H
#define COASTLINE_WIN_CLIP_PLANE_H

template <typename Polygon, typename Distance, typename Interpolate, typename Recycle>
static inline void ClipLinkedPolygonPlane(Polygon *pPolygon,
                                          Distance distance,
                                          Interpolate interpolate,
                                          Recycle recycle) {
    auto *pCurrent = pPolygon->pHead;
    auto *pNext = pCurrent->pNext;
    auto *pPrevious = pCurrent;
    decltype(pCurrent) pRemoved = nullptr;
    int nRemaining = pPolygon->nCount;
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

#endif
