#include "physics.h"

#include <PR/gu.h>
#include <PR/os.h>
#include <PR/os_libc.h>

#include "game.h"
#include "physicsimpact.h"
#include "timing.h"
#include "track.h"
#include "trianglebox.h"
#include "vehicle.h"
#include "weather.h"

/* The speed below which a sliding tyre grips more and more, as a vehicle slows to a stop. */
#define SLOW_GRIP_SPEED 27.0f

/* The load above which the tyres of an axle slide, and while they slid. */
#define SLIDE_LOAD 8000.0f
#define SLIDING_REAR_SLIDE_LOAD 5600.0f

/* How fast the body of a vehicle leans into a skid, by step. */
#define LEAN_STEP 0.2666666806f

/* The weather of the grip of the dry surfaces, which fog and night share. */
#define GRIP_WEATHER_DRY 0

#define ABS(x) ((x) < 0.0f ? -(x) : (x))

#define SIGN(x) ((x) == 0.0f ? 0.0 : (0.0f < (x) ? 1.0 : -1.0))

/* A coordinate of a vector by its axis, read as an element of an array of three. */
#define COMPONENT(v, axis) ((&(v)->x)[axis])

/* The slowing a brake gives a wheel against its spin. */
static inline f32 BrakeSlowing(RigidBody *wheel) {
    return (f32)(SIGN(wheel->spinRate) * (f64)-ABS(wheel->brake) * 2.0) / wheel->radius;
}

/* How much of a slide a tyre keeps, from the grip of its surface and the load on it. */
static inline f32 SlideShare(RigidBody *body, f32 grip, s32 weather, s32 surface, f32 load) {
    f32 clamped, keep;

    clamped = load;
    if (g_afSurfaceMaxLoad[weather][surface] < load) {
        clamped = g_afSurfaceMaxLoad[weather][surface];
    }
    if (clamped < g_afSurfaceSlipLoad[weather][surface]) {
        clamped = g_afSurfaceSlipLoad[weather][surface];
    }
    keep = g_afSurfaceSlipLoad[weather][surface] / clamped * 20.0f * grip;
    if (body->wheels[2]->steerAngle == 0.0f) {
        keep = (f32)((f64)keep * 1.5);
    }
    if (ABS(keep) > 1.0f) {
        keep = 1.0f;
    }
    return keep;
}

/* The speed of the body of a vehicle. */
#define BODY_SPEED(body)                                                                           \
    sqrtf((body)->state.velocity.z *(body)->state.velocity.z +                                     \
          ((body)->state.velocity.x * (body)->state.velocity.x +                                   \
           (body)->state.velocity.y * (body)->state.velocity.y))

/* The step the side grip works with: the step given in the USA release, and a fixed thirtieth of a
 * second in the European and Japanese releases. */
#if TGR_REGION == TGR_REGION_US
#define GRIP_STEP(dt) (dt)
#else
#define GRIP_STEP(dt) 0.033333335f
#endif

void ApplySideGrip(
    RigidBody *body, f32 dt, f32 *rearLoad, f32 *frontLoad, u8 *rearSliding, u8 *frontSliding) {
    Vec3f point, velocity, rear, front, old, lateral, local, unitX;
    f32 skid = 0.0f;
    s32 both = 0;
    f32 frontBrake, rearBrake, quarter, dt2, load, limit, grip, keep, lateralSpeed, speed;
    f32 slowGrip, forward, side, yaw, target, lean, slowed, d;
    s32 i, rearSurface, frontSurface;
    s16 weather;
    u8 rearSurfaceA, rearSurfaceB, frontSurfaceA, frontSurfaceB;

    for (i = 0; i < VEHICLE_WHEELS; i++) {
        if (!body->wheels[i]->ground) {
            body->wheels[i]->contactFrames = 0;
        }
    }

    /* The brakes slow the axles along the body, by step, no more than the speed of a step. */
    frontBrake = BrakeSlowing(body->wheels[2]);
    rearBrake = BrakeSlowing(body->wheels[0]);
    quarter = body->mass / 4.0f;
    dt2 = GRIP_STEP(dt) * GRIP_STEP(dt);
    /* The original stores the front quotient before scaling it. */
    frontBrake = frontBrake / quarter;
    rearBrake = rearBrake / quarter;
    frontBrake = frontBrake * dt2;
    rearBrake = rearBrake * dt2;
    if (ABS(frontBrake) > 1.0f) {
        frontBrake = (f32)(SIGN(frontBrake) * 1.5);
    }
    if (ABS(rearBrake) > 1.0f) {
        rearBrake = (f32)(SIGN(rearBrake) * 1.5);
    }

    /* The velocity of the rear axle in the frame of the body. */
    point.x = body->wheels[0]->state.position.x;
    point.y = 0.0f;
    point.z = 0.0f;
    GetBodyPointVelocity(&velocity, body, &point);
    Mtx4fInverseRotate(&rear, body->orientation, &velocity);

    weather = g_dwCurrentWeather - 1;
    rearSurfaceA = body->wheels[0]->surface;
    rearSurfaceB = body->wheels[1]->surface;
    frontSurfaceA = body->wheels[2]->surface;
    frontSurfaceB = body->wheels[3]->surface;
    if (weather >= GRIP_WEATHERS || weather < 0) {
        weather = GRIP_WEATHER_DRY;
    }
    body->skidding = 0;

    if ((body->wheels[0]->contactFrames || body->wheels[1]->contactFrames) &&
        (body->wheels[2]->contactFrames || body->wheels[3]->contactFrames)) {
        rearSurface = (rearSurfaceA + rearSurfaceB + 1) >> 1;
        load = ABS(*rearLoad) + ABS(rear.y) * body->mass / GRIP_STEP(dt);
        both = 1;
        limit = SLIDE_LOAD;
        /* A braking wheel loads its tyres the more when the other axle does not brake. */
        if ((f64)ABS(frontBrake) > 0.0001) {
            load += 10000.0f * (f32)((f64)ABS(rearBrake) > 0.0001);
        } else {
            load += 100000.0f * (f32)((f64)ABS(rearBrake) > 0.0001);
        }
        if (*rearSliding) {
            limit = SLIDING_REAR_SLIDE_LOAD;
        }
        *rearSliding = 0;
        grip =
            (f32)((f64)g_afSurfaceGrip[weather][rearSurface] - 0.002 * (f64)(body->tireType - 1));
        keep = SlideShare(body, grip, weather, rearSurface, load);
        if (load < limit) {
            skid = 0.0f;
            lateralSpeed = 0.0f;
        } else {
            speed = BODY_SPEED(body);
            if (speed < SLOW_GRIP_SPEED) {
                slowGrip = (SLOW_GRIP_SPEED - speed) * 0.1f / SLOW_GRIP_SPEED;
                if (keep < slowGrip) {
                    keep = slowGrip;
                }
            }
            rear.y = rear.y - rear.y * keep;
            skid = rear.y * keep;
            *rearSliding = 1;
            lateralSpeed = rear.y;
        }
        /* The original copies the unit vector along x and never reads the copy. */
        unitX = g_vecUnitX;
        (void)unitX;
        if (ABS(rear.x) > 1.0f) {
            rear.y = lateralSpeed;
            if (ABS(lateralSpeed / rear.x) > 0.25f) {
                body->skidding = 0x80;
            }
        } else {
            rear.y = lateralSpeed;
            if (ABS(lateralSpeed) > 1.0f) {
                body->skidding = 0x80;
            } else {
                body->skidding = 0;
            }
        }
        /* The brakes slow the rear, but do not turn it back. */
        slowed = rear.x;
        rear.x = rear.x - rearBrake;
        if (ABS(rear.x) > 1e-05f && SIGN(rear.x) != SIGN(slowed)) {
            rear.x = 0.0f;
        }
    } else {
        *rearSliding = 0;
    }

    /* The velocity of the front axle in the frame of the body. */
    point.x = body->wheels[2]->state.position.x;
    GetBodyPointVelocity(&velocity, body, &point);
    Mtx4fInverseRotate(&front, body->orientation, &velocity);

    if ((body->wheels[2]->contactFrames || body->wheels[3]->contactFrames) &&
        (body->wheels[0]->contactFrames || body->wheels[1]->contactFrames)) {
        both = 1;
        point.x = cosf(body->wheels[2]->steerAngle);
        point.y = sinf(body->wheels[2]->steerAngle);
        point.z = 0.0f;
        old.x = front.x;
        old.y = front.y;
        old.z = front.z;
        /* Keep the velocity along the way the front wheels point; the rest slides. */
        d = old.z * point.z + (point.x * old.x + point.y * old.y);
        front.x = point.x * d;
        front.y = point.y * d;
        front.z = point.z * d;
        lateral.x = old.x - front.x;
        lateral.y = old.y - front.y;
        lateral.z = old.z - front.z;
        lateralSpeed =
            sqrtf(lateral.z * lateral.z + (lateral.x * lateral.x + lateral.y * lateral.y));
        load = ABS(*frontLoad) + lateralSpeed * body->mass / GRIP_STEP(dt);
        limit = SLIDE_LOAD;
        load += 10000.0f * (f32)((f64)ABS(frontBrake) > 0.0001);
        if (*frontSliding) {
            limit = (f32)((f64)SLIDE_LOAD * 0.7);
        }
        *frontSliding = 0;
        if (limit < load) {
            frontSurface = (frontSurfaceA + frontSurfaceB + 1) >> 1;
            grip = g_afSurfaceGrip[weather][frontSurface] - 0.002f * (f32)(body->tireType - 1);
            keep = SlideShare(body, grip, weather, frontSurface, load);
            speed = BODY_SPEED(body);
            if (speed < SLOW_GRIP_SPEED) {
                slowGrip = (SLOW_GRIP_SPEED - speed) * 0.1f / SLOW_GRIP_SPEED;
                if (keep < slowGrip) {
                    keep = slowGrip;
                }
            }
            lateral.x = lateral.x * keep;
            lateral.y = lateral.y * keep;
            lateral.z = lateral.z * keep;
            front.x = old.x - lateral.x;
            front.y = old.y - lateral.y;
            front.z = old.z - lateral.z;
            *frontSliding = 1;
        }
    }

    /* Give the body the velocity and the turn that the axles now move by. */
    if (both) {
        forward = (rear.x + front.x) / 2.0f;
        yaw = (rear.y - front.y) /
              (body->wheels[0]->state.position.x - body->wheels[2]->state.position.x);
        side = rear.y - yaw * body->wheels[0]->state.position.x;
        Mtx4fInverseRotate(&local, body->orientation, &body->state.angularVelocity);
        local.z = yaw;
        Mtx3x3MulVec3f(&body->state.angularVelocity, body->orientation, &local);
        Mtx4fInverseRotate(&local, body->orientation, &body->state.velocity);
        local.x = forward;
        local.y = side;
        Mtx3x3MulVec3f(&body->state.velocity, body->orientation, &local);
    }

    /* Lean the body towards the skid, a step at a time. */
    if (ABS(skid) > 0.5f) {
        skid = (f32)(SIGN(skid) * 0.5);
    }
    target = skid / 0.5f * -4.0f;
    lean = body->rollAngle;
    if (ABS(lean - target) < LEAN_STEP) {
        body->rollAngle = target;
    } else if (lean < target) {
        body->rollAngle = lean + LEAN_STEP;
    } else {
        body->rollAngle = lean - LEAN_STEP;
    }
}

s32 UnusedCollisionResponseStub(void *body, void *normal, void *damp) {
    return 1;
}

void PrintCollisionTriangleCount(void) {
    CollisionTriangleNode *node;
    s16 count;

    node = g_pCollisionTriangleList;
    if (node != NULL) {
        count = 0;
        for (; node != NULL; node = node->next) {
            count++;
        }
        osSyncPrintf("----%d-------------------------------\n", count);
    }
}

s32 UnusedTrackCollisionStub(void *body) {
    return 0;
}

/* The cell of the grid under a point, as its row times 64 plus its column. */
#define GRID_SECTOR(x, y) ((s32)(y) / 32 * 64 + (s32)(x) / 32)

s32 InvalidateTriCacheSectorAt(f32 x, f32 y) {
    s16 sector = GRID_SECTOR(x, y);
    s32 slot;

    for (slot = 0; slot < TRI_CACHE_SLOTS; slot++) {
        if (sector == g_awTriCacheSectorId[slot]) {
            g_adwTriCacheLruStamp[slot] = 0;
            g_awTriCacheSectorId[slot] = 0;
            return 1;
        }
    }
    return 0;
}
