#include <PR/gu.h>

#include "common/camera.h"
#include "common/frametime.h"
#include "common/physics.h"
#include "common/timing.h"
#include "common/vehicleaccess.h"
#include "common/vehiclephysics.h"
#include "common/vehiclereset.h"

#if TGR_BOSS_RALLY
#include <math.h>
#include <string.h>

#include "common/screen.h"
#include "common/settings.h"
#else
#include <PR/os_libc.h>
#endif

#if TGR_BOSS_RALLY

#define FRAME_PRODUCT(a, b) ((long double)(a) * (b))
#define FRAME_LESS(a, b) (!((a) >= (b)))
#else
#define FRAME_LESS(a, b) ((a) < (b))
#endif /* TGR_BOSS_RALLY */

/* The rates of a flying vehicle. */
#define FLY_SPEED 100.0f
#define FLY_TURN_RATE 3.141592741f
#define FLY_TURN_SCALE 0.4f

/* The reverse toggle and the command to level a flying vehicle. */
#define INPUT_REVERSE_TOGGLE (1 << 17)
#define INPUT_FLY_LEVEL 0x10

/* The share of the drive the rear wheels take with rear-wheel drive, and the front ones. */
#if TGR_PLATFORM == TGR_PLATFORM_N64
#define REAR_DRIVE_SHARE 2
#define FRONT_DRIVE_SHARE 0
#else
#define REAR_DRIVE_SHARE 2.0f
#define FRONT_DRIVE_SHARE 0.0f
#endif

/* The drive of a front wheel below which the brakes act on the front wheels as well. */
#define FRONT_BRAKE_DRIVE_LIMIT 1.0e-4f

/* The scale from the speed of a vehicle to the speed the display shows. */
#define SPEED_TO_DISPLAY 2.24f

#if TGR_BOSS_RALLY
inline s32 Vehicle::CastGroundRay() {
    return CastRayAgainstTrack(&frame.position,
                               &frame.up,
                               &frame.position,
                               groundInstances,
                               &groundInstanceCount,
                               nearbyInstances,
                               &nearbyInstanceCount,
                               &groundDistance,
                               &groundSurface);
}

#define CAST_GROUND_RAY(vehicle) ((vehicle)->CastGroundRay())
#else
#define CAST_GROUND_RAY(vehicle)                                                                   \
    CastRayAgainstTrack(&(vehicle)->frame.position,                                                \
                        &(vehicle)->frame.up,                                                      \
                        &(vehicle)->frame.position,                                                \
                        (vehicle)->groundInstances,                                                \
                        &(vehicle)->groundInstanceCount,                                           \
                        (vehicle)->nearbyInstances,                                                \
                        &(vehicle)->nearbyInstanceCount,                                           \
                        &(vehicle)->groundDistance,                                                \
                        &(vehicle)->groundSurface)
#endif /* TGR_BOSS_RALLY */

void UpdateVehiclePhysicsFrame(Vehicle *vehicle) {
#if TGR_PLATFORM == TGR_PLATFORM_N64
#define wheels (VEHICLE_GET(vehicle, Body).wheels)
#else
    RigidBody **wheels = VEHICLE_GET(vehicle, Body).wheels;
#endif
#if TGR_PLATFORM == TGR_PLATFORM_N64
    /* Preserve the original stack layout for IDO. */
    s32 unusedBefore[4];
#endif
    Vec3f step;
    Vec3f *velocity;
    f32 drive TGR_MATCHING_UNUSED;
    s32 i;
#if TGR_PLATFORM == TGR_PLATFORM_N64
    Vec3f *side;
    Vec3f *up;
#endif

#if TGR_PLATFORM == TGR_PLATFORM_N64
    (void)unusedBefore;
#endif

    if (VEHICLE_GET(vehicle, FlyMode)) {
        /* Fly along the vehicle and turn with the controls. */
#if TGR_BOSS_RALLY
        Vec3fScale(&step,
                   &VEHICLE_GET(vehicle, Frame).forward,
                   FRAME_PRODUCT(VEHICLE_GET(vehicle, Accelerator), g_fFrameDeltaTime) * FLY_SPEED);
#else
        Vec3fScale(&step,
                   &VEHICLE_GET(vehicle, Frame).forward,
                   vehicle->accelerator * g_fFrameDeltaTime * FLY_SPEED);
#endif
        Vec3fAddInPlace(&VEHICLE_GET(vehicle, Frame).position, &step);
        SetVehiclePosition(vehicle,
                           VEHICLE_GET(vehicle, Frame).position.x,
                           VEHICLE_GET(vehicle, Frame).position.y,
                           VEHICLE_GET(vehicle, Frame).position.z);
        SetVehicleVelocity(vehicle, 0.0f, 0.0f, 0.0f);
        SetVehicleTurnRate(vehicle, 0.0f, 0.0f, 0.0f);
        VEHICLE_SET(vehicle, EngineRpm, 0.0f);
#if TGR_BOSS_RALLY
        RotateVehicleByEuler(vehicle,
                             FRAME_PRODUCT(VEHICLE_GET(vehicle, Steering), g_fFrameDeltaTime) *
                                 1.256637096405029296875f,
                             FRAME_PRODUCT(VEHICLE_GET(vehicle, PitchInput), g_fFrameDeltaTime) *
                                 1.256637096405029296875f,
                             FRAME_PRODUCT(VEHICLE_GET(vehicle, RollInput), g_fFrameDeltaTime) *
                                 1.256637096405029296875f);
#else
        RotateVehicleByEuler(
            vehicle,
            vehicle->steering * FLY_TURN_RATE * FLY_TURN_SCALE * g_fFrameDeltaTime,
            vehicle->pitchInput * FLY_TURN_RATE * FLY_TURN_SCALE * g_fFrameDeltaTime,
            vehicle->rollInput * FLY_TURN_RATE * FLY_TURN_SCALE * g_fFrameDeltaTime);
#endif /* TGR_BOSS_RALLY */
        if (VEHICLE_GET(vehicle, Input)->buttons & INPUT_FLY_LEVEL) {
#if TGR_PLATFORM == TGR_PLATFORM_N64
            up = &VEHICLE_GET(vehicle, Frame).up;
            side = &VEHICLE_GET(vehicle, Frame).side;
            up->x = 0.0f;
            up->y = 0.0f;
            up->z = 1.0f;
            Vec3fCross(side, up, &VEHICLE_GET(vehicle, Frame).forward);
            Vec3fCross(up, &VEHICLE_GET(vehicle, Frame).forward, side);
#else
            VEHICLE_GET(vehicle, Frame).up.x = 0.0f;
            VEHICLE_GET(vehicle, Frame).up.y = 0.0f;
            VEHICLE_GET(vehicle, Frame).up.z = 1.0f;
            Vec3fCross(&VEHICLE_GET(vehicle, Frame).side,
                       &VEHICLE_GET(vehicle, Frame).up,
                       &VEHICLE_GET(vehicle, Frame).forward);
            Vec3fCross(&VEHICLE_GET(vehicle, Frame).up,
                       &VEHICLE_GET(vehicle, Frame).forward,
                       &VEHICLE_GET(vehicle, Frame).side);
#endif
            TransferFlags(VEHICLE_GET(vehicle, Input), INPUT_FLY_LEVEL);
        }
    } else {
        /* Put the vehicle on the ground, then drive and brake its wheels and step its physics. */
        VEHICLE_SET(vehicle, GroundHits, CAST_GROUND_RAY(vehicle));
#if TGR_PLATFORM == TGR_PLATFORM_N64
        wheels[0]->steerAngle = wheels[1]->steerAngle = 0.0f;
#else
        wheels[1]->steerAngle = 0.0f;
        wheels[0]->steerAngle = wheels[1]->steerAngle;
#endif
        UpdateVehicleEngineAndGearbox(vehicle);
#if TGR_PLATFORM == TGR_PLATFORM_N64
        wheels[2]->steerAngle = wheels[3]->steerAngle = VEHICLE_GET(vehicle, SteerAngle);
        wheels[0]->drive = 0.0f;
        wheels[1]->drive = 0.0f;
        wheels[2]->drive = 0.0f;
        wheels[3]->drive = 0.0f;
        wheels[0]->brake = 0.0f;
        wheels[1]->brake = 0.0f;
        wheels[2]->brake = 0.0f;
        wheels[3]->brake = 0.0f;
#else
        wheels[3]->steerAngle = VEHICLE_GET(vehicle, SteerAngle);
        wheels[2]->steerAngle = VEHICLE_GET(vehicle, SteerAngle);
        for (i = 0; i < VEHICLE_WHEELS; i++) {
            wheels[i]->drive = 0.0f;
        }
        for (i = 0; i < VEHICLE_WHEELS; i++) {
            wheels[i]->brake = 0.0f;
        }
#endif
        if (0.0f < VEHICLE_GET(vehicle, Throttle)) {
#if TGR_BOSS_RALLY
            if (VEHICLE_GET(vehicle, Input)->buttons & INPUT_REVERSE_TOGGLE) {
#else
            if ((vehicle->input->buttons & INPUT_REVERSE_TOGGLE) && vehicle->gear == 1) {
#endif
                VEHICLE_SET(vehicle, Throttle, -VEHICLE_GET(vehicle, Throttle));
            }
#if TGR_BOSS_RALLY
            switch (VEHICLE_GET(vehicle, Model)->driveMode) {
            case 0:
                wheels[0]->drive = VEHICLE_GET(vehicle, Throttle) * 0.1f;
                wheels[1]->drive = VEHICLE_GET(vehicle, Throttle) * 0.1f;
                wheels[2]->drive = VEHICLE_GET(vehicle, Throttle) * -2.1f;
                wheels[3]->drive = VEHICLE_GET(vehicle, Throttle) * -2.1f;
                break;
            case 1:
                wheels[0]->drive = VEHICLE_GET(vehicle, Throttle) * -2.0f;
                wheels[1]->drive = VEHICLE_GET(vehicle, Throttle) * -2.0f;
                wheels[2]->drive = 0.0f;
                wheels[3]->drive = 0.0f;
                break;
            default:
                for (i = 0; i < VEHICLE_WHEELS; i++) {
                    wheels[i]->drive = -VEHICLE_GET(vehicle, Throttle);
                }
                break;
            }
#else
            if (vehicle->rearWheelDrive) {
                wheels[0]->drive = -vehicle->throttle * REAR_DRIVE_SHARE;
                wheels[1]->drive = -vehicle->throttle * REAR_DRIVE_SHARE;
                wheels[2]->drive = -vehicle->throttle * FRONT_DRIVE_SHARE;
                wheels[3]->drive = -vehicle->throttle * FRONT_DRIVE_SHARE;
            } else {
#if TGR_PLATFORM == TGR_PLATFORM_N64
                wheels[0]->drive = -vehicle->throttle;
                wheels[1]->drive = -vehicle->throttle;
                wheels[2]->drive = -vehicle->throttle;
                wheels[3]->drive = -vehicle->throttle;
#else
                for (i = 0; i < VEHICLE_WHEELS; i++) {
                    wheels[i]->drive = -vehicle->throttle;
                }
#endif
            }
#endif /* TGR_BOSS_RALLY */
        }
        if (FRAME_LESS(VEHICLE_GET(vehicle, Brake), 0.0f)) {
            wheels[0]->brake = -VEHICLE_GET(vehicle, Brake);
            wheels[1]->brake = -VEHICLE_GET(vehicle, Brake);
            wheels[0]->drive = 0.0f;
            wheels[1]->drive = 0.0f;
#if TGR_PLATFORM == TGR_PLATFORM_N64
            if ((wheels[2]->drive < 0.0f ? -wheels[2]->drive : wheels[2]->drive) <
                FRONT_BRAKE_DRIVE_LIMIT) {
#else
            drive = FRAME_LESS(wheels[2]->drive, 0.0f) ? -wheels[2]->drive : wheels[2]->drive;
            if (FRAME_LESS(drive, FRONT_BRAKE_DRIVE_LIMIT)) {
#endif
                wheels[2]->brake = -VEHICLE_GET(vehicle, Brake);
                wheels[3]->brake = -VEHICLE_GET(vehicle, Brake);
            }
        }
#if TGR_BOSS_RALLY
        if (ShouldStepVehiclePhysics(vehicle)) {
            SimulateVehiclePhysicsSubstep(vehicle);
        }
#else
        SimulateVehiclePhysicsSubstep(vehicle);
#endif
    }

#if TGR_BOSS_RALLY
    FinishVehiclePhysicsFrame(vehicle);
#else
    TransformAllWheelMatrices(vehicle);
#endif
    /* The speed changes while the first front wheel is on the ground. */
    if (VEHICLE_GET(vehicle, WheelBodies)[1].contactFrames) {
        velocity = &VEHICLE_GET(vehicle, Body).state.velocity;
#if TGR_BOSS_RALLY
        VEHICLE_SET(vehicle,
                    SpeedKph,
                    sqrtl((f32)(FRAME_PRODUCT(velocity->z, velocity->z) +
                                (FRAME_PRODUCT(velocity->x, velocity->x) +
                                 FRAME_PRODUCT(velocity->y, velocity->y)))) *
                        SPEED_TO_DISPLAY);
#else
        vehicle->speedKph = sqrtf(velocity->z * velocity->z +
                                  (velocity->x * velocity->x + velocity->y * velocity->y)) *
                            SPEED_TO_DISPLAY;
#endif /* TGR_BOSS_RALLY */
    }
#if TGR_BOSS_RALLY
    if (g_dwNetworkPlayMode) {
        return;
    }
#endif
    for (i = 0; i < g_dwNumViewports; i++) {
        if (VEHICLE_GET(vehicle, Index) == g_aViewportRects[i].vehicleIndex) {
            UpdateCameraSystem(vehicle);
            return;
        }
    }
}
#if TGR_PLATFORM == TGR_PLATFORM_N64
#undef wheels
#endif
