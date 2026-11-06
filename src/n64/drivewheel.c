#include <PR/gu.h>

#include "common/physics.h"
#include "common/track.h"

#define WHEEL_ABS(x) ((x) < 0.0f ? -(x) : (x))
#define WHEEL_SIGN(x) ((x) == 0.0f ? 0.0 : ((x) > 0 ? 1.0 : -1.0))
typedef union {
    f32 components[3];
    Vec3f vector;
} WheelVector;

void DriveWheel(RigidBody *body, RigidBody *wheel, f32 *loadOut, u8 *sliding, f32 dt) {
    WheelVector axle;
    f32 forward[3];
    f32 side[3];
    WheelVector push;
    f32 roll[3];
    f32 slide[3];
    WheelVector velocity;
    f32 rollSpeed;
    f32 sine;
    f32 cosine;
    f32 normalLoad;
    TrackTriangle *contact;
    f32 force;
    /* Unused slots preserve the original IDO stack layout. */
    s32 unusedBeforeAxis[2];
    f32 drive;
    f32 height;
    WheelVector axis = {{0.0f, 1.0f, 0.0f}};
    s32 unusedAfterAxis[2];

    (void)unusedBeforeAxis;
    (void)unusedAfterAxis;
    contact = wheel->ground;
    if (contact == 0) {
        return;
    }
    if (wheel->groundNormalComponents[2] < 0.7) {
        return;
    }
    Mtx3x3MulVec3f(&axle.vector, body->orientation, &axis.vector);
    forward[0] = axle.components[1] * wheel->groundNormalComponents[2] -
                 axle.components[2] * wheel->groundNormalComponents[1];
    forward[1] = axle.components[2] * wheel->groundNormalComponents[0] -
                 axle.components[0] * wheel->groundNormalComponents[2];
    forward[2] = axle.components[0] * wheel->groundNormalComponents[1] -
                 axle.components[1] * wheel->groundNormalComponents[0];
    side[0] = wheel->groundNormalComponents[1] * forward[2] -
              wheel->groundNormalComponents[2] * forward[1];
    side[1] = wheel->groundNormalComponents[2] * forward[0] -
              wheel->groundNormalComponents[0] * forward[2];
    side[2] = wheel->groundNormalComponents[0] * forward[1] -
              wheel->groundNormalComponents[1] * forward[0];
    cosine = cosf(wheel->steerAngle);
    sine = sinf(wheel->steerAngle);
    roll[0] = forward[0] * cosine;
    roll[1] = forward[1] * cosine;
    roll[2] = forward[2] * cosine;
    slide[0] = forward[0] * -sine;
    slide[1] = forward[1] * -sine;
    slide[2] = forward[2] * -sine;
    forward[0] = side[0] * sine;
    forward[1] = side[1] * sine;
    forward[2] = side[2] * sine;
    roll[0] = roll[0] + forward[0];
    roll[1] = roll[1] + forward[1];
    roll[2] = roll[2] + forward[2];
    forward[0] = side[0] * cosine;
    forward[1] = side[1] * cosine;
    forward[2] = side[2] * cosine;
    slide[0] = slide[0] + forward[0];
    slide[1] = slide[1] + forward[1];
    slide[2] = slide[2] + forward[2];
    if (body->wheels[0]->contactFrames != 0 && body->wheels[2]->contactFrames != 0 &&
        body->wheels[1]->contactFrames != 0 && body->wheels[3]->contactFrames != 0) {
        GetWheelWorldVelocity(&velocity.vector, body, wheel);
        rollSpeed = velocity.components[0] * roll[0] + velocity.components[1] * roll[1] +
                    velocity.components[2] * roll[2];
        height = wheel->state.position.z - -0.97;
        height *= 0;
        axle.components[1] = 0.0f;
        axle.components[0] = 0.0f;
        axle.components[2] = (body->mass + 4.0f * wheel->mass) * 2.9430003f + height;
        normalLoad = (axle.components[0] * wheel->groundNormalComponents[0] +
                      axle.components[1] * wheel->groundNormalComponents[1] +
                      axle.components[2] * wheel->groundNormalComponents[2]) *
                     3.5f;
        drive = wheel->drive;
        force = drive / wheel->radius;
        *loadOut = *loadOut + force / 2;
        if (*sliding != 0) {
            force = force * 0.9;
        }
        if (WHEEL_ABS(force) > WHEEL_ABS(normalLoad)) {
            normalLoad = WHEEL_ABS(normalLoad / force);
            force = force * (normalLoad * 0.1);
        }
        push.components[0] = -force * roll[0];
        push.components[1] = -force * roll[1];
        push.components[2] = -force * roll[2];
        Mtx4fInverseRotate(&axle.vector, body->orientation, &push.vector);
        wheel->forces->vector.x = wheel->forces->vector.x + axle.components[0];
        wheel->forces->vector.y = wheel->forces->vector.y + axle.components[1];
        wheel->forces->vector.z = wheel->forces->vector.z + axle.components[2];
        height = drive - wheel->radius * force;
        wheel->spinRate = wheel->spinRate + height * dt;
        wheel->spinRate = wheel->spinRate - (wheel->spinRate * wheel->radius + rollSpeed) * 0.4;
        if (WHEEL_ABS(wheel->spinRate) > 300.0f) {
            wheel->spinRate = WHEEL_SIGN(wheel->spinRate) * 300.0;
        }
    } else {
        drive = wheel->drive;
        wheel->spinRate = wheel->spinRate + drive * dt;
        if (WHEEL_ABS(wheel->spinRate) > 300.0f) {
            wheel->spinRate = WHEEL_SIGN(wheel->spinRate) * 300.0;
        }
    }
    wheel->rollAngle = wheel->rollAngle - wheel->spinRate * 57.295776f * dt;
    while (wheel->rollAngle > 360.0) {
        wheel->rollAngle = wheel->rollAngle - 360.0;
    }
    if (wheel->rollAngle < 0.) {
        do {
            wheel->rollAngle = wheel->rollAngle + 360.0f;
        } while (wheel->rollAngle < 0.);
    }
}
