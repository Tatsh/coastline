#include "common/driving.h"
#include "common/vehicleaccess.h"

#if TGR_PC
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
#if TGR_PC
    long double highT, highSquare, highCube, sideOffset;
    f32 negativeTwiceCube;
#endif

    /* Tangent storage first records route direction and vehicle offset for segment selection. */
    span = VEHICLE_GET(vehicle, Section)->points[VEHICLE_GET(vehicle, PointIndex)].distance -
           VEHICLE_GET(vehicle, Section)->points[VEHICLE_GET(vehicle, PointIndex) + 1].distance;
    Vec3fSubtract(
        &fromTangent,
        &VEHICLE_GET(vehicle, Section)->points[VEHICLE_GET(vehicle, PointIndex)].left,
        &VEHICLE_GET(vehicle, Section)->points[VEHICLE_GET(vehicle, PointIndex)].position);
    Vec3fSubtract(
        &toTangent,
        &VEHICLE_GET(vehicle, Section)->points[VEHICLE_GET(vehicle, PointIndex) + 1].left,
        &VEHICLE_GET(vehicle, Section)->points[VEHICLE_GET(vehicle, PointIndex) + 1].position);
    Vec3fMidpoint(&toTangent, &fromTangent, &toTangent);
    /* The left of the route turned a quarter: back along it. */
    fromTangent.x = -toTangent.y;
    fromTangent.y = toTangent.x;
    fromTangent.z = 0.0f;
    Vec3fMidpoint(
        &toTangent,
        &VEHICLE_GET(vehicle, Section)->points[VEHICLE_GET(vehicle, PointIndex)].position,
        &VEHICLE_GET(vehicle, Section)->points[VEHICLE_GET(vehicle, PointIndex) + 1].position);
    Vec3fSubtract(&toTangent, &VEHICLE_GET(vehicle, Frame).position, &toTangent);

    if (ROUTE_LESS(Vec3fDot(&fromTangent, &toTangent), 0.0f)) {
        fromSection = VEHICLE_GET(vehicle, Section);
        from = VEHICLE_GET(vehicle, PointIndex);
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
        fromSection = VEHICLE_GET(vehicle, Section);
        from = VEHICLE_GET(vehicle, PointIndex) - 1;
        toSection = VEHICLE_GET(vehicle, Section);
        to = VEHICLE_GET(vehicle, PointIndex);
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
    flat.x = VEHICLE_GET(vehicle, Frame).position.x;
    flat.y = VEHICLE_GET(vehicle, Frame).position.y;
    flat.z = 0.0f;
    Vec3fSubtract(&fromOffset, &flat, &fromMiddle);
    Vec3fSubtract(&toOffset, &flat, &toMiddle);
    fromAlong = Vec3fDot(&fromOffset, &fromTangent);
    toAlong = -Vec3fDot(&toOffset, &toTangent);
    length = Vec3fDistance(&fromMiddle, &toMiddle);
#if TGR_PC
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
#endif /* TGR_PC */

    /* A Hermite curve between the middles of the two segments. */
#if TGR_PC
    Vec3fScale(&VEHICLE_GET(vehicle, TrackPoint),
               &fromMiddle,
               1.0L - (negativeTwiceCube - (long double)t2 * -3.0f));
    Vec3fMulAddInPlace(
        &VEHICLE_GET(vehicle, TrackPoint), &toMiddle, negativeTwiceCube - (long double)t2 * -3.0f);
    Vec3fMulAddInPlace(&VEHICLE_GET(vehicle, TrackPoint),
                       &fromTangent,
                       (long double)t3 - ((long double)t2 + t2) + t);
    Vec3fMulAddInPlace(&VEHICLE_GET(vehicle, TrackPoint), &toTangent, (long double)t3 - t2);
#else
    Vec3fScale(&VEHICLE_GET(vehicle, TrackPoint), &fromMiddle, t3 + t3 - 3.0f * t2 + 1.0f);
    Vec3fMulAddInPlace(&VEHICLE_GET(vehicle, TrackPoint), &toMiddle, -2.0f * t3 + 3.0f * t2);
    Vec3fMulAddInPlace(&VEHICLE_GET(vehicle, TrackPoint), &fromTangent, t3 - (t2 + t2) + t);
    Vec3fMulAddInPlace(&VEHICLE_GET(vehicle, TrackPoint), &toTangent, t3 - t2);
#endif /* TGR_PC */
    Vec3fLerp(&VEHICLE_GET(vehicle, TrackForward), &toTangent, &fromTangent, t);
    Vec3fNormalize(&VEHICLE_GET(vehicle, TrackForward));
    Vec3fCross(&VEHICLE_GET(vehicle, TrackSide),
               &VEHICLE_GET(vehicle, TrackUp),
               &VEHICLE_GET(vehicle, TrackForward));
    Vec3fNormalize(&VEHICLE_GET(vehicle, TrackSide));
    Vec3fCross(&VEHICLE_GET(vehicle, TrackUp),
               &VEHICLE_GET(vehicle, TrackForward),
               &VEHICLE_GET(vehicle, TrackSide));
    Vec3fSubtract(&VEHICLE_GET(vehicle, TrackOffset),
                  &VEHICLE_GET(vehicle, Frame).position,
                  &VEHICLE_GET(vehicle, TrackPoint));
#if TGR_PC
    sideOffset = Vec3fDot(&VEHICLE_GET(vehicle, TrackSide), &VEHICLE_GET(vehicle, TrackOffset));
    VEHICLE_SET(vehicle, TrackSideOffset, sideOffset);
    VEHICLE_SET(
        vehicle, TrackSideDistance, ROUTE_LESS(sideOffset, 0.0L) ? -sideOffset : sideOffset);
#else
    vehicle->trackSideOffset =
        Vec3fDot(&VEHICLE_GET(vehicle, TrackSide), &VEHICLE_GET(vehicle, TrackOffset));
    if (vehicle->trackSideOffset < 0.0f) {
        vehicle->trackSideDistance = -vehicle->trackSideOffset;
    } else {
        vehicle->trackSideDistance = vehicle->trackSideOffset;
    }
#endif /* TGR_PC */
}
