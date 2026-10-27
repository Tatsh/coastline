#include "physics.h"

#include <PR/gu.h>
#include <PR/os.h>
#include <PR/os_libc.h>

#include "game.h"
#include "timing.h"
#include "track.h"
#include "vehicle.h"
#include "weather.h"

/* The weight on a wheel for each unit of mass of the vehicle: gravity times 0.3. */
#define WHEEL_WEIGHT_SHARE (9.81f * 0.3f)

/* The most a wheel spins, in radians a second. */
#define MAX_SPIN_RATE 300.0f

/* The speed below which a sliding tyre grips more and more, as a vehicle slows to a stop. */
#define SLOW_GRIP_SPEED 27.0f

/* The load above which the tyres of an axle slide, and while they slid. */
#define SLIDE_LOAD 8000.0f
#define SLIDING_REAR_SLIDE_LOAD 5600.0f

/* How fast the body of a vehicle leans into a skid, by step. */
#define LEAN_STEP 0.2666666806f

/* The speed a hit is counted up to for its sound. */
#define MAX_IMPACT_SPEED 27.0f

/* The weather of the grip of the dry surfaces, which fog and night share. */
#define GRIP_WEATHER_DRY 0

/* The steps of the collisions in a frame. */
#define COLLISION_STEPS 4

/* The frames a vehicle counts down from while it is upside down. */
#define UPRIGHT_FRAMES 35

/* The height of the body above which a collision does not hold it. */
#define UPRIGHT_LIMIT 0.5f

#define ABS(x) ((x) < 0.0f ? -(x) : (x))

#define SIGN(x) ((x) == 0.0f ? 0.0 : (0.0f < (x) ? 1.0 : -1.0))

/* A coordinate of a vector by its axis, read as an element of an array of three. */
#define COMPONENT(v, axis) ((&(v)->x)[axis])

/* The loudness of a hit at a speed, which the sound of a crash plays at. */
static inline u8 ImpactVolume(f32 speed) {
    return (u32)(speed * 127.0f / MAX_IMPACT_SPEED + 128.0f);
}

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

void DriveWheel(RigidBody *body, RigidBody *wheel, f32 *load, u8 *sliding, f32 dt) {
    Vec3f unitY, axle, forward, side, roll, slide, velocity, weight, push, local;
    Vec3f *normal = &wheel->groundNormal;
    f32 c, s, rollSpeed, pressure, drive, force, grip, ratio;

    unitY = g_vecUnitY;
    if (!wheel->ground) {
        return;
    }
    if ((f64)normal->z < 0.7) {
        return;
    }
    /* The ways the wheel rolls and slides on the ground. */
    Mtx3x3MulVec3f(&axle, body->orientation, &unitY);
    forward.x = axle.y * normal->z - normal->y * axle.z;
    forward.y = axle.z * normal->x - normal->z * axle.x;
    forward.z = axle.x * normal->y - normal->x * axle.y;
    side.x = normal->y * forward.z - forward.y * normal->z;
    side.y = normal->z * forward.x - forward.z * normal->x;
    side.z = normal->x * forward.y - forward.x * normal->y;
    c = cosf(wheel->steerAngle);
    s = sinf(wheel->steerAngle);
    roll.x = forward.x * c;
    roll.y = forward.y * c;
    roll.z = forward.z * c;
    /* The original also turns the slide direction with the wheel and never reads it. */
    slide.x = forward.x * -s;
    slide.y = forward.y * -s;
    slide.z = forward.z * -s;
    forward.x = side.x * s;
    forward.y = side.y * s;
    forward.z = side.z * s;
    roll.x = forward.x + roll.x;
    roll.y = forward.y + roll.y;
    roll.z = forward.z + roll.z;
    forward.x = side.x * c;
    forward.y = side.y * c;
    forward.z = side.z * c;
    slide.x = forward.x + slide.x;
    slide.y = forward.y + slide.y;
    slide.z = forward.z + slide.z;
    (void)slide;

    if (body->wheels[0]->contactFrames && body->wheels[2]->contactFrames &&
        body->wheels[1]->contactFrames && body->wheels[3]->contactFrames) {
        GetWheelWorldVelocity(&velocity, body, wheel);
        rollSpeed = roll.z * velocity.z + (velocity.x * roll.x + velocity.y * roll.y);
        /* The weight on the wheel, which bounds how hard its drive can push. */
        weight.x = 0.0f;
        weight.y = 0.0f;
        weight.z = (body->mass + 4.0f * wheel->mass) * WHEEL_WEIGHT_SHARE +
                   (f32)((f64)wheel->state.position.z - -0.97) * 0.0f;
        pressure = normal->z * weight.z + (weight.x * normal->x + weight.y * normal->y);
        drive = wheel->drive;
        force = drive / wheel->radius;
        grip = pressure * 3.0f;
        *load = *load + force / 2.0f;
        if (*sliding) {
            force = (f32)((f64)force * 0.9);
        }
        if (ABS(grip) < ABS(force)) {
            ratio = grip / force;
            force = (f32)((f64)force * ((f64)ABS(ratio) * 0.1));
        }
        push.x = roll.x * -force;
        push.y = roll.y * -force;
        push.z = roll.z * -force;
        Mtx4fInverseRotate(&local, body->orientation, &push);
        wheel->forces->vector.x = local.x + wheel->forces->vector.x;
        wheel->forces->vector.y = local.y + wheel->forces->vector.y;
        wheel->forces->vector.z = local.z + wheel->forces->vector.z;
        /* The drive spins the wheel against its push, and the ground turns it to the speed. */
        wheel->spinRate = wheel->spinRate + (drive - wheel->radius * force) * dt;
        wheel->spinRate =
            (f32)((f64)wheel->spinRate - (f64)(wheel->spinRate * wheel->radius + rollSpeed) * 0.4);
        if (ABS(wheel->spinRate) > MAX_SPIN_RATE) {
            wheel->spinRate = (f32)(SIGN(wheel->spinRate) * 300.0);
        }
    } else {
        wheel->spinRate = wheel->spinRate + wheel->drive * dt;
        if (ABS(wheel->spinRate) > MAX_SPIN_RATE) {
            wheel->spinRate = (f32)(SIGN(wheel->spinRate) * 300.0);
        }
    }

    /* Turn the wheel by its spin, within a turn. */
    wheel->rollAngle = wheel->rollAngle - wheel->spinRate * 57.29577637f * dt;
    while ((f64)wheel->rollAngle > 360.0) {
        wheel->rollAngle = (f32)((f64)wheel->rollAngle - 360.0);
    }
    while ((f64)wheel->rollAngle < 0.0) {
        wheel->rollAngle = wheel->rollAngle + 360.0f;
    }
}

s16 IsPointInTrackTriangle(TrackTriangle *triangle, Vec3f *point) {
    f32 u0, v0, pu, pv, au, av, bu, bv, alpha, beta;
    s32 axis, i1, i2;
    s32 inside = 0;

    /* Drop the main axis of the normal, and solve for the triangle there. */
    if (ABS(triangle->normal.y) < ABS(triangle->normal.x)) {
        axis = ABS(triangle->normal.z) < ABS(triangle->normal.x) ? 0 : 2;
    } else {
        axis = ABS(triangle->normal.z) < ABS(triangle->normal.y) ? 1 : 2;
    }
    i1 = (axis + 1) % 3;
    i2 = (axis + 2) % 3;
    u0 = COMPONENT(triangle->vertices[0], i1);
    v0 = COMPONENT(triangle->vertices[0], i2);
    pu = COMPONENT(point, i1) - u0;
    pv = COMPONENT(point, i2) - v0;
    au = COMPONENT(triangle->vertices[1], i1) - u0;
    av = COMPONENT(triangle->vertices[1], i2) - v0;
    bu = COMPONENT(triangle->vertices[2], i1) - u0;
    bv = COMPONENT(triangle->vertices[2], i2) - v0;
    if (au == 0.0f) {
        beta = pu / bu;
        if (0.0f <= beta && beta <= 1.0f) {
            alpha = (pv - beta * bv) / av;
            inside = 0.0f <= alpha && alpha + beta <= 1.0f;
        }
    } else {
        beta = (pv * au - pu * av) / (bv * au - bu * av);
        if (0.0f <= beta && beta <= 1.0f) {
            alpha = (pu - beta * bu) / au;
            inside = 0.0f <= alpha && alpha + beta <= 1.0f;
        }
    }
    return inside;
}

/* Record a hard hit at a speed into a surface for its sound, and where it was. */
static inline void RecordImpact(RigidBody *body, f32 speed) {
    body->impactSpeed = (u32)speed;
    if (body->impactSpeed < 10) {
        body->impactSpeed = 0;
    } else {
        body->impactPoint.x = g_vCollisionContact.x;
        body->impactPoint.y = g_vCollisionContact.y;
        body->impactPoint.z = g_vCollisionContact.z;
    }
}

/* Keep the loudest hit for the sound. */
static inline void RecordImpactVolume(RigidBody *body, f32 speed) {
    u8 volume = ImpactVolume(speed);

    /* The original writes the volume back even when it does not change. */
    body->impactVolume = body->impactVolume < volume ? volume : body->impactVolume;
}

s32 BounceOffPlane(RigidBody *body, Vec3f *normal, s32 damp, s32 spin) {
    Vec3f *velocity = &body->stepStates[1].velocity;
    Vec3f *angular = &body->stepStates[1].angularVelocity;
    Vec3f impulse, pivot, scale, turn;
    f32 basis[4][4];
    f32 into, damping, speed;
    f64 bounce;

    into = velocity->z * normal->z + (normal->x * velocity->x + normal->y * velocity->y);
    if (0.0f <= into) {
        return 0;
    }
    bounce = (f64)into * 1.05;
    impulse.x = (f32)((f64)normal->x * bounce);
    impulse.y = (f32)((f64)normal->y * bounce);
    impulse.z = (f32)((f64)normal->z * bounce);
    velocity->x = velocity->x - impulse.x;
    velocity->y = velocity->y - impulse.y;
    velocity->z = velocity->z - impulse.z;
    if (body->framesSinceImpact < 10) {
        damping = 0.9f;
    } else {
        speed = ABS(into);
        if (MAX_IMPACT_SPEED < speed) {
            speed = MAX_IMPACT_SPEED;
        }
        RecordImpact(body, speed);
        RecordImpactVolume(body, speed);
        damping = 0.9f;
        velocity->x = velocity->x * damping;
        velocity->y = velocity->y * damping;
        velocity->z = velocity->z * damping;
    }
    if (damp) {
        velocity->x = velocity->x * damping;
        velocity->y = velocity->y * damping;
        velocity->z = velocity->z * damping;
    }
    if (!spin) {
        return 1;
    }

    /* Damp the spin by its parts in a frame of the normal and two axes across it. */
    pivot.x = normal->y;
    pivot.y = normal->z;
    pivot.z = normal->x;
    basis[1][0] = normal->y * pivot.z - pivot.y * normal->z;
    basis[1][1] = normal->z * pivot.x - pivot.z * normal->x;
    basis[1][2] = normal->x * pivot.y - pivot.x * normal->y;
    basis[2][0] = normal->y * basis[1][2] - basis[1][1] * normal->z;
    basis[2][1] = normal->z * basis[1][0] - basis[1][2] * normal->x;
    basis[2][2] = normal->x * basis[1][1] - basis[1][0] * normal->y;
    basis[0][0] = normal->x;
    basis[0][1] = normal->y;
    basis[0][2] = normal->z;
    Mtx3x3MulVec3f(&scale, basis, normal);
    Mtx3x3MulVec3f(&turn, basis, angular);
    turn.x = turn.x * scale.x;
    turn.y = turn.y * scale.y;
    turn.z = turn.z * scale.z;
    Mtx4fInverseRotate(angular, basis, &turn);
    return 1;
}

s32 UnusedCollisionResponseStub(void *body, void *normal, void *damp) {
    return 1;
}

s32 ApplyCollisionImpulse(
    RigidBody *body, Vec3f *point, Vec3f *normal, s32 friction, f32 restitution) {
    Vec3f *velocity = &body->stepStates[1].velocity;
    Vec3f *angular = &body->stepStates[1].angularVelocity;
    f32 transpose[3][3], rotation[3][3], skew[3][3], a[3][3], b[3][3], k[3][3], unit[3][3];
    Vec3f unitZ, arm, copy, motion, across, slide, target, impulse, linear, turn, spin;
    f32 into, speed, bounce;
    s32 i, j;

    /* The original copies the unit vector along z and never reads the copy. */
    unitZ = g_vecUnitZ;
    (void)unitZ;
    copy.x = point->x;
    copy.y = point->y;
    copy.z = point->z;
    TransposeCopyMatrix3x3(transpose, rotation, body->orientation);
    Mtx3fMulVec3f(&arm, transpose, &copy);
    /* How the point moves. */
    motion.x = angular->y * arm.z - arm.y * angular->z;
    motion.y = angular->z * arm.x - arm.z * angular->x;
    motion.z = angular->x * arm.y - arm.x * angular->y;
    motion.x = velocity->x + motion.x;
    motion.y = velocity->y + motion.y;
    motion.z = velocity->z + motion.z;
    into = motion.z * normal->z + (normal->x * motion.x + normal->y * motion.y);
    if (0.0f <= into) {
        return 0;
    }

    /* The mass the point moves with along each axis. */
    BuildSkewSymmetricMatrix(skew, &arm);
    Mtx3fMultiply(a, body->inverseInertia, rotation);
    Mtx3fMultiply(b, transpose, a);
    Mtx3fMultiply(k, b, skew);
    Mtx3fMultiply(a, skew, k);
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            unit[i][j] = i == j ? 1.0f : 0.0f;
        }
    }
    unit[2][2] = unit[1][1] = unit[0][0] = 1.0f / body->mass;
    SubtractMatrix3x3(k, unit, a);

    into = motion.z * normal->z + (normal->x * motion.x + normal->y * motion.y);
    across.x = normal->x * into;
    across.y = normal->y * into;
    across.z = normal->z * into;
    speed = ABS(into);
    if (MAX_IMPACT_SPEED < speed) {
        speed = MAX_IMPACT_SPEED;
    }
    RecordImpact(body, speed);
    if (body->framesSinceImpact >= 11 && restitution < 0.0001f) {
        RecordImpactVolume(body, speed);
        across.x = across.x * 0.9f;
        across.y = across.y * 0.9f;
        across.z = across.z * 0.9f;
        motion.x = motion.x * 0.9f;
        motion.y = motion.y * 0.9f;
        motion.z = motion.z * 0.9f;
    }
    /* Stop the point moving into the surface, and keep a part of its slide along it. */
    slide.x = motion.x - across.x;
    slide.y = motion.y - across.y;
    slide.z = motion.z - across.z;
    if (friction) {
        slide.x = slide.x * 0.2f;
        slide.y = slide.y * 0.2f;
        slide.z = slide.z * 0.2f;
    } else {
        slide.x = 0.0f;
        slide.y = 0.0f;
        slide.z = 0.0f;
    }
    target.x = across.x + slide.x;
    target.y = across.y + slide.y;
    target.z = across.z + slide.z;
    SolveLinearSystem3x3(&impulse, k, &target);
    linear.x = impulse.x * (1.0f / body->mass);
    linear.y = impulse.y * (1.0f / body->mass);
    linear.z = impulse.z * (1.0f / body->mass);
    turn.x = arm.y * impulse.z - impulse.y * arm.z;
    turn.y = arm.z * impulse.x - impulse.z * arm.x;
    turn.z = arm.x * impulse.y - impulse.x * arm.y;
    Mtx3fMulVec3f(&spin, b, &turn);
    bounce = 1.05f + restitution;
    linear.x = linear.x * bounce;
    linear.y = linear.y * bounce;
    linear.z = linear.z * bounce;
    spin.x = spin.x * bounce;
    spin.y = spin.y * bounce;
    spin.z = spin.z * bounce;
    velocity->x = velocity->x - linear.x;
    velocity->y = velocity->y - linear.y;
    velocity->z = velocity->z - linear.z;
    angular->x = angular->x - spin.x;
    angular->y = angular->y - spin.y;
    angular->z = angular->z - spin.z;
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

/* The faces of the unit cube a point is outside, two bits an axis. */
static inline s32 FaceOutcode(Vec3f *v) {
    s32 code = 0;

    if ((f64)v->x > 0.5) {
        code = 0x1;
    } else if ((f64)v->x < -0.5) {
        code = 0x2;
    }
    if ((f64)v->y > 0.5) {
        code |= 0x4;
    } else if ((f64)v->y < -0.5) {
        code |= 0x8;
    }
    if ((f64)v->z > 0.5) {
        code |= 0x10;
    } else if ((f64)v->z < -0.5) {
        code |= 0x20;
    }
    return code;
}

/* The edges of the unit cube a point is outside, by the planes through them. */
static inline s32 EdgeOutcode(Vec3f *v, s32 mask) {
    s32 code = 0;
    f32 sum;

    if (mask & 0x3) {
        sum = v->y + v->x;
        if ((mask & 0x1) && (f64)sum > 1.0) {
            code = 0x1;
        } else if ((mask & 0x2) && (f64)sum < -1.0) {
            code = 0x2;
        }
    }
    if (mask & 0xC) {
        sum = v->x - v->y;
        if ((mask & 0x4) && (f64)sum > 1.0) {
            code |= 0x4;
        } else if ((mask & 0x8) && (f64)sum < -1.0) {
            code |= 0x8;
        }
    }
    if (mask & 0x30) {
        sum = v->z + v->x;
        if ((mask & 0x10) && (f64)sum > 1.0) {
            code |= 0x10;
        } else if ((mask & 0x20) && (f64)sum < -1.0) {
            code |= 0x20;
        }
    }
    if (mask & 0xC0) {
        sum = v->x - v->z;
        if ((mask & 0x40) && (f64)sum > 1.0) {
            code |= 0x40;
        } else if ((mask & 0x80) && (f64)sum < -1.0) {
            code |= 0x80;
        }
    }
    if (mask & 0x300) {
        sum = v->z + v->y;
        if ((mask & 0x100) && (f64)sum > 1.0) {
            code |= 0x100;
        } else if ((mask & 0x200) && (f64)sum < -1.0) {
            code |= 0x200;
        }
    }
    if (mask & 0xC00) {
        sum = v->y - v->z;
        if ((mask & 0x400) && (f64)sum > 1.0) {
            code |= 0x400;
        } else if ((mask & 0x800) && (f64)sum < -1.0) {
            code |= 0x800;
        }
    }
    return code;
}

/* The corners of the unit cube a point is outside, by the planes through them. */
static inline s32 CornerOutcode(Vec3f *v, s32 mask) {
    s32 code = 0;
    f32 sum;

    if (mask & 0x3) {
        sum = v->z + (v->x + v->y);
        if ((mask & 0x1) && (f64)sum > 1.5) {
            code = 0x1;
        } else if ((mask & 0x2) && (f64)sum < -1.5) {
            code = 0x2;
        }
    }
    if (mask & 0xC) {
        sum = (v->x + v->y) - v->z;
        if ((mask & 0x4) && (f64)sum > 1.5) {
            code |= 0x4;
        } else if ((mask & 0x8) && (f64)sum < -1.5) {
            code |= 0x8;
        }
    }
    if (mask & 0x30) {
        sum = v->z + (v->x - v->y);
        if ((mask & 0x10) && (f64)sum > 1.5) {
            code |= 0x10;
        } else if ((mask & 0x20) && (f64)sum < -1.5) {
            code |= 0x20;
        }
    }
    if (mask & 0xC0) {
        sum = (v->x - v->y) - v->z;
        if ((mask & 0x40) && (f64)sum > 1.5) {
            code |= 0x40;
        } else if ((mask & 0x80) && (f64)sum < -1.5) {
            code |= 0x80;
        }
    }
    return code;
}

s32 ClassifyTriangleInUnitCube(Vec3f triangle[3]) {
    s32 shared = -1;
    s32 code, i;

    /* A corner inside, or every corner beyond the same face. */
    for (i = 0; i < 3; i++) {
        code = FaceOutcode(&triangle[i]);
        if (code == 0) {
            return 1;
        }
        shared &= code;
    }
    if (shared) {
        return 0;
    }
    /* Every corner beyond the same edge, then the same corner, of the cube. */
    code = -1;
    for (i = 0; i < 3; i++) {
        code = EdgeOutcode(&triangle[i], code);
        if (code == 0) {
            break;
        }
    }
    if (code) {
        return 0;
    }
    code = -1;
    for (i = 0; i < 3; i++) {
        code = CornerOutcode(&triangle[i], code);
        if (code == 0) {
            break;
        }
    }
    if (code) {
        return 0;
    }
    return -1;
}

s32 CountTriangleWinding(Vec3f triangle[3], Vec3f *normal, Vec3f *point) {
    f32 n[3];
    f32 p0, p1, a0, a1, b0, b1;
    s32 i, axis, i0, i1, edge, next, cross0, cross1;
    s32 winding = 0;

    for (i = 0; i < 3; i++) {
        n[i] = ABS(COMPONENT(normal, i));
    }
    if (n[2] < n[0]) {
        axis = n[1] < n[0] ? 0 : 1;
    } else {
        axis = (n[2] < n[1] ? 0 : 1) + 1;
    }
    if (COMPONENT(normal, axis) < 0.0f) {
        i0 = (axis + 2) % 3;
        i1 = (axis + 1) % 3;
    } else {
        i0 = (axis + 1) % 3;
        i1 = (axis + 2) % 3;
    }
    p0 = COMPONENT(point, i0);
    for (edge = 0; edge < 3; edge++) {
        next = (edge + 1) % 3;
        a0 = COMPONENT(&triangle[edge], i0);
        b0 = COMPONENT(&triangle[next], i0);
        cross0 = (p0 < b0) - (p0 < a0);
        if (cross0) {
            b1 = COMPONENT(&triangle[next], i1);
            p1 = COMPONENT(point, i1);
            a1 = COMPONENT(&triangle[edge], i1);
            cross1 = (p1 < b1) - (p1 < a1);
            if (cross1) {
                if ((f32)cross0 * (p0 - a0) * (b1 - a1) <= (b0 - a0) * ((f32)cross0 * (p1 - a1))) {
                    winding += cross0;
                }
            } else if (a1 <= p1) {
                winding += cross0;
            }
        }
    }
    return winding;
}

s32 DoesSegmentCrossUnitCube(Vec3f *start, Vec3f *end) {
    s32 i, j, k;
    s32 sign[3];
    f32 d[3];
    f32 across, reach;

    d[0] = end->x - start->x;
    d[1] = end->y - start->y;
    d[2] = end->z - start->z;
    for (i = 0; i < 3; i++) {
        sign[i] = d[i] < 0.0f ? -1 : 1;
    }
    /* Both ends beyond a face. */
    for (i = 0; i < 3; i++) {
        if ((f64)(COMPONENT(start, i) * (f32)sign[i]) > 0.5) {
            return 0;
        }
        if ((f64)(COMPONENT(end, i) * (f32)sign[i]) < -0.5) {
            return 0;
        }
    }
    /* The line passes beside the cube, seen along an axis. */
    for (i = 0; i < 3; i++) {
        j = (i + 1) % 3;
        k = (i + 2) % 3;
        across = d[k] * COMPONENT(start, j) - d[j] * COMPONENT(start, k);
        reach = (f32)((f64)(d[k] * (f32)sign[j] + d[j] * (f32)sign[k]) * 0.5);
        if (reach * reach < across * across) {
            return 0;
        }
    }
    return 1;
}

s32 DoesTriangleCrossUnitCube(Vec3f triangle[3], Vec3f *normal) {
    s32 sign[3];
    f32 sx, sy, sz, t;
    Vec3f point;
    s32 edge, i;

    for (edge = 0; edge < 3; edge++) {
        if (DoesSegmentCrossUnitCube(&triangle[edge], &triangle[(edge + 1) % 3])) {
            return 1;
        }
    }
    /* Where the diagonal of the cube nearest the normal meets the plane of the triangle. */
    for (i = 0; i < 3; i++) {
        sign[i] = COMPONENT(normal, i) < 0.0f ? -1 : 1;
    }
    sx = (f32)sign[0];
    sy = (f32)sign[1];
    sz = (f32)sign[2];
    t = (normal->x * triangle[0].x + normal->y * triangle[0].y + normal->z * triangle[0].z) /
        (sz * normal->z + (normal->x * sx + normal->y * sy));
    if (((f64)t - -0.5) * ((f64)t - 0.5) <= 0.0) {
        point.x = sx * t;
        point.y = sy * t;
        point.z = sz * t;
        return CountTriangleWinding(triangle, normal, &point);
    }
    return 0;
}

s32 IsTriangleInUnitCube(Vec3f triangle[3], Vec3f *normal) {
    s32 inside = ClassifyTriangleInUnitCube(triangle);

    if (inside == -1) {
        return DoesTriangleCrossUnitCube(triangle, normal);
    }
    return inside;
}

/* Move a triangle into the unit cube of a box, and find its normal there. */
static inline void TriangleInBox(Vec3f corners[3], Vec3f *normal, f32 box[4][4], TrackTriangle *t) {
    Vec3f e1, e2;

    Mtx4x4TransformPoint(&corners[0], box, t->vertices[0]);
    Mtx4x4TransformPoint(&corners[1], box, t->vertices[1]);
    Mtx4x4TransformPoint(&corners[2], box, t->vertices[2]);
    e1.x = corners[1].x - corners[0].x;
    e1.y = corners[1].y - corners[0].y;
    e1.z = corners[1].z - corners[0].z;
    e2.x = corners[2].x - corners[0].x;
    e2.y = corners[2].y - corners[0].y;
    e2.z = corners[2].z - corners[0].z;
    normal->x = e1.y * e2.z - e2.y * e1.z;
    normal->y = e1.z * e2.x - e2.z * e1.x;
    normal->z = e1.x * e2.y - e2.x * e1.y;
}

s32 CollectTrianglesInBox(RigidBody *body, f32 box[4][4]) {
    TrackTriangle *triangles;
    Vec3f corners[3], normal;
    s32 slot, count, i;
    s32 found = 0;

    slot = BuildTrackTriangleCache(body->orientation[3][0], body->orientation[3][1]);
    count = g_awTriCacheTriCount[slot];
    triangles = g_aTriCacheTriangles[slot];
    if (g_bTrackCollisionFrameToggle) {
        for (i = count - 1; i >= 0; i--) {
            TriangleInBox(corners, &normal, box, &triangles[i]);
            if (IsTriangleInUnitCube(corners, &normal)) {
                PushCollisionTriangle(&triangles[i]);
                found++;
            }
        }
    } else {
        for (i = 0; i < count; i++) {
            TriangleInBox(corners, &normal, box, &triangles[i]);
            if (IsTriangleInUnitCube(corners, &normal)) {
                PushCollisionTriangle(&triangles[i]);
                found++;
            }
        }
    }
    return found;
}

s32 UnusedTrackCollisionStub(void *body) {
    return 0;
}

/* The distance of a point from the ground of a wheel, either side of it. */
static inline f32 GroundDistance(RigidBody *wheel, Vec3f *point) {
    if (PlaneDistance(&wheel->groundNormal, wheel->groundOffset, point) < 0.0f) {
        return -PlaneDistance(&wheel->groundNormal, wheel->groundOffset, point);
    }
    return PlaneDistance(&wheel->groundNormal, wheel->groundOffset, point);
}

/* The original sets the wheel only in the loop, which a touching wheel always reaches. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#pragma GCC diagnostic ignored "-Wuninitialized"
s32 TipBalancedVehicle(RigidBody *body) {
    RigidBody *wheel;
    Vec3f corner, point, velocity, nudge;
    f32 nearest, distance, along, tilt;
    s32 touching, i;
    Vec3f *spin;

    QuaternionToMatrix(body->orientation, &body->stepStates[0]);
    nearest = 100.0f;
    touching = 0;
    /* The nearest of the corners of the box over the wheels on the ground to their ground. */
    for (i = 0; i < VEHICLE_WHEELS; i++) {
        if (!body->wheels[i]->contactFrames) {
            continue;
        }
        touching++;
        wheel = body->wheels[i];
        corner.x = (f32)(SIGN(wheel->state.position.x) * (f64)(body->boxSize.x * 0.5f));
        corner.y = (f32)(SIGN(wheel->state.position.y) * (f64)(body->boxSize.y * 0.5f));
        corner.z = -body->boxSize.z * 0.5f + body->boxCenterZ;
        Mtx4x4TransformPoint(&point, body->orientation, &corner);
        distance = GroundDistance(wheel, &point);
        if (!(nearest < distance)) {
            nearest = distance;
        }
    }
    if (touching >= 3 || touching <= 0) {
        return 0;
    }
    osSyncPrintf("-- Stand Dist = %f\n", (f64)nearest);
    if ((f64)nearest > 0.6) {
        return 0;
    }
    GetBodyPointVelocity(&velocity, body, &corner);
    along = wheel->groundNormal.z * velocity.z +
            (velocity.x * wheel->groundNormal.x + velocity.y * wheel->groundNormal.y);
    osSyncPrintf("-- Stand Vel = %f\n", (f64)along);
    if (ABS(along) > 0.1f) {
        return 0;
    }
    /* Roll it towards the way its nose leans across the ground. */
    tilt = body->groundNormal.z * body->orientation[0][2] +
           (body->orientation[0][0] * body->groundNormal.x +
            body->orientation[0][1] * body->groundNormal.y);
    if (0.0f < tilt) {
        nudge.x = 0.0f;
        nudge.y = 0.1f;
        nudge.z = 0.0f;
    } else {
        nudge.x = 0.0f;
        nudge.y = -0.1f;
        nudge.z = 0.0f;
    }
    osSyncPrintf("-- Stand Point = %f\n", (f64)tilt);
    Mtx3x3MulVec3f(&point, body->orientation, &nudge);
    spin = &body->stepStates[0].angularVelocity;
    for (i = 0; i < 2; i++) {
        spin->x = point.x + spin->x;
        spin->y = point.y + spin->y;
        spin->z = point.z + spin->z;
    }
    ComputeDeltaQuaternion(&body->stepStates[0]);
    return 1;
}
#pragma GCC diagnostic pop

void ComputeCollisionPenetration(
    RigidBody *body, Vec3f *normal, f32 offset, Vec3f *contact, Vec3f triangle[3]) {
    f32 middle[3];
    f32 distance;
    s32 i;

    if (g_dwCollisionContactType != 2) {
        distance =
            contact->z * normal->z + (normal->x * contact->x + normal->y * contact->y) - offset;
    } else {
        /* The face of the box by the middle of the triangle, signed by its x as the original. */
        for (i = 0; i < 3; i++) {
            middle[i] = COMPONENT(&triangle[2], i) +
                        (COMPONENT(&triangle[0], i) + COMPONENT(&triangle[1], i));
            /* The original stores the sum before dividing it. */
            middle[i] = middle[i] / 3.0f;
        }
        if (ABS(middle[0]) < ABS(middle[1])) {
            if (ABS(middle[0]) < ABS(middle[2])) {
                g_vCollisionContact.z = 0.0f;
                g_vCollisionContact.y = g_vCollisionContact.z;
                g_vCollisionContact.x = (f32)(middle[0] < 0.0f ? -1 : 1) * 0.5f;
            } else {
                g_vCollisionContact.y = 0.0f;
                g_vCollisionContact.x = g_vCollisionContact.y;
                g_vCollisionContact.z = (f32)(middle[0] < 0.0f ? -1 : 1) * 0.5f;
            }
        } else if (ABS(middle[1]) < ABS(middle[2])) {
            g_vCollisionContact.z = 0.0f;
            g_vCollisionContact.x = g_vCollisionContact.z;
            g_vCollisionContact.y = (f32)(middle[0] < 0.0f ? -1 : 1) * 0.5f;
        } else {
            g_vCollisionContact.y = 0.0f;
            g_vCollisionContact.x = g_vCollisionContact.y;
            g_vCollisionContact.z = (f32)(middle[0] < 0.0f ? -1 : 1) * 0.5f;
        }
        distance = g_vCollisionContact.z * normal->z +
                   (normal->x * g_vCollisionContact.x + normal->y * g_vCollisionContact.y) - offset;
        /* The original scales x by the length and the width, and the word after the contact. */
        g_vCollisionContact.x = g_vCollisionContact.x * body->boxSize.x;
        g_vCollisionContact.x = g_vCollisionContact.x * body->boxSize.y;
        g_fCollisionContactW = g_fCollisionContactW * body->boxSize.z;
    }
    g_vCollisionPenetration.x = normal->x * -distance;
    g_vCollisionPenetration.y = normal->y * -distance;
    g_vCollisionPenetration.z = normal->z * -distance;
}

/* The original passes the spin flag to the bounce without setting it outside the demonstration. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#pragma GCC diagnostic ignored "-Wuninitialized"
/* The races of the demonstration that sort their contacts with the track: every one in the USA
 * release, the ones under the credits in the Japanese release, and none in the European release. */
#if TGR_REGION == TGR_REGION_JP
#define SORTS_CONTACTS() (g_dwGameMode == GAME_MODE_DEMO && g_dwRaceSessionType)
#else
#define SORTS_CONTACTS() (g_dwGameMode == GAME_MODE_DEMO)
#endif

s32 CollideBodyWithTriangles(RigidBody *body, f32 box[4][4]) {
    CollisionTriangleNode *node;
    TrackTriangle *triangle;
    Vec3f corners[3], normal, along, half, back;
    f32 distance, push;
    s32 resist, spin, moved;
    s32 hit = 0;
    s16 hits = 0;

    for (node = g_pCollisionTriangleList; node != NULL; node = node->next) {
        triangle = node->triangle;
        TriangleInBox(corners, &normal, box, triangle);
        if (!IsTriangleInUnitCube(corners, &normal)) {
            continue;
        }
        Vec3fNormalizeFast(&normal);
        resist = 1;
        distance = corners[0].z * normal.z + (normal.x * corners[0].x + normal.y * corners[0].y);
        g_dwCollisionContactType = 0;
        hits++;
#if TGR_REGION != TGR_REGION_EU
        if (SORTS_CONTACTS()) {
            if (ABS(normal.x) > 0.999f || ABS(normal.y) > 0.999f || ABS(normal.z) > 0.999f) {
                g_dwCollisionContactType = 1;
                spin = 1;
                osSyncPrintf("Wank CT1 case\n");
            } else if (ABS(distance) < 0.5f) {
                osSyncPrintf("Triangle Edge to CubeFace\n");
                spin = 0;
                g_dwCollisionContactType = 2;
            }
        }
#endif
        osSyncPrintf("Cube Edge to Triangle Face\n");
        /* The corner of the box towards the triangle. */
        along.x = normal.x * distance;
        along.y = normal.y * distance;
        along.z = normal.z * distance;
        half.x = (f32)(along.x < 0.0f ? -1 : 1) * 0.5f;
        half.y = (f32)(along.y < 0.0f ? -1 : 1) * 0.5f;
        half.z = (f32)(along.z < 0.0f ? -1 : 1) * 0.5f;
        g_vCollisionContact.x = half.x * body->boxSize.x;
        g_vCollisionContact.y = half.y * body->boxSize.y;
        g_vCollisionContact.z = half.z * body->boxSize.z + body->boxCenterZ;
        g_pCollisionTriangle = triangle;
        ComputeCollisionPenetration(body, &normal, distance, &half, corners);
        if (UPRIGHT_LIMIT < body->orientation[2][2]) {
            resist = 0;
        }
        if (resist) {
            osSyncPrintf("Resistive collision %10.3f\n", (f64)body->orientation[2][2]);
        }
        if (g_dwCollisionContactType == 1) {
            moved = BounceOffPlane(body, &g_pCollisionTriangle->normal, resist, spin);
        } else {
            moved = ApplyCollisionImpulse(
                body, &g_vCollisionContact, &g_pCollisionTriangle->normal, resist, 0.0f);
        }
        if (moved) {
            hit = 1;
            /* Move the step back out along the normal by a tenth more than it moved in. */
            back.x = body->stepStates[1].position.x - body->stepStates[0].position.x;
            back.y = body->stepStates[1].position.y - body->stepStates[0].position.y;
            back.z = body->stepStates[1].position.z - body->stepStates[0].position.z;
            push = (f32)((f64)(triangle->normal.z * back.z +
                               (back.x * triangle->normal.x + back.y * triangle->normal.y)) *
                         1.1);
            back.x = triangle->normal.x * push;
            back.y = triangle->normal.y * push;
            back.z = triangle->normal.z * push;
            body->stepStates[1].position.x = body->stepStates[1].position.x - back.x;
            body->stepStates[1].position.y = body->stepStates[1].position.y - back.y;
            body->stepStates[1].position.z = body->stepStates[1].position.z - back.z;
            /* The original takes back three of the four parts of the orientation. */
            body->stepStates[1].orientation.w = body->stepStates[0].orientation.w;
            body->stepStates[1].orientation.y = body->stepStates[0].orientation.y;
            body->stepStates[1].orientation.x = body->stepStates[0].orientation.x;
            ComputeDeltaQuaternion(&body->stepStates[1]);
            QuaternionToMatrix(body->orientation, &body->stepStates[1]);
        }
    }
    if (hits) {
        body->framesSinceImpact = 0;
    } else if (body->framesSinceImpact < 40) {
        body->framesSinceImpact++;
    }
    return hit;
}
#pragma GCC diagnostic pop

void StepVehicleCollisions(RigidBody *body) {
    f32 box[4][4];
    Vec3f scale;
    /* The original reserves stack for twelve bytes of locals it does not use. */
    f32 unused[3];
    f32 remaining, step;
    /* The count of the steps is a variable, and the original divides by it at run time. */
    f32 steps;

    (void)unused;
    g_pCollisionTriangleList = NULL;
    g_pNextCollisionTriangleNode = g_aCollisionTriangleNodes;
    g_vCollisionContact.x = 0.0f;
    g_vCollisionContact.y = 0.0f;
    g_vCollisionContact.z = 0.0f;
    g_vCollisionPenetration.x = 0.0f;
    g_vCollisionPenetration.y = 0.0f;
    g_vCollisionPenetration.z = 0.0f;
    g_dwCollisionContactType = 0;
    g_pCollisionTriangle = NULL;
    AddTimingMarker(0, 0x80, 0x80, 0x80, 0xFF);
    /* The triangles near the body, in a box ten times its size. */
    scale.y = 0.1f;
    scale.x = 0.1f;
    scale.z = 0.1f;
    MtxBuildScaledInverse(body->orientation, box, &scale);
    CollectTrianglesInBox(body, box);
    AddTimingMarker(0, 0x80, 0x80, 0, 0xFF);
    scale.x = 1.0f / body->boxSize.x;
    scale.y = 1.0f / body->boxSize.y;
    scale.z = 1.0f / body->boxSize.z;
    steps = COLLISION_STEPS;
    remaining = 1.0f / 30.0f;
    if (0.002f < remaining) {
        step = 1.0f / 30.0f / steps;
        do {
            if (TipBalancedVehicle(body)) {
                osSyncPrintf("Standing on it's F'in Nose damnit\n");
            }
            CollideVehicles();
            IntegrateRigidBodyState(&body->stepStates[1], &body->stepStates[0], step);
            QuaternionToMatrix(body->orientation, &body->stepStates[1]);
            MtxBuildScaledInverse(body->orientation, box, &scale);
            box[3][2] = box[3][2] - body->boxCenterZ;
            if (CollideBodyWithTriangles(body, box)) {
                ComputeDeltaQuaternion(&body->stepStates[1]);
                QuaternionToMatrix(body->orientation, &body->stepStates[1]);
            }
            memcpy(&body->stepStates[0], &body->stepStates[1], sizeof(RigidBodyState));
            remaining = remaining - step;
        } while (0.002f < remaining);
    }
    QuaternionToMatrix(body->orientation, &body->stepStates[1]);
    AddTimingMarker(0, 0x80, 0x80, 0, 0xFF);
    memcpy(&body->stepStates[0], &body->stepStates[1], sizeof(RigidBodyState));
    if (body->orientation[2][2] < UPRIGHT_LIMIT) {
        if (body->uprightFrames < 0) {
            body->uprightFrames = -1;
        }
        body->uprightFrames--;
    } else {
        body->uprightFrames = UPRIGHT_FRAMES;
    }
}

f32 CastWheelRay(RigidBody *body, RigidBody *wheel) {
    TrackTriangle *triangle;
    Vec3f down, start, origin, direction, hit;
    f32 nearest, distance, facing;
    s32 slot, count, i;

    down = g_vecNegUnitZ;
    nearest = 100.0f;
    start.x = wheel->state.position.x;
    start.y = wheel->state.position.y;
    start.z = 0.0f;
    Mtx4x4TransformPoint(&origin, body->orientation, &start);
    Mtx3x3MulVec3f(&direction, body->orientation, &down);
    wheel->ground = NULL;
    slot = BuildTrackTriangleCache(origin.x, origin.y);
    count = g_awTriCacheTriCount[slot];
    triangle = g_aTriCacheTriangles[slot];
    for (i = 0; i < count; i++, triangle++) {
        distance = PlaneDistance(&triangle->normal, triangle->planeOffset, &origin);
        if (!((f64)distance > -2.0) || !((f64)distance < 2.0)) {
            continue;
        }
        facing = direction.z * triangle->normal.z +
                 (triangle->normal.x * direction.x + triangle->normal.y * direction.y);
        if (!((f64)ABS(facing) > 0.001)) {
            continue;
        }
        distance = -(triangle->planeOffset +
                     (origin.z * triangle->normal.z +
                      (triangle->normal.x * origin.x + triangle->normal.y * origin.y))) /
                   facing;
        /* The original stores the scaled direction before adding the origin to it. */
        hit.x = direction.x * distance;
        hit.y = direction.y * distance;
        hit.z = direction.z * distance;
        hit.x = origin.x + hit.x;
        hit.y = origin.y + hit.y;
        hit.z = origin.z + hit.z;
        if (!((f64)distance > -2.0) || !((f64)distance < 2.0) || !(distance < nearest)) {
            continue;
        }
        if (!((f64)triangle->normal.z > 0.2)) {
            continue;
        }
        if (!IsPointInTrackTriangle(triangle, &hit)) {
            continue;
        }
        wheel->ground = triangle;
        nearest = distance;
        wheel->surface = triangle->surface;
        wheel->groundNormal.x = triangle->normal.x;
        wheel->groundNormal.y = triangle->normal.y;
        wheel->groundNormal.z = triangle->normal.z;
        wheel->groundOffset = triangle->planeOffset;
    }
    return nearest;
}

/* The original picks the wheel by a switch without a default, and keeps the last one otherwise. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#pragma GCC diagnostic ignored "-Wuninitialized"
void UpdateWheelHeights(RigidBody *body) {
    /* The original reserves stack for sixteen bytes of locals it does not use. */
    f32 unused[4];
    RigidBody *wheel;
    f32 height;
    s32 i;

    (void)unused;
    for (i = 0; i < VEHICLE_WHEELS; i++) {
        switch (i) {
        case 0:
            wheel = body->wheels[0];
            break;
        case 1:
            wheel = body->wheels[1];
            break;
        case 2:
            wheel = body->wheels[2];
            break;
        case 3:
            wheel = body->wheels[3];
            break;
        }
        height = -CastWheelRay(body, wheel);
        wheel->suspension = height;
        if ((f64)height > 0.0) {
            height = 0.0f;
        }
        if ((f64)height < -0.4) {
            wheel->state.position.z = -0.4f;
        } else {
            wheel->state.position.z = height;
        }
    }
}
#pragma GCC diagnostic pop

void ComputeSpringForces(RigidBody *body) {
    RigidBodyForce *force = body->forces;
    RigidBody *wheel;
    f64 extension;
    f32 compression;
    s16 previous;
    s32 i;

    for (i = 0; i < VEHICLE_WHEELS; i++) {
        force->vector.y = 0.0f;
        force->vector.x = 0.0f;
        switch (i) {
        case 0:
            wheel = body->wheels[0];
            break;
        case 1:
            wheel = body->wheels[1];
            break;
        case 2:
            wheel = body->wheels[2];
            break;
        default:
            wheel = body->wheels[3];
            break;
        }
        previous = wheel->contactFrames;
        if (wheel->contactFrames < 100) {
            wheel->contactFrames++;
        }
        /* A wheel hanging at full droop is off the ground. */
        extension = (f64)wheel->suspension;
        if (extension <= -0.3999) {
            wheel->contactFrames = 0;
            extension = (f64)-0.3f;
        }
        if (0.0 < extension) {
            extension = (f64)0.0f;
        }
        compression = (f32)(extension - -0.3);
        if (compression < 0.0f) {
            compression = 0.0f;
        }
        force->vector.z =
            (f32)((f64)compression * (SIGN(compression) * (f64)compression)) * body->springRate;
        if (wheel->contactFrames && !previous) {
            body->landed = 0x80;
        }
        force = force->next;
    }
}

void ComputeDampingForces(RigidBody *body) {
    /* The original reserves stack for sixteen bytes of locals it does not use. */
    f32 unused[4];
    RigidBodyForce *force = body->forces;
    RigidBody *wheel;
    f32 velocity[3];
    s32 i;
    f32 damping;

    (void)unused;
    for (i = 0; i < VEHICLE_WHEELS; i++) {
        force->vector.y = 0.0f;
        force->vector.x = 0.0f;
        /* The original keeps the velocity as an array of three. */
        switch (i) {
        case 0:
            ComputeWheelLocalVelocity((Vec3f *)velocity, body, body->wheels[0]);
            wheel = body->wheels[0];
            break;
        case 1:
            ComputeWheelLocalVelocity((Vec3f *)velocity, body, body->wheels[1]);
            wheel = body->wheels[1];
            break;
        case 2:
            ComputeWheelLocalVelocity((Vec3f *)velocity, body, body->wheels[2]);
            wheel = body->wheels[2];
            break;
        default:
            ComputeWheelLocalVelocity((Vec3f *)velocity, body, body->wheels[3]);
            wheel = body->wheels[3];
            break;
        }
        if (wheel->contactFrames == 0) {
            damping = 0.0f;
        } else if (velocity[2] < 0.0f) {
            damping = 0.0f;
        } else {
            damping = velocity[2] * body->damping;
        }
        force->vector.z = damping;
        force = force->next;
    }
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

s32 TestObbIntersection(f32 rotation[3][3], Vec3f *offset, Vec3f *halfA, Vec3f *halfB) {
    f32 r[3][3];
    f32(*m)[3] = rotation;
    f32 tx = offset->x, ty = offset->y, tz = offset->z;
    f32 ax = halfA->x, ay = halfA->y, az = halfA->z;
    f32 bx = halfB->x, by = halfB->y, bz = halfB->z;
    s32 overlap = 1;

    r[0][0] = ABS(m[0][0]);
    r[0][1] = ABS(m[0][1]);
    r[0][2] = ABS(m[0][2]);
    r[1][0] = ABS(m[1][0]);
    r[1][1] = ABS(m[1][1]);
    r[1][2] = ABS(m[1][2]);
    r[2][0] = ABS(m[2][0]);
    r[2][1] = ABS(m[2][1]);
    r[2][2] = ABS(m[2][2]);
    /* The axes of the two boxes, in the order of the original. */
    overlap &= ABS(tx) <= ax + bx * r[0][0] + by * r[0][1] + r[0][2] * bz;
    overlap &= ABS(tx * m[0][0] + ty * m[1][0] + m[2][0] * tz) <=
               bx + ax * r[0][0] + ay * r[1][0] + r[2][0] * az;
    overlap &= ABS(ty) <= ay + bx * r[1][0] + by * r[1][1] + r[1][2] * bz;
    overlap &= ABS(tz) <= az + bx * r[2][0] + by * r[2][1] + r[2][2] * bz;
    overlap &= ABS(tx * m[0][1] + ty * m[1][1] + m[2][1] * tz) <=
               by + ax * r[0][1] + ay * r[1][1] + r[2][1] * az;
    overlap &= ABS(tx * m[0][2] + ty * m[1][2] + m[2][2] * tz) <=
               bz + ax * r[0][2] + ay * r[1][2] + r[2][2] * az;
    /* The products of their edges. */
    overlap &= ABS(tz * m[1][0] - m[2][0] * ty) <=
               ay * r[2][0] + az * r[1][0] + by * r[0][2] + bz * r[0][1];
    overlap &= ABS(tz * m[1][1] - m[2][1] * ty) <=
               ay * r[2][1] + az * r[1][1] + bx * r[0][2] + bz * r[0][0];
    overlap &= ABS(tz * m[1][2] - m[2][2] * ty) <=
               ay * r[2][2] + az * r[1][2] + bx * r[0][1] + by * r[0][0];
    overlap &= ABS(tx * m[2][0] - m[0][0] * tz) <=
               ax * r[2][0] + az * r[0][0] + by * r[1][2] + bz * r[1][1];
    overlap &= ABS(tx * m[2][1] - m[0][1] * tz) <=
               ax * r[2][1] + az * r[0][1] + bx * r[1][2] + bz * r[1][0];
    overlap &= ABS(tx * m[2][2] - m[0][2] * tz) <=
               ax * r[2][2] + az * r[0][2] + bx * r[1][1] + by * r[1][0];
    overlap &= ABS(ty * m[0][0] - m[1][0] * tx) <=
               ax * r[1][0] + ay * r[0][0] + by * r[2][2] + bz * r[2][1];
    overlap &= ABS(ty * m[0][1] - m[1][1] * tx) <=
               ax * r[1][1] + ay * r[0][1] + bx * r[2][2] + bz * r[2][0];
    overlap &= ABS(ty * m[0][2] - m[1][2] * tx) <=
               ax * r[1][2] + ay * r[0][2] + bx * r[2][1] + by * r[2][0];
    return overlap;
}

/* Collide one vehicle with another: share their speed, then push it by its impulse. */
static inline void
PushVehicle(RacerSlot *slot, RigidBodyState *state, Vec3f *shared, Vec3f *towards, Vec3f *normal) {
    Vec3f arm, local;

    /* The original reads the vehicle from its slot afresh at each use. */
    arm.x = towards->x * 2.5f;
    arm.y = towards->y * 2.5f;
    arm.z = towards->z * 2.5f;
    Mtx4fInverseRotate(&local, slot->vehicle->body.orientation, &arm);
    state->velocity.x = state->velocity.x - shared->x;
    state->velocity.y = state->velocity.y - shared->y;
    state->velocity.z = state->velocity.z - shared->z;
    memcpy(&slot->vehicle->body.stepStates[1], &slot->vehicle->body.state, sizeof(RigidBodyState));
    ApplyCollisionImpulse(&slot->vehicle->body, &local, normal, 0, 0.45f);
    memcpy(&slot->vehicle->body.state, &slot->vehicle->body.stepStates[1], sizeof(RigidBodyState));
    slot->vehicle->body.stepStates[1].velocity.x =
        shared->x + slot->vehicle->body.stepStates[1].velocity.x;
    slot->vehicle->body.stepStates[1].velocity.y =
        shared->y + slot->vehicle->body.stepStates[1].velocity.y;
    slot->vehicle->body.stepStates[1].velocity.z =
        shared->z + slot->vehicle->body.stepStates[1].velocity.z;
    slot->vehicle->body.stepStates[0].velocity.x = slot->vehicle->body.stepStates[1].velocity.x;
    slot->vehicle->body.stepStates[0].velocity.y = slot->vehicle->body.stepStates[1].velocity.y;
    slot->vehicle->body.stepStates[0].velocity.z = slot->vehicle->body.stepStates[1].velocity.z;
    state->velocity.x = shared->x + state->velocity.x;
    state->velocity.y = shared->y + state->velocity.y;
    state->velocity.z = shared->z + state->velocity.z;
}

void CollideVehicles(void) {
    RigidBodyState *sa, *sb;
    Vec3f apart, offset, local, extents, shared, back;
    f32 transposeB[3][3], rotationA[3][3], relative[3][3];
    f32 speedA, speedB, mean, closing;
    u8 volume;
    s32 i, j;

    /* The original reads each vehicle from its slot afresh at each use. */
    for (i = 0; i < g_dwNumVehicles; i++) {
        if (g_aRacerSlots[i].vehicle == NULL ||
            g_aRacerSlots[i].vehicle->ghostMode == VEHICLE_GHOST) {
            continue;
        }
        g_aRacerSlots[i].vehicle->body.framesSinceVehicleImpact++;
        sa = &g_aRacerSlots[i].vehicle->body.state;
        for (j = i + 1; j < g_dwNumVehicles; j++) {
            if (g_aRacerSlots[j].vehicle == NULL ||
                g_aRacerSlots[j].vehicle->ghostMode == VEHICLE_GHOST) {
                continue;
            }
            sb = &g_aRacerSlots[j].vehicle->body.state;
            apart.x = sa->position.x - sb->position.x;
            apart.y = sa->position.y - sb->position.y;
            apart.z = sa->position.z - sb->position.z;
            if (!(sqrtf(apart.z * apart.z + (apart.x * apart.x + apart.y * apart.y)) < 5.0f)) {
                continue;
            }
            extents = g_vVehicleObbHalfExtents;
            Transpose4x4Into3x3(transposeB, g_aRacerSlots[j].vehicle->body.orientation);
            CopyMatrix4x4To3x3(rotationA, g_aRacerSlots[i].vehicle->body.orientation);
            Mtx3fMultiply(relative, transposeB, rotationA);
            offset.x = sa->position.x - sb->position.x;
            offset.y = sa->position.y - sb->position.y;
            offset.z = sa->position.z - sb->position.z;
            Mtx3fMulVec3f(&local, rotationA, &offset);
            if (!TestObbIntersection(relative, &local, &extents, &extents)) {
                return;
            }
            /* They share the mean of their speeds along the line between them. */
            Vec3fNormalizeFast(&apart);
            speedA =
                apart.z * sa->velocity.z + (sa->velocity.x * apart.x + sa->velocity.y * apart.y);
            speedB = sb->velocity.x * apart.x + sb->velocity.y * apart.y + sb->velocity.z * apart.z;
            mean = (speedA + speedB) * 0.5f;
            shared.x = apart.x * mean;
            shared.y = apart.y * mean;
            shared.z = apart.z * mean;
            closing = mean < speedA ? -(mean - speedA) : mean - speedA;
            if (MAX_IMPACT_SPEED < closing) {
                closing = MAX_IMPACT_SPEED;
            }
            if (g_aRacerSlots[i].vehicle->body.framesSinceVehicleImpact >= 41) {
                volume = (u32)(127.0f * closing / MAX_IMPACT_SPEED + 128.0f);
                g_aRacerSlots[i].vehicle->body.vehicleImpactVolume = volume;
                g_aRacerSlots[j].vehicle->body.vehicleImpactVolume = volume;
            }
            g_aRacerSlots[i].vehicle->body.framesSinceVehicleImpact = 0;
            back.x = apart.x * -1.0f;
            back.y = apart.y * -1.0f;
            back.z = apart.z * -1.0f;
            PushVehicle(&g_aRacerSlots[i], sa, &shared, &back, &apart);
            PushVehicle(&g_aRacerSlots[j], sb, &shared, &apart, &back);
        }
    }
}
