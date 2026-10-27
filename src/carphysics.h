#ifndef COASTLINE_CARPHYSICS_H
#define COASTLINE_CARPHYSICS_H

#include "vecmath.h"

/** The gears of a gearbox, neutral first. */
#define CAR_GEAR_RATIOS 7

/** The physics of a car. */
typedef struct {
    f32 gearRatios[CAR_GEAR_RATIOS]; /*!< The ratio of each gear, from 0 for neutral. */
    f32 torqueCurve[4]; /*!< The torque of the engine as a cubic of its revolutions, the highest
                             power first. */
    f32 finalDrive;     /*!< The ratio of the final drive. */
    s32 gearCount;      /*!< The number of forward gears. */
    s32 rearWheelDrive; /*!< Whether the throttle drives the rear wheels alone, rather than all
                             four. */
    Vec3f boxSize;      /*!< The size of the box the track collides with. */
    f32 boxCenterZ;     /*!< The height of the middle of the box. */
    s32 tableIndex;     /*!< The class of the car among the arcade times of each track. */
} CarPhysicsSpec;

#endif /* COASTLINE_CARPHYSICS_H */
