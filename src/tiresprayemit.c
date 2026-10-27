#include "effects.h"
#include "frametime.h"
#include "trackselect.h"
#include "weather.h"

#define SPRAY_UNITS 127.0f
#define SPRAY_FRAMES 3.0f
#define SCATTER_DISTANCE_SQUARED 256.0f
#define RANDOM_SHARE (1.0f / 65535.0f)

#if TGR_PLATFORM == TGR_PLATFORM_WIN
typedef ConvertedModelVertex SprayVertex;
typedef long double SprayReal;
#define SPRAY_NEGATIVE(value) (!((value) >= 0.0L))
#define SPRAY_NONZERO(value) ((value) < 0.0f || (value) > 0.0f)
#define SPRAY_SURFACE(value) ((s8)(value))
#define SPRAY_SCATTER(distance) (!((distance) >= SCATTER_DISTANCE_SQUARED))
#define SPRAY_INTEGER(value) ((s64)(value))
#define SPRAY_COORDINATE(value) ((s16)SPRAY_INTEGER(value))
#define SPRAY_VERTEX(head, vertex, coordinate, field) ((head)[vertex].position.field)
#else
typedef Vtx SprayVertex;
typedef f32 SprayReal;
#define SPRAY_NEGATIVE(value) ((value) < 0.0f)
#define SPRAY_NONZERO(value) ((value) != 0.0f)
#define SPRAY_SURFACE(value) (value)
#define SPRAY_SCATTER(distance) ((distance) < SCATTER_DISTANCE_SQUARED)
#define SPRAY_INTEGER(value) ((s32)(value))
#define SPRAY_COORDINATE(value) SPRAY_INTEGER(value)
#define SPRAY_VERTEX(head, vertex, coordinate, field) ((head)[vertex].v.ob[coordinate])
#endif

#if TGR_PLATFORM == TGR_PLATFORM_WIN
void Vehicle::EmitTireSpray(void) {
    Vehicle *vehicle = this;
#else
void EmitTireSpray(Vehicle *vehicle) {
#endif
    RigidBody *bodies[VEHICLE_WHEELS];
    SprayVertex *head;
    s16(*velocities)[3];
    Vec3f velocity, position, thrown;
    f32 speedShare;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    /* Preserve the x87 sign change before multiplying a NaN. */
    volatile SprayReal forwardSpeed, negativeForwardSpeed;
#else
    SprayReal forwardSpeed;
#endif
    f32 scale, slide;
    SprayReal random;
    s32 sheltered, holding, throwing, surface, wheel;

    sheltered = FALSE;
    if (vehicle->groundInstanceCount &&
        (g_LoadedTrack.instances[vehicle->groundInstances[0]].flags & TRACK_INSTANCE_SHELTER)) {
        sheltered = TRUE;
    }
    if (g_dwSnowMode && g_dwSelectedTrackId != TRACK_COASTLINE &&
        g_dwSelectedTrackId != TRACK_MIRROR_COASTLINE) {
        return;
    }
    bodies[0] = &vehicle->wheelBodies[3];
    bodies[1] = &vehicle->wheelBodies[1];
    bodies[2] = &vehicle->wheelBodies[0];
    bodies[3] = &vehicle->wheelBodies[2];
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    speedShare = (1.0L - 50.0f / ((long double)vehicle->speedKph - -50.0f)) * 3.0f;
#else
    speedShare = (1.0f - 50.0f / (vehicle->speedKph + 50.0f)) * 3.0f;
#endif
    forwardSpeed = Vec3fDot(&vehicle->frame.forward, &vehicle->placedVelocity);
    if (SPRAY_NEGATIVE(forwardSpeed)) {
        forwardSpeed = -forwardSpeed;
    }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    negativeForwardSpeed = forwardSpeed * -2.24f;
    slide = (1.0L - 25.0f / (25.0L - negativeForwardSpeed)) * 3.0f;
#endif

    for (wheel = 0; wheel < VEHICLE_WHEELS; wheel++) {
        /* A new segment begins four times a second; the spray is stationary between segments. */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        vehicle->sprayPhases[wheel] =
            (long double)vehicle->sprayPhases[wheel] - (long double)g_fFrameDeltaTime * -4.0f;
#else
        vehicle->sprayPhases[wheel] += g_fFrameDeltaTime * 4.0f;
#endif
        if (vehicle->sprayPhases[wheel] > 0.75f) {
            holding = FALSE;
            if (vehicle->sprayPhases[wheel] >= 1.7f) {
                vehicle->sprayPhases[wheel] -= 1.0f;
            } else {
                vehicle->sprayPhases[wheel] = 0.0f;
            }
            vehicle->sprayAdvanced[wheel] = TRUE;
        } else {
            holding = TRUE;
            vehicle->sprayAdvanced[wheel] = FALSE;
        }

        if (bodies[wheel]->contactFrames != 0) {
            vehicle->sprayFramesLeft[wheel] = SPRAY_FRAMES;
            vehicle->spraySurfaces[wheel] = SPRAY_SURFACE(bodies[wheel]->surface);
        } else if (SPRAY_NONZERO(vehicle->sprayFramesLeft[wheel])) {
            vehicle->sprayFramesLeft[wheel] -= 1.0f;
        }
        surface = vehicle->spraySurfaces[wheel];

        throwing = FALSE;
        if (SPRAY_NONZERO(vehicle->sprayFramesLeft[wheel])) {
            if (holding) {
                scale = 1.5f;
                throwing = TRUE;
            } else if (surface == TRACK_SURFACE_SLOW) {
                scale = 2.5f;
                throwing = TRUE;
            } else if (surface == TRACK_SURFACE_PAVED && g_dwRainMode && !sheltered) {
                scale = 1.5f;
                throwing = TRUE;
                /* Water begins a segment every frame for the next seven. */
                if (!holding) {
                    vehicle->sprayPhases[wheel] = 7.75f;
                }
            }
        }

        if (throwing) {
            if (!holding) {
                Vec3fScale(&velocity, &vehicle->frame.up, scale);
                /* The original tests the surface again for the rest. */
                if (surface != TRACK_SURFACE_PAVED) {
                    Vec3fMulAddInPlace(&velocity,
                                       &vehicle->frame.side,
                                       wheel == 0 ? scale :
                                       wheel < 3  ? -scale :
                                                    scale);
                }
                if (surface != TRACK_SURFACE_PAVED) {
                    Vec3fScaleInPlace(&velocity, speedShare);
                    Vec3fMulAddInPlace(&velocity, &vehicle->placedVelocity, 0.3f);
                    Vec3fMulAddInPlace(&velocity,
                                       &vehicle->frame.side,
                                       Vec3fDot(&vehicle->frame.side, &vehicle->placedVelocity) *
                                           0.5f);
                } else {
#if TGR_PLATFORM == TGR_PLATFORM_N64
                    slide = (1.0f - 25.0f / (2.24f * forwardSpeed + 25.0f)) * 3.0f;
#endif
                    Vec3fScaleInPlace(&velocity, slide);
#if TGR_PLATFORM == TGR_PLATFORM_WIN
                    if (SPRAY_NONZERO(slide)) {
#else
                    if (slide != 0.0f) {
#endif
                        if (wheel >= 2) {
                            velocity.z += velocity.z;
                        }
                        Vec3fMulAddInPlace(
                            &velocity,
                            &vehicle->frame.side,
                            Vec3fDot(&vehicle->frame.side, &vehicle->placedVelocity) * 0.3f);
                    }
                }
            }
            Vec3fMulAdd(
                &position, &vehicle->wheelFrames[wheel].position, &vehicle->frame.up, -0.25f);
            if (surface != TRACK_SURFACE_PAVED || wheel < 2) {
                Vec3fMulAddInPlace(
                    &position, &vehicle->frame.side, wheel != 0 && wheel < 3 ? -0.15f : 0.15f);
            }
            thrown = position;
            if (!holding &&
                SPRAY_SCATTER(Vec3fDistanceSquared(&position, &vehicle->sprayPositions[wheel]))) {
                random = (SprayReal)(AdvanceRng() & 0xFFFF) * RANDOM_SHARE;
                Vec3fLerp(
                    &position, &vehicle->sprayPositions[wheel], &position, (f32)(random * random));
            }
            vehicle->sprayPositions[wheel] = thrown;
        } else {
            Vec3fClear(&velocity);
            position = vehicle->wheelFrames[wheel].position;
        }

        if (!holding) {
            /* Store the top velocity and the lower edge velocity along the ground. */
            velocities = vehicle->sprayVelocities[wheel][0];
            velocities[1][0] = SPRAY_INTEGER((SprayReal)velocity.x * SPRAY_UNITS);
            velocities[1][1] = SPRAY_INTEGER((SprayReal)velocity.y * SPRAY_UNITS);
            velocities[1][2] = SPRAY_INTEGER((SprayReal)velocity.z * SPRAY_UNITS);
            if (surface == TRACK_SURFACE_PAVED && g_dwRainMode) {
                velocities[0][2] = 0;
                velocities[0][0] = velocities[1][0];
                velocities[0][1] = velocities[1][1];
            } else {
                velocities[0][2] = 0;
                velocities[0][0] = velocities[1][0] >> 1;
                velocities[0][1] = velocities[1][1] >> 1;
            }
        }

        vehicle->spraySegmentSurfaces[wheel][0] = surface;
        head = vehicle->sprayVertices[wheel][0];
        SPRAY_VERTEX(head, 1, 0, x) =
            SPRAY_COORDINATE(((SprayReal)position.x - vehicle->sprayOrigin.x) * SPRAY_UNITS);
        SPRAY_VERTEX(head, 1, 1, y) =
            SPRAY_COORDINATE(((SprayReal)position.y - vehicle->sprayOrigin.y) * SPRAY_UNITS);
        SPRAY_VERTEX(head, 1, 2, z) =
            SPRAY_COORDINATE(((SprayReal)position.z - vehicle->sprayOrigin.z) * SPRAY_UNITS);
        SPRAY_VERTEX(head, 0, 0, x) = SPRAY_VERTEX(head, 1, 0, x);
        SPRAY_VERTEX(head, 0, 1, y) = SPRAY_VERTEX(head, 1, 1, y);
        SPRAY_VERTEX(head, 0, 2, z) = SPRAY_VERTEX(head, 1, 2, z);
    }
}
