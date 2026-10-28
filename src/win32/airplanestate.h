#pragma once

#include "vecmath.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    s32 instance;
    s32 trigger;
    s32 triggerLap;
    s32 flying;
    s32 node;
    s32 nodeCount;
    f32 scale;
    Vec3f *pathLeft;
    Vec3f *pathRight;
    f32 distance;
    f32 segmentLength;
    Vec3f previousDirection;
    Vec3f direction;
    Vec3f nextDirection;
} AirplaneState;

/** @ghidraAddress BRD3D: 0x10680618 */
extern AirplaneState g_AirplaneState;

/** @ghidraAddress BRD3D: 0x10680618 */
#define g_dwAirplaneInstance (g_AirplaneState.instance)
/** @ghidraAddress BRD3D: 0x1068061c */
#define g_dwAirplaneTrigger (g_AirplaneState.trigger)
/** @ghidraAddress BRD3D: 0x10680620 */
#define g_dwAirplaneTriggerLap (g_AirplaneState.triggerLap)
/** @ghidraAddress BRD3D: 0x10680624 */
#define g_bAirplaneFlying (g_AirplaneState.flying)
/** @ghidraAddress BRD3D: 0x10680628 */
#define g_dwAirplaneNode (g_AirplaneState.node)
/** @ghidraAddress BRD3D: 0x1068062c */
#define g_dwAirplaneNodeCount (g_AirplaneState.nodeCount)
/** @ghidraAddress BRD3D: 0x10680630 */
#define g_fAirplaneScale (g_AirplaneState.scale)
/** @ghidraAddress BRD3D: 0x10680634 */
#define g_pAirplanePathLeft (g_AirplaneState.pathLeft)
/** @ghidraAddress BRD3D: 0x10680638 */
#define g_pAirplanePathRight (g_AirplaneState.pathRight)
/** @ghidraAddress BRD3D: 0x1068063c */
#define g_fAirplaneDistance (g_AirplaneState.distance)
/** @ghidraAddress BRD3D: 0x10680640 */
#define g_fAirplaneSegmentLength (g_AirplaneState.segmentLength)
/** @ghidraAddress BRD3D: 0x10680644 */
#define g_vAirplaneDirPrev (g_AirplaneState.previousDirection)
/** @ghidraAddress BRD3D: 0x10680650 */
#define g_vAirplaneDir (g_AirplaneState.direction)
/** @ghidraAddress BRD3D: 0x1068065c */
#define g_vAirplaneDirNext (g_AirplaneState.nextDirection)

#ifdef __cplusplus
}
#endif
