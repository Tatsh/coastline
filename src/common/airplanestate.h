#pragma once

#include "common/vecmath.h"

#ifdef __cplusplus
extern "C" {
#endif

/** The aeroplane's position and flight path during a race. */
typedef struct {
    s32 instance;            /*!< The scene instance used to draw the aeroplane. */
    s32 trigger;             /*!< The trigger that starts a flight. */
    s32 triggerLap;          /*!< The lap on which the flight begins. */
    s32 flying;              /*!< Whether the aeroplane is in flight. */
    s32 node;                /*!< The current flight-path node. */
    s32 nodeCount;           /*!< The number of flight-path nodes. */
    f32 scale;               /*!< The scale applied to the scene instance. */
    Vec3f *pathLeft;         /*!< The left edge of the flight path. */
    Vec3f *pathRight;        /*!< The right edge of the flight path. */
    f32 distance;            /*!< The distance travelled along the current segment. */
    f32 segmentLength;       /*!< The length of the current path segment. */
    Vec3f previousDirection; /*!< The direction of the previous segment. */
    Vec3f direction;         /*!< The direction of the current segment. */
    Vec3f nextDirection;     /*!< The direction of the next segment. */
} AirplaneState;

/** Aeroplane flight state for the current race.
 * @ghidraAddress BRD3D: 0x10680618
 */
extern AirplaneState g_AirplaneState;

/** @cond */
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
/** @endcond */

#ifdef __cplusplus
}
#endif
