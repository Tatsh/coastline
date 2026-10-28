#include "physics.h"
#include "vehiclemodel.h"
#include "weather.h"

#if TGR_PC
#include <math.h>
#else
#include <PR/gu.h>
#endif

/* The speed below which a sliding tyre grips more and more, as a vehicle slows to a stop. */
#define SLOW_GRIP_SPEED 27.0f

/* The load above which the tyres of an axle slide, and while they slid. */
#define SLIDE_LOAD 8000.0f
#define SLIDING_REAR_SLIDE_LOAD 5600.0f

/* How fast the body of a vehicle leans into a skid, by step. */
#define LEAN_STEP 0.2666666806f

/* The weather of the grip of the dry surfaces, which fog and night share. */
#define GRIP_WEATHER_DRY 0

#if TGR_PC
#define GRIP_EXT long double
#define GRIP_PRODUCT(a, b) ((long double)(a) * (b))
#define GRIP_LESS(a, b) (!((a) >= (b)))
#define GRIP_MAX_LOAD(weather, surface) g_pSteeringFrictionTableB[weather][surface]
#define GRIP_SIGN(x) (!((x) < 0.0f) && !((x) > 0.0f) ? 0.0L : ((x) > 0.0f ? 1.0L : -1.0L))
#define ABS(x) (!((x) >= 0.0f) ? -(x) : (x))
#else
#define GRIP_EXT f32
#define GRIP_PRODUCT(a, b) ((a) * (b))
#define GRIP_LESS(a, b) ((a) < (b))
#define GRIP_MAX_LOAD(weather, surface) g_afSurfaceMaxLoad[weather][surface]
#define GRIP_SIGN(x) SIGN(x)
#define ABS(x) ((x) < 0.0f ? -(x) : (x))
#endif /* TGR_PC */

#define SIGN(x) ((x) == 0.0f ? 0.0 : (0.0f < (x) ? 1.0 : -1.0))

/* The slowing a brake gives a wheel against its spin. */
static inline GRIP_EXT BrakeSlowing(RigidBody *wheel) {
#if TGR_PC
    return GRIP_SIGN(wheel->spinRate) * ABS(wheel->brake) * -2.0f / wheel->radius;
#else
    return (f32)(SIGN(wheel->spinRate) * (f64)-ABS(wheel->brake) * 2.0) / wheel->radius;
#endif
}

/* The retained share of a slide from surface grip and tyre load. */
static inline f32 SlideShare(RigidBody *body, f32 grip, s32 weather, s32 surface, GRIP_EXT load) {
    GRIP_EXT clamped;
#if TGR_PC
    long double highKeep;
#endif
    f32 keep;

    clamped = load;
    if (GRIP_MAX_LOAD(weather, surface) < load) {
        clamped = GRIP_MAX_LOAD(weather, surface);
    }
    if (GRIP_LESS(clamped, g_afSurfaceSlipLoad[weather][surface])) {
        clamped = g_afSurfaceSlipLoad[weather][surface];
    }
#if TGR_PC
    highKeep = g_afSurfaceSlipLoad[weather][surface] / clamped * grip * 20.0f;
    keep = highKeep;
    if (!(body->wheels[2]->steerAngle < 0.0f) && !(body->wheels[2]->steerAngle > 0.0f)) {
        highKeep *= 1.5;
        keep = highKeep;
    }
    if (ABS(highKeep) > 1.0f) {
        keep = 1.0f;
    }
#else
    keep = g_afSurfaceSlipLoad[weather][surface] / clamped * 20.0f * grip;
    if (body->wheels[2]->steerAngle == 0.0f) {
        keep = (f32)((f64)keep * 1.5);
    }
    if (ABS(keep) > 1.0f) {
        keep = 1.0f;
    }
#endif /* TGR_PC */
    return keep;
}

#if TGR_PC
#define BODY_SPEED(body)                                                                           \
    sqrtl((f32)(GRIP_PRODUCT((body)->state.velocity.z, (body)->state.velocity.z) +                 \
                (GRIP_PRODUCT((body)->state.velocity.x, (body)->state.velocity.x) +                \
                 GRIP_PRODUCT((body)->state.velocity.y, (body)->state.velocity.y))))
#else
/* The speed of the body of a vehicle. */
#define BODY_SPEED(body)                                                                           \
    sqrtf((body)->state.velocity.z *(body)->state.velocity.z +                                     \
          ((body)->state.velocity.x * (body)->state.velocity.x +                                   \
           (body)->state.velocity.y * (body)->state.velocity.y))

#endif /* TGR_PC */

/* The step the side grip works with: the step given in the USA release, and a fixed thirtieth of a
 * second in the European and Japanese releases. */
#if TGR_PC || TGR_REGION == TGR_REGION_US
#define GRIP_STEP(dt) (dt)
#else
#define GRIP_STEP(dt) 0.033333335f
#endif

void ApplySideGrip(
    RigidBody *body, f32 dt, f32 *rearLoad, f32 *frontLoad, u8 *rearSliding, u8 *frontSliding) {
    Vec3f point, velocity, rear, front, old, lateral, local;
#if TGR_PLATFORM == TGR_PLATFORM_N64
    Vec3f unitX;
#endif
    f32 skid = 0.0f;
    s32 both = 0;
    f32 frontBrake, rearBrake, quarter, limit, grip, keep;
    f32 forward, side, yaw, lean, slowed, d;
    GRIP_EXT load, lateralSpeed, speed, slowGrip, target;
#if TGR_PC
    long double highFrontBrake, highRearBrake, highLateral, highDot, highYaw;
    f32 square;
#else
    f32 dt2;
#endif
    s32 i, rearSurface, frontSurface;
    s16 weather;
    u8 rearSurfaceA, rearSurfaceB, frontSurfaceA, frontSurfaceB;

    for (i = 0; i < VEHICLE_WHEELS; i++) {
        if (!body->wheels[i]->ground) {
            body->wheels[i]->contactFrames = 0;
        }
    }

    /* The brakes slow the axles along the body, by step, no more than the speed of a step. */
#if TGR_PC
    highFrontBrake = BrakeSlowing(body->wheels[2]);
    highRearBrake = BrakeSlowing(body->wheels[0]);
    quarter = body->mass * 0.25f;
    frontBrake = (highFrontBrake / quarter * dt) * dt;
    rearBrake = (highRearBrake / quarter * dt) * dt;
#else
    frontBrake = BrakeSlowing(body->wheels[2]);
    rearBrake = BrakeSlowing(body->wheels[0]);
    quarter = body->mass / 4.0f;
    dt2 = GRIP_STEP(dt) * GRIP_STEP(dt);
    /* The original stores the front quotient before scaling it. */
    frontBrake = frontBrake / quarter;
    rearBrake = rearBrake / quarter;
    frontBrake = frontBrake * dt2;
    rearBrake = rearBrake * dt2;
#endif /* TGR_PC */
    if (ABS(frontBrake) > 1.0f) {
        frontBrake = (f32)(GRIP_SIGN(frontBrake) * 1.5);
    }
    if (ABS(rearBrake) > 1.0f) {
        rearBrake = (f32)(GRIP_SIGN(rearBrake) * 1.5);
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
        load = ABS(*rearLoad) + GRIP_PRODUCT(ABS(rear.y), body->mass) / GRIP_STEP(dt);
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
#if TGR_PC
        grip = g_pSteeringFrictionTableA[body->tireType][weather][rearSurface];
#else
        grip =
            (f32)((f64)g_afSurfaceGrip[weather][rearSurface] - 0.002 * (f64)(body->tireType - 1));
#endif
        keep = SlideShare(body, grip, weather, rearSurface, load);
        if (GRIP_LESS(load, limit)) {
            skid = 0.0f;
            lateralSpeed = 0.0f;
        } else {
            speed = BODY_SPEED(body);
            if (GRIP_LESS(speed, SLOW_GRIP_SPEED)) {
#if TGR_PC
                slowGrip = (SLOW_GRIP_SPEED - speed) * 0.0037037038718760013580322265625f;
#else
                slowGrip = (SLOW_GRIP_SPEED - speed) * 0.1f / SLOW_GRIP_SPEED;
#endif
                if (GRIP_LESS(keep, slowGrip)) {
                    keep = slowGrip;
                }
            }
#if TGR_PC
            highLateral = (long double)rear.y - GRIP_PRODUCT(rear.y, keep);
            rear.y = highLateral;
            skid = highLateral * keep;
#else
            rear.y = rear.y - rear.y * keep;
            skid = rear.y * keep;
#endif
            *rearSliding = 1;
#if TGR_PC
            lateralSpeed = highLateral;
#else
            lateralSpeed = rear.y;
#endif
        }
#if TGR_PLATFORM == TGR_PLATFORM_N64
        /* The original copies the unit vector along x and never reads the copy. */
        unitX = g_vecUnitX;
        (void)unitX;
#endif
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
        if (ABS(rear.x) > 1e-05f && GRIP_SIGN(rear.x) != GRIP_SIGN(slowed)) {
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
#if TGR_PC
        highDot = GRIP_PRODUCT(point.x, old.x) + GRIP_PRODUCT(point.y, old.y);
        d = highDot;
        front.x = point.x * highDot;
        front.y = GRIP_PRODUCT(point.y, d);
        front.z = 0.0f;
        lateral.x = old.x - front.x;
        lateral.y = old.y - front.y;
        lateral.z = old.z - front.z;
        square = GRIP_PRODUCT(lateral.z, lateral.z) +
                 (GRIP_PRODUCT(lateral.x, lateral.x) + GRIP_PRODUCT(lateral.y, lateral.y));
        load = (f32)(ABS(*frontLoad) + sqrtl(square) * body->mass / dt);
#else
        /* Retain velocity along the front wheels and apply sliding grip to the remainder. */
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
#endif /* TGR_PC */
        limit = SLIDE_LOAD;
#if TGR_PC
        load = (f32)(load + 10000.0f * (f32)((f64)ABS(frontBrake) > 0.0001));
#else
        load += 10000.0f * (f32)((f64)ABS(frontBrake) > 0.0001);
#endif
        if (*frontSliding) {
            limit = (f32)((f64)SLIDE_LOAD * 0.7);
        }
        *frontSliding = 0;
        if (limit < load) {
            frontSurface = (frontSurfaceA + frontSurfaceB + 1) >> 1;
#if TGR_PC
            grip = g_pSteeringFrictionTableA[body->tireType][weather][frontSurface];
#else
            grip = g_afSurfaceGrip[weather][frontSurface] - 0.002f * (f32)(body->tireType - 1);
#endif
            keep = SlideShare(body, grip, weather, frontSurface, load);
            speed = BODY_SPEED(body);
            if (GRIP_LESS(speed, SLOW_GRIP_SPEED)) {
#if TGR_PC
                slowGrip = (SLOW_GRIP_SPEED - speed) * 0.0037037038718760013580322265625f;
#else
                slowGrip = (SLOW_GRIP_SPEED - speed) * 0.1f / SLOW_GRIP_SPEED;
#endif
                if (GRIP_LESS(keep, slowGrip)) {
                    keep = slowGrip;
                }
            }
#if TGR_PC
            front.x = old.x - GRIP_PRODUCT(lateral.x, keep);
            front.y = old.y - GRIP_PRODUCT(lateral.y, keep);
            front.z = old.z - GRIP_PRODUCT(lateral.z, keep);
#else
            lateral.x = lateral.x * keep;
            lateral.y = lateral.y * keep;
            lateral.z = lateral.z * keep;
            front.x = old.x - lateral.x;
            front.y = old.y - lateral.y;
            front.z = old.z - lateral.z;
#endif /* TGR_PC */
            *frontSliding = 1;
        }
#if TGR_PC
        else {
            /* The libraries clear the flag a second time. */
            *frontSliding = 0;
        }
#endif
    }

    /* Give the body the velocity and the turn that the axles now move by. */
    if (both) {
#if TGR_PC
        forward = ((long double)rear.x + front.x) * 0.5f;
        highYaw =
            ((long double)rear.y - front.y) /
            ((long double)body->wheels[0]->state.position.x - body->wheels[2]->state.position.x);
        yaw = highYaw;
        side = rear.y - highYaw * body->wheels[0]->state.position.x;
#else
        forward = (rear.x + front.x) / 2.0f;
        yaw = (rear.y - front.y) /
              (body->wheels[0]->state.position.x - body->wheels[2]->state.position.x);
        side = rear.y - yaw * body->wheels[0]->state.position.x;
#endif /* TGR_PC */
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
        skid = (f32)(GRIP_SIGN(skid) * 0.5);
    }
    target = GRIP_PRODUCT(skid, 2.0f) * -4.0f;
    lean = body->rollAngle;
    if (GRIP_LESS(ABS(lean - target), LEAN_STEP)) {
        body->rollAngle = target;
    } else if (lean < target) {
        body->rollAngle = lean + LEAN_STEP;
    } else {
        body->rollAngle = lean - LEAN_STEP;
    }
}
