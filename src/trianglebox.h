#ifndef COASTLINE_TRIANGLEBOX_H
#define COASTLINE_TRIANGLEBOX_H

#include "physics.h"

/* Move a triangle into the unit cube of a box, and find its normal there. */
static inline void TriangleInBox(Vec3f corners[3], Vec3f *normal, f32 box[4][4], TrackTriangle *t) {
    Vec3f e1, e2;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    long double highE2[3];
#endif

    Mtx4x4TransformPoint(&corners[0], box, t->vertices[0]);
    Mtx4x4TransformPoint(&corners[1], box, t->vertices[1]);
    Mtx4x4TransformPoint(&corners[2], box, t->vertices[2]);
    e1.x = corners[1].x - corners[0].x;
    e1.y = corners[1].y - corners[0].y;
    e1.z = corners[1].z - corners[0].z;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    highE2[0] = (long double)corners[2].x - corners[0].x;
    highE2[1] = (long double)corners[2].y - corners[0].y;
    highE2[2] = (long double)corners[2].z - corners[0].z;
    e2.x = highE2[0];
    e2.y = highE2[1];
    e2.z = highE2[2];
#else
    e2.x = corners[2].x - corners[0].x;
    e2.y = corners[2].y - corners[0].y;
    e2.z = corners[2].z - corners[0].z;
#endif
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    /* The normal mixes retained second-edge values with their stored float copies. */
    normal->x = e1.y * highE2[2] - highE2[1] * e1.z;
    normal->y = e1.z * highE2[0] - (long double)e2.z * e1.x;
    normal->z = (long double)e1.x * e2.y - (long double)e2.x * e1.y;
#else
    normal->x = e1.y * e2.z - e2.y * e1.z;
    normal->y = e1.z * e2.x - e2.z * e1.x;
    normal->z = e1.x * e2.y - e2.x * e1.y;
#endif
}

#endif
