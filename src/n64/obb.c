#include "common/physics.h"

#define OBB_COMPONENT_ABS(value) ((value) < 0.0f ? -(value) : (value))

s32 TestObbIntersection(f32 rotation[3][3], Vec3f *offset, Vec3f *halfA, Vec3f *halfB) {
    int overlap;
    float absoluteProjection;
    float projection;
    float absoluteRotation[9];
    int faceOverlap;

    absoluteRotation[0] = OBB_COMPONENT_ABS(rotation[0][0]);
    absoluteRotation[1] = OBB_COMPONENT_ABS(rotation[0][1]);
    absoluteRotation[2] = OBB_COMPONENT_ABS(rotation[0][2]);
    absoluteRotation[3] = OBB_COMPONENT_ABS(rotation[1][0]);
    absoluteRotation[4] = OBB_COMPONENT_ABS(rotation[1][1]);
    absoluteRotation[5] = OBB_COMPONENT_ABS(rotation[1][2]);
    absoluteRotation[6] = OBB_COMPONENT_ABS(rotation[2][0]);
    absoluteRotation[7] = OBB_COMPONENT_ABS(rotation[2][1]);
    absoluteRotation[8] = OBB_COMPONENT_ABS(rotation[2][2]);
    overlap = 1;

    absoluteProjection = OBB_COMPONENT_ABS(offset->x);
    faceOverlap = absoluteProjection <= halfA->x + halfB->x * absoluteRotation[0] +
                                            halfB->y * absoluteRotation[1] +
                                            halfB->z * absoluteRotation[2];
    overlap &= faceOverlap;
    projection =
        offset->x * rotation[0][0] + offset->y * rotation[1][0] + offset->z * rotation[2][0];
    absoluteProjection = OBB_COMPONENT_ABS(projection);
    faceOverlap = absoluteProjection <= halfB->x + halfA->x * absoluteRotation[0] +
                                            halfA->y * absoluteRotation[3] +
                                            halfA->z * absoluteRotation[6];
    overlap &= faceOverlap;
    absoluteProjection = OBB_COMPONENT_ABS(offset->y);
    faceOverlap = absoluteProjection <= halfA->y + halfB->x * absoluteRotation[3] +
                                            halfB->y * absoluteRotation[4] +
                                            halfB->z * absoluteRotation[5];
    overlap &= faceOverlap;
    absoluteProjection = OBB_COMPONENT_ABS(offset->z);
    faceOverlap = absoluteProjection <= halfA->z + halfB->x * absoluteRotation[6] +
                                            halfB->y * absoluteRotation[7] +
                                            halfB->z * absoluteRotation[8];
    overlap &= faceOverlap;
    projection =
        offset->x * rotation[0][1] + offset->y * rotation[1][1] + offset->z * rotation[2][1];
    absoluteProjection = OBB_COMPONENT_ABS(projection);
    faceOverlap = absoluteProjection <= halfB->y + halfA->x * absoluteRotation[1] +
                                            halfA->y * absoluteRotation[4] +
                                            halfA->z * absoluteRotation[7];
    overlap &= faceOverlap;
    projection =
        offset->x * rotation[0][2] + offset->y * rotation[1][2] + offset->z * rotation[2][2];
    absoluteProjection = OBB_COMPONENT_ABS(projection);
    faceOverlap = absoluteProjection <= halfB->z + halfA->x * absoluteRotation[2] +
                                            halfA->y * absoluteRotation[5] +
                                            halfA->z * absoluteRotation[8];
    overlap &= faceOverlap;

    projection = offset->z * rotation[1][0] - offset->y * rotation[2][0];
    absoluteProjection = OBB_COMPONENT_ABS(projection);
    overlap &=
        absoluteProjection <= halfA->y * absoluteRotation[6] + halfA->z * absoluteRotation[3] +
                                  halfB->y * absoluteRotation[2] + halfB->z * absoluteRotation[1];
    projection = offset->z * rotation[1][1] - offset->y * rotation[2][1];
    absoluteProjection = OBB_COMPONENT_ABS(projection);
    overlap &=
        absoluteProjection <= halfA->y * absoluteRotation[7] + halfA->z * absoluteRotation[4] +
                                  halfB->x * absoluteRotation[2] + halfB->z * absoluteRotation[0];
    projection = offset->z * rotation[1][2] - offset->y * rotation[2][2];
    absoluteProjection = OBB_COMPONENT_ABS(projection);
    overlap &=
        absoluteProjection <= halfA->y * absoluteRotation[8] + halfA->z * absoluteRotation[5] +
                                  halfB->x * absoluteRotation[1] + halfB->y * absoluteRotation[0];
    projection = offset->x * rotation[2][0] - offset->z * rotation[0][0];
    absoluteProjection = OBB_COMPONENT_ABS(projection);
    overlap &=
        absoluteProjection <= halfA->x * absoluteRotation[6] + halfA->z * absoluteRotation[0] +
                                  halfB->y * absoluteRotation[5] + halfB->z * absoluteRotation[4];
    projection = offset->x * rotation[2][1] - offset->z * rotation[0][1];
    absoluteProjection = OBB_COMPONENT_ABS(projection);
    overlap &=
        absoluteProjection <= halfA->x * absoluteRotation[7] + halfA->z * absoluteRotation[1] +
                                  halfB->x * absoluteRotation[5] + halfB->z * absoluteRotation[3];
    projection = offset->x * rotation[2][2] - offset->z * rotation[0][2];
    absoluteProjection = OBB_COMPONENT_ABS(projection);
    overlap &=
        absoluteProjection <= halfA->x * absoluteRotation[8] + halfA->z * absoluteRotation[2] +
                                  halfB->x * absoluteRotation[4] + halfB->y * absoluteRotation[3];
    projection = offset->y * rotation[0][0] - offset->x * rotation[1][0];
    absoluteProjection = OBB_COMPONENT_ABS(projection);
    overlap &=
        absoluteProjection <= halfA->x * absoluteRotation[3] + halfA->y * absoluteRotation[0] +
                                  halfB->y * absoluteRotation[8] + halfB->z * absoluteRotation[7];
    projection = offset->y * rotation[0][1] - offset->x * rotation[1][1];
    absoluteProjection = OBB_COMPONENT_ABS(projection);
    overlap &=
        absoluteProjection <= halfA->x * absoluteRotation[4] + halfA->y * absoluteRotation[1] +
                                  halfB->x * absoluteRotation[8] + halfB->z * absoluteRotation[6];
    projection = offset->y * rotation[0][2] - offset->x * rotation[1][2];
    absoluteProjection = OBB_COMPONENT_ABS(projection);
    overlap &=
        absoluteProjection <= halfA->x * absoluteRotation[5] + halfA->y * absoluteRotation[2] +
                                  halfB->x * absoluteRotation[7] + halfB->y * absoluteRotation[6];
    return overlap;
}
