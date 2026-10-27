#include "driving.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#define ROUTE_LESS(a, b) (!((a) >= (b)))
#else
#define ROUTE_LESS(a, b) ((a) < (b))
#endif

void ComputeTrackPosition(Vehicle *vehicle) {
    f32 span;
    Vec3f fromTangent;
    Vec3f toTangent;
    TrackSection *fromSection;
    TrackSection *toSection;
    s32 from;
    s32 to;
    Vec3f fromMiddle;
    Vec3f toMiddle;
    Vec3f flat;
    Vec3f fromOffset;
    Vec3f toOffset;
    f32 fromAlong;
    f32 toAlong;
    f32 length;
    f32 t;
    f32 t2;
    f32 t3;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    long double highT, highSquare, highCube, sideOffset;
    f32 negativeTwiceCube;
#endif

    /* Tangent storage first records route direction and vehicle offset for segment selection. */
    span = vehicle->section->points[vehicle->pointIndex].distance -
           vehicle->section->points[vehicle->pointIndex + 1].distance;
    Vec3fSubtract(&fromTangent,
                  &vehicle->section->points[vehicle->pointIndex].left,
                  &vehicle->section->points[vehicle->pointIndex].position);
    Vec3fSubtract(&toTangent,
                  &vehicle->section->points[vehicle->pointIndex + 1].left,
                  &vehicle->section->points[vehicle->pointIndex + 1].position);
    Vec3fMidpoint(&toTangent, &fromTangent, &toTangent);
    /* The left of the route turned a quarter: back along it. */
    fromTangent.x = -toTangent.y;
    fromTangent.y = toTangent.x;
    fromTangent.z = 0.0f;
    Vec3fMidpoint(&toTangent,
                  &vehicle->section->points[vehicle->pointIndex].position,
                  &vehicle->section->points[vehicle->pointIndex + 1].position);
    Vec3fSubtract(&toTangent, &vehicle->frame.position, &toTangent);

    if (ROUTE_LESS(Vec3fDot(&fromTangent, &toTangent), 0.0f)) {
        fromSection = vehicle->section;
        from = vehicle->pointIndex;
        if (from + 1 == fromSection->pointCount) {
            toSection = fromSection->child;
            while (toSection->flags & TRACK_SECTION_ALTERNATE) {
                toSection = toSection->next;
            }
            to = 0;
        } else {
            toSection = fromSection;
            to = from + 1;
        }
    } else {
        /* The point before the first of a section is read from before its array. */
        fromSection = vehicle->section;
        from = vehicle->pointIndex - 1;
        toSection = vehicle->section;
        to = vehicle->pointIndex;
    }

    Vec3fDirection(
        &fromTangent, &fromSection->points[from].position, &fromSection->points[from + 1].position);
    Vec3fDirection(
        &toTangent, &toSection->points[to].position, &toSection->points[to + 1].position);
    Vec3fScaleInPlace(&fromTangent, span * 0.5f);
    Vec3fScaleInPlace(&toTangent, span * 0.5f);
    Vec3fMidpoint(
        &fromMiddle, &fromSection->points[from].position, &fromSection->points[from + 1].position);
    Vec3fMidpoint(&toMiddle, &toSection->points[to].position, &toSection->points[to + 1].position);
    flat.x = vehicle->frame.position.x;
    flat.y = vehicle->frame.position.y;
    flat.z = 0.0f;
    Vec3fSubtract(&fromOffset, &flat, &fromMiddle);
    Vec3fSubtract(&toOffset, &flat, &toMiddle);
    fromAlong = Vec3fDot(&fromOffset, &fromTangent);
    toAlong = -Vec3fDot(&toOffset, &toTangent);
    length = Vec3fDistance(&fromMiddle, &toMiddle);
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    /* Stores of t and its powers do not discard the extended values still on the x87 stack. */
    highT = (long double)fromAlong * length / ((long double)toAlong + fromAlong) / length;
    t = highT;
    highSquare = highT * t;
    t2 = highSquare;
    highCube = highSquare * t;
    t3 = highCube;
    negativeTwiceCube = highCube * -2.0f;
#else
    t = length * fromAlong / (fromAlong + toAlong) / length;
    t2 = t * t;
    t3 = t2 * t;
#endif

    /* A Hermite curve between the middles of the two segments. */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    Vec3fScale(
        &vehicle->trackPoint, &fromMiddle, 1.0L - (negativeTwiceCube - (long double)t2 * -3.0f));
    Vec3fMulAddInPlace(
        &vehicle->trackPoint, &toMiddle, negativeTwiceCube - (long double)t2 * -3.0f);
    Vec3fMulAddInPlace(
        &vehicle->trackPoint, &fromTangent, (long double)t3 - ((long double)t2 + t2) + t);
    Vec3fMulAddInPlace(&vehicle->trackPoint, &toTangent, (long double)t3 - t2);
#else
    Vec3fScale(&vehicle->trackPoint, &fromMiddle, t3 + t3 - 3.0f * t2 + 1.0f);
    Vec3fMulAddInPlace(&vehicle->trackPoint, &toMiddle, -2.0f * t3 + 3.0f * t2);
    Vec3fMulAddInPlace(&vehicle->trackPoint, &fromTangent, t3 - (t2 + t2) + t);
    Vec3fMulAddInPlace(&vehicle->trackPoint, &toTangent, t3 - t2);
#endif
    Vec3fLerp(&vehicle->trackForward, &toTangent, &fromTangent, t);
    Vec3fNormalize(&vehicle->trackForward);
    Vec3fCross(&vehicle->trackSide, &vehicle->trackUp, &vehicle->trackForward);
    Vec3fNormalize(&vehicle->trackSide);
    Vec3fCross(&vehicle->trackUp, &vehicle->trackForward, &vehicle->trackSide);
    Vec3fSubtract(&vehicle->trackOffset, &vehicle->frame.position, &vehicle->trackPoint);
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    sideOffset = Vec3fDot(&vehicle->trackSide, &vehicle->trackOffset);
    vehicle->trackSideOffset = sideOffset;
    vehicle->trackSideDistance = ROUTE_LESS(sideOffset, 0.0L) ? -sideOffset : sideOffset;
#else
    vehicle->trackSideOffset = Vec3fDot(&vehicle->trackSide, &vehicle->trackOffset);
    if (vehicle->trackSideOffset < 0.0f) {
        vehicle->trackSideDistance = -vehicle->trackSideOffset;
    } else {
        vehicle->trackSideDistance = vehicle->trackSideOffset;
    }
#endif
}
