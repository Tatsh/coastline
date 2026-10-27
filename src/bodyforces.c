#include <PR/gu.h>

#include "physics.h"
#include "physicsimpact.h"
#include "track.h"
#include "vehiclemodel.h"
#include "weather.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
f32 g_fCollisionImpulseScale = 1.0f;

#define FORCE_PRODUCT(a, b) ((long double)(a) * (b))
#else
#define FORCE_PRODUCT(a, b) ((a) * (b))
#endif

#define SIGN(x) ((x) == 0.0f ? 0.0 : (0.0f < (x) ? 1.0 : -1.0))

s32 ApplyCollisionImpulse(
    RigidBody *body, Vec3f *point, Vec3f *normal, s32 friction, f32 restitution) {
    Vec3f *velocity = &body->stepStates[1].velocity;
    Vec3f *angular = &body->stepStates[1].angularVelocity;
    f32 transpose[3][3], rotation[3][3], skew[3][3], a[3][3], b[3][3], k[3][3], unit[3][3];
    Vec3f arm, copy, motion, across, target, impulse, linear, turn, spin;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    long double rawInto, highMotion[3], acrossX, slide[3], bounce;
    f32 inverseMass;
#else
    Vec3f unitZ, slide;
    f32 bounce;
#endif
    f32 into, speed;
    s32 i, j;

#if TGR_PLATFORM != TGR_PLATFORM_WIN
    /* The original copies the unit vector along z and never reads the copy. */
    unitZ = g_vecUnitZ;
    (void)unitZ;
#endif
    copy.x = point->x;
    copy.y = point->y;
    copy.z = point->z;
    TransposeCopyMatrix3x3(transpose, rotation, body->orientation);
    Mtx3fMulVec3f(&arm, transpose, &copy);
    /* How the point moves. */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    highMotion[0] =
        FORCE_PRODUCT(angular->y, arm.z) - FORCE_PRODUCT(arm.y, angular->z) + velocity->x;
    highMotion[1] =
        FORCE_PRODUCT(angular->z, arm.x) - FORCE_PRODUCT(arm.z, angular->x) + velocity->y;
    highMotion[2] =
        FORCE_PRODUCT(angular->x, arm.y) - FORCE_PRODUCT(arm.x, angular->y) + velocity->z;
    motion.x = highMotion[0];
    motion.y = highMotion[1];
    motion.z = highMotion[2];
    rawInto = FORCE_PRODUCT(normal->x, motion.x) +
              (normal->y * highMotion[1] + normal->z * highMotion[2]);
    if (0.0f <= rawInto) {
        return 0;
    }
#else
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

#endif

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

#if TGR_PLATFORM == TGR_PLATFORM_WIN
    into = Vec3fDot(normal, &motion);
    acrossX = FORCE_PRODUCT(normal->x, into);
#else
    into = motion.z * normal->z + (normal->x * motion.x + normal->y * motion.y);
    across.x = normal->x * into;
#endif
    across.y = normal->y * into;
    across.z = normal->z * into;
    speed = into < 0.0f ? -into : into;
    if (MAX_IMPACT_SPEED < speed) {
        speed = MAX_IMPACT_SPEED;
    }
    RecordImpact(body, speed);
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    if (body->framesSinceImpact >= 11 && !(restitution >= 0.0001f)) {
#else
    if (body->framesSinceImpact >= 11 && restitution < 0.0001f) {
#endif
        RecordImpactVolume(body, speed);
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        velocity->x *= g_fCollisionImpulseScale;
        velocity->y *= g_fCollisionImpulseScale;
        velocity->z *= g_fCollisionImpulseScale;
        acrossX *= 0.9f;
#else
        across.x = across.x * 0.9f;
#endif
        across.y = across.y * 0.9f;
        across.z = across.z * 0.9f;
        motion.x = motion.x * 0.9f;
        motion.y = motion.y * 0.9f;
        motion.z = motion.z * 0.9f;
    }
    /* Stop the point moving into the surface, and retain a part of its slide along it. */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    slide[0] = motion.x - acrossX;
    slide[1] = (long double)motion.y - across.y;
    slide[2] = (long double)motion.z - across.z;
    for (i = 0; i < 3; i++) {
        slide[i] = friction ? slide[i] * 0.2f : 0.0L;
    }
    target.x = acrossX + slide[0];
    target.y = across.y + slide[1];
    target.z = across.z + slide[2];
#else
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
#endif
    SolveLinearSystem3x3(&impulse, k, &target);
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    inverseMass = 1.0f / body->mass;
    linear.x = impulse.x * inverseMass;
    linear.y = impulse.y * inverseMass;
    linear.z = impulse.z * inverseMass;
    turn.x = FORCE_PRODUCT(arm.y, impulse.z) - FORCE_PRODUCT(impulse.y, arm.z);
    turn.y = FORCE_PRODUCT(arm.z, impulse.x) - FORCE_PRODUCT(impulse.z, arm.x);
    turn.z = FORCE_PRODUCT(arm.x, impulse.y) - FORCE_PRODUCT(impulse.x, arm.y);
#else
    linear.x = impulse.x * (1.0f / body->mass);
    linear.y = impulse.y * (1.0f / body->mass);
    linear.z = impulse.z * (1.0f / body->mass);
    turn.x = arm.y * impulse.z - impulse.y * arm.z;
    turn.y = arm.z * impulse.x - impulse.z * arm.x;
    turn.z = arm.x * impulse.y - impulse.x * arm.y;
#endif
    Mtx3fMulVec3f(&spin, b, &turn);
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    bounce = (long double)restitution - -1.05f;
    velocity->x = velocity->x - linear.x * bounce;
    velocity->y = velocity->y - linear.y * bounce;
    velocity->z = velocity->z - linear.z * bounce;
    angular->x = angular->x - spin.x * bounce;
    angular->y = angular->y - spin.y * bounce;
    angular->z = angular->z - spin.z * bounce;
#else
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
#endif
    return 1;
}

s32 BounceOffPlane(RigidBody *body, Vec3f *normal, s32 damp, s32 spin) {
    Vec3f *velocity = &body->stepStates[1].velocity;
    Vec3f *angular = &body->stepStates[1].angularVelocity;
    Vec3f impulse, pivot, scale, turn;
    f32 basis[4][4];
    f32 into, damping, speed;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    long double bounce, rawInto, reflected[3], tangent[3];
#else
    f64 bounce;
#endif

#if TGR_PLATFORM == TGR_PLATFORM_WIN
    rawInto = Vec3fDot(normal, velocity);
    into = (f32)rawInto;
    if (0.0f <= rawInto) {
        return 0;
    }
    bounce = FORCE_PRODUCT(into, 1.05f);
#else
    into = velocity->z * normal->z + (normal->x * velocity->x + normal->y * velocity->y);
    if (0.0f <= into) {
        return 0;
    }
    bounce = (f64)into * 1.05;
#endif
    impulse.x = (f32)((f64)normal->x * bounce);
    impulse.y = (f32)((f64)normal->y * bounce);
    impulse.z = (f32)((f64)normal->z * bounce);
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    reflected[0] = (long double)velocity->x - impulse.x;
    reflected[1] = (long double)velocity->y - impulse.y;
    reflected[2] = (long double)velocity->z - impulse.z;
    velocity->x = reflected[0];
    velocity->y = reflected[1];
    velocity->z = reflected[2];
#else
    velocity->x = velocity->x - impulse.x;
    velocity->y = velocity->y - impulse.y;
    velocity->z = velocity->z - impulse.z;
#endif
    if (body->framesSinceImpact < 10) {
        damping = 0.9f;
    } else {
        speed = into < 0.0f ? -into : into;
        if (MAX_IMPACT_SPEED < speed) {
            speed = MAX_IMPACT_SPEED;
        }
        RecordImpact(body, speed);
        RecordImpactVolume(body, speed);
        damping = 0.9f;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        velocity->x = reflected[0] * damping;
        velocity->y = reflected[1] * damping;
        velocity->z = reflected[2] * damping;
#else
        velocity->x = velocity->x * damping;
        velocity->y = velocity->y * damping;
        velocity->z = velocity->z * damping;
#endif
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
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    tangent[0] = FORCE_PRODUCT(normal->y, pivot.z) - FORCE_PRODUCT(pivot.y, normal->z);
    tangent[1] = FORCE_PRODUCT(normal->z, pivot.x) - FORCE_PRODUCT(pivot.z, normal->x);
    tangent[2] = FORCE_PRODUCT(normal->x, pivot.y) - FORCE_PRODUCT(pivot.x, normal->y);
    basis[1][0] = tangent[0];
    basis[1][1] = tangent[1];
    basis[1][2] = tangent[2];
    /* The second cross product mixes retained x87 values with the stored first row. */
    basis[2][0] = normal->y * tangent[2] - tangent[1] * normal->z;
    basis[2][1] = normal->z * tangent[0] - FORCE_PRODUCT(basis[1][2], normal->x);
    basis[2][2] = FORCE_PRODUCT(normal->x, basis[1][1]) - FORCE_PRODUCT(basis[1][0], normal->y);
#else
    basis[1][0] = normal->y * pivot.z - pivot.y * normal->z;
    basis[1][1] = normal->z * pivot.x - pivot.z * normal->x;
    basis[1][2] = normal->x * pivot.y - pivot.x * normal->y;
    basis[2][0] = normal->y * basis[1][2] - basis[1][1] * normal->z;
    basis[2][1] = normal->z * basis[1][0] - basis[1][2] * normal->x;
    basis[2][2] = normal->x * basis[1][1] - basis[1][0] * normal->y;
#endif
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

f32 CastWheelRay(RigidBody *body, RigidBody *wheel) {
    TrackTriangle *triangle;
    Vec3f down, start, origin, direction, hit;
    f32 nearest;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    long double distance, facing;
#else
    f32 distance, facing;
#endif
    s32 slot, count, i;

#if TGR_PLATFORM == TGR_PLATFORM_WIN
    down.x = 0.0f;
    down.y = 0.0f;
    down.z = -1.0f;
#else
    down = g_vecNegUnitZ;
#endif
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
        if (!(distance > -2.0) || !(distance < 2.0)) {
            continue;
        }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        facing = Vec3fDot(&triangle->normal, &direction);
#else
        facing = direction.z * triangle->normal.z +
                 (triangle->normal.x * direction.x + triangle->normal.y * direction.y);
#endif
        if (!((facing < 0 ? -facing : facing) > 0.001)) {
            continue;
        }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        distance =
            (f32)(-PlaneDistance(&triangle->normal, triangle->planeOffset, &origin) / facing);
        hit.x = FORCE_PRODUCT(direction.x, distance) + origin.x;
        hit.y = FORCE_PRODUCT(direction.y, distance) + origin.y;
        hit.z = FORCE_PRODUCT(direction.z, distance) + origin.z;
#else
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
#endif
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

/* The original picks the wheel by a switch without a default, and retains the last one otherwise. */
#pragma GCC diagnostic push
#if !defined(__clang__)
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
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
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        if (!(height >= -0.4f)) {
#else
        if ((f64)height < -0.4) {
#endif
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
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    long double extension, compression;
#else
    f64 extension;
    f32 compression;
#endif
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
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        if (!(extension > -0.3999000134110451)) {
#else
        if (extension <= -0.3999) {
#endif
            wheel->contactFrames = 0;
            extension = (f64)-0.3f;
        }
        if (0.0 < extension) {
            extension = (f64)0.0f;
        }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        compression = extension - -0.3f;
#else
        compression = (f32)(extension - -0.3);
#endif
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        if (!(compression >= 0.0f)) {
#else
        if (compression < 0.0f) {
#endif
            compression = 0.0f;
        }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        force->vector.z = compression * compression * body->springRate;
#else
        force->vector.z =
            (f32)((f64)compression * (SIGN(compression) * (f64)compression)) * body->springRate;
#endif
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
    Vec3f velocity;
    s32 i;
    f32 damping;

    (void)unused;
    for (i = 0; i < VEHICLE_WHEELS; i++) {
        force->vector.y = 0.0f;
        force->vector.x = 0.0f;
        switch (i) {
        case 0:
            ComputeWheelLocalVelocity(&velocity, body, body->wheels[0]);
            wheel = body->wheels[0];
            break;
        case 1:
            ComputeWheelLocalVelocity(&velocity, body, body->wheels[1]);
            wheel = body->wheels[1];
            break;
        case 2:
            ComputeWheelLocalVelocity(&velocity, body, body->wheels[2]);
            wheel = body->wheels[2];
            break;
        default:
            ComputeWheelLocalVelocity(&velocity, body, body->wheels[3]);
            wheel = body->wheels[3];
            break;
        }
        if (wheel->contactFrames == 0) {
            damping = 0.0f;
        } else if (velocity.z < 0.0f) {
            damping = 0.0f;
        } else {
            damping = velocity.z * body->damping;
        }
        force->vector.z = damping;
        force = force->next;
    }
}

void ComputeDragForce(RigidBody *body, RigidBodyForce *force) {
    s32 surface0, surface1, surface2, surface3;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    long double extra[3];
#else
    f32 extra[3];
#endif

    force->vector.x = body->state.velocity.x * -110.0f;
    force->vector.y = body->state.velocity.y * -110.0f;
    force->vector.z = body->state.velocity.z * -110.0f;
    surface0 = body->wheels[0]->surface;
    surface1 = body->wheels[1]->surface;
    surface2 = body->wheels[2]->surface;
    surface3 = body->wheels[3]->surface;
    if (4.0f < sqrtf(FORCE_PRODUCT(body->state.velocity.z, body->state.velocity.z) +
                     (FORCE_PRODUCT(body->state.velocity.x, body->state.velocity.x) +
                      FORCE_PRODUCT(body->state.velocity.y, body->state.velocity.y))) &&
        g_dwCurrentWeather != WEATHER_SNOW &&
        (surface0 == TRACK_SURFACE_SLOW || surface1 == TRACK_SURFACE_SLOW ||
         surface2 == TRACK_SURFACE_SLOW || surface3 == TRACK_SURFACE_SLOW)) {
        extra[0] = FORCE_PRODUCT(body->state.velocity.x, -220.0f);
        extra[1] = FORCE_PRODUCT(body->state.velocity.y, -220.0f);
        extra[2] = FORCE_PRODUCT(body->state.velocity.z, -220.0f);
        force->vector.x += extra[0];
        force->vector.y += extra[1];
        force->vector.z += extra[2];
    }
}

void SumBodyForces(RigidBody *body) {
    RigidBodyForce *force;
    Vec3f vector;
    Vec3f arm;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    long double torque[3];
#else
    f32 torque[3];
#endif

    for (force = body->forces; force != NULL; force = force->next) {
        switch (force->frame) {
        case RIGID_BODY_FORCE_WORLD:
            vector.x = force->vector.x;
            vector.y = force->vector.y;
            vector.z = force->vector.z;
            break;
        case RIGID_BODY_FORCE_BODY:
            Mtx3x3MulVec3f(&vector, body->orientation, &force->vector);
            break;
        }
        body->force.x = vector.x + body->force.x;
        body->force.y = vector.y + body->force.y;
        body->force.z = vector.z + body->force.z;
        if (body->kind != RIGID_BODY_NO_ROTATION) {
            Mtx3x3MulVec3f(&arm, body->orientation, &force->point);
            torque[0] = FORCE_PRODUCT(arm.y, vector.z) - FORCE_PRODUCT(vector.y, arm.z);
            torque[1] = FORCE_PRODUCT(arm.z, vector.x) - FORCE_PRODUCT(vector.z, arm.x);
            torque[2] = FORCE_PRODUCT(arm.x, vector.y) - FORCE_PRODUCT(vector.x, arm.y);
            body->torque.x = torque[0] + body->torque.x;
            body->torque.y = torque[1] + body->torque.y;
            body->torque.z = torque[2] + body->torque.z;
        }
    }
}

void SumWheelForces(RigidBody *body, RigidBody *wheel) {
    RigidBodyForce *force;
    Vec3f along, flat;
    Vec3f local;
    Vec3f position, arm;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    long double torque[3];
#else
    f32 torque[3];
#endif

    for (force = wheel->forces; force != NULL; force = force->next) {
        if (force->frame == RIGID_BODY_FORCE_WORLD) {
            Mtx4fInverseRotate(&local, body->orientation, &force->vector);
        }
        if (force->frame == RIGID_BODY_FORCE_BODY) {
            local.x = force->vector.x;
            local.y = force->vector.y;
            local.z = force->vector.z;
        }
        /* The part of the force along the ground turns the body. */
        flat.x = local.x;
        flat.y = local.y;
        flat.z = 0.0f;
        Mtx3x3MulVec3f(&along, body->orientation, &flat);
        wheel->force.x = local.x + wheel->force.x;
        wheel->force.y = local.y + wheel->force.y;
        wheel->force.z = local.z + wheel->force.z;
        if (wheel->contactFrames) {
            position.x = wheel->orientation[3][0];
            position.y = wheel->orientation[3][1];
            position.z = wheel->orientation[3][2];
            Mtx3x3MulVec3f(&arm, body->orientation, &position);
            torque[0] = FORCE_PRODUCT(arm.y, along.z) - FORCE_PRODUCT(along.y, arm.z);
            torque[1] = FORCE_PRODUCT(arm.z, along.x) - FORCE_PRODUCT(along.z, arm.x);
            torque[2] = FORCE_PRODUCT(arm.x, along.y) - FORCE_PRODUCT(along.x, arm.y);
            body->torque.x = torque[0] + body->torque.x;
            body->torque.y = torque[1] + body->torque.y;
            body->torque.z = torque[2] + body->torque.z;
        }
    }
}

void ComputeVehicleAccelerations(RigidBody *body) {
    Vec3f local;
    /* The original reserves stack for twelve bytes of locals it does not use. */
    f32 unused[3];
    Vec3f angular, spin;

    (void)unused;
    Mtx4fInverseRotate(&local, body->orientation, &body->force);
    body->force.x = local.x;
    body->force.y = local.y;
    body->force.z = local.z;
    /* The wheels move the body along the ground, not up and down. The acceleration reuses the
     * vector of the local force. */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    local.x = ((long double)body->wheels[2]->force.x + body->wheels[1]->force.x +
               body->wheels[0]->force.x + body->wheels[3]->force.x + body->force.x) /
              body->mass;
    local.y = ((long double)body->wheels[3]->force.y + body->wheels[0]->force.y +
               body->wheels[1]->force.y + body->wheels[2]->force.y + body->force.y) /
              body->mass;
#else
    local.x = (body->force.x + body->wheels[0]->force.x + body->wheels[1]->force.x +
               body->wheels[2]->force.x + body->wheels[3]->force.x) /
              body->mass;
    local.y = (body->force.y + body->wheels[0]->force.y + body->wheels[1]->force.y +
               body->wheels[2]->force.y + body->wheels[3]->force.y) /
              body->mass;
#endif
    local.z = body->force.z / body->mass;
    Mtx3x3MulVec3f(&body->force, body->orientation, &local);
    Mtx4fInverseRotate(&angular, body->orientation, &body->torque);
    Mtx3fMulVec3f(&spin, body->inverseInertia, &angular);
    Mtx3x3MulVec3f(&body->torque, body->orientation, &spin);
}

void ComputeVehicleForces(RigidBody *body) {
    body->force.z = 0.0f;
    body->force.y = 0.0f;
    body->force.x = 0.0f;
    body->torque.z = 0.0f;
    body->torque.y = 0.0f;
    body->torque.x = 0.0f;
    body->wheels[0]->force.x = 0.0f;
    body->wheels[0]->force.y = 0.0f;
    body->wheels[0]->force.z = 0.0f;
    body->wheels[1]->force.x = 0.0f;
    body->wheels[1]->force.y = 0.0f;
    body->wheels[1]->force.z = 0.0f;
    body->wheels[2]->force.x = 0.0f;
    body->wheels[2]->force.y = 0.0f;
    body->wheels[2]->force.z = 0.0f;
    body->wheels[3]->force.x = 0.0f;
    body->wheels[3]->force.y = 0.0f;
    body->wheels[3]->force.z = 0.0f;
    SumBodyForces(body);
    SumWheelForces(body, body->wheels[0]);
    SumWheelForces(body, body->wheels[1]);
    SumWheelForces(body, body->wheels[2]);
    SumWheelForces(body, body->wheels[3]);
    ComputeVehicleAccelerations(body);
}

/* The velocity the spin of a body gives a point at an arm from its middle. */
#define SPIN_VELOCITY(turn, body, arm)                                                             \
    (turn)[0] = FORCE_PRODUCT((body)->state.angularVelocity.y, (arm).z) -                          \
                FORCE_PRODUCT((arm).y, (body)->state.angularVelocity.z);                           \
    (turn)[1] = FORCE_PRODUCT((body)->state.angularVelocity.z, (arm).x) -                          \
                FORCE_PRODUCT((arm).z, (body)->state.angularVelocity.x);                           \
    (turn)[2] = FORCE_PRODUCT((body)->state.angularVelocity.x, (arm).y) -                          \
                FORCE_PRODUCT((arm).x, (body)->state.angularVelocity.y)

/* Copy the velocity of a body to a vector. */
#define COPY_BODY_VELOCITY(out, body)                                                              \
    (out)->x = (body)->state.velocity.x;                                                           \
    (out)->y = (body)->state.velocity.y;                                                           \
    (out)->z = (body)->state.velocity.z

void ComputeWheelLocalVelocity(Vec3f *out, RigidBody *body, RigidBody *wheel) {
    Vec3f point;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    long double turn[3];
#else
    f32 turn[3];
#endif
    Vec3f arm;

    point.x = wheel->state.position.x;
    point.y = wheel->state.position.y;
    point.z = 0.0f;
    Mtx3x3MulVec3f(&arm, body->orientation, &point);
    COPY_BODY_VELOCITY(out, body);
    SPIN_VELOCITY(turn, body, arm);
    point.x = turn[0] + out->x;
    point.y = turn[1] + out->y;
    point.z = turn[2] + out->z;
    Mtx4fInverseRotate(out, body->orientation, &point);
}

void GetWheelWorldVelocity(Vec3f *out, RigidBody *body, RigidBody *wheel) {
    Vec3f point;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    long double turn[3];
#else
    f32 turn[3];
#endif
    Vec3f arm;

    point.x = wheel->state.position.x;
    point.y = wheel->state.position.y;
    point.z = wheel->state.position.z;
    Mtx3x3MulVec3f(&arm, body->orientation, &point);
    COPY_BODY_VELOCITY(out, body);
    SPIN_VELOCITY(turn, body, arm);
    out->x = turn[0] + out->x;
    out->y = turn[1] + out->y;
    out->z = turn[2] + out->z;
}

#if TGR_PLATFORM == TGR_PLATFORM_WIN
long double PlaneDistance(Vec3f *normal, f32 offset, Vec3f *point) {
    return Vec3fDot(normal, point) + offset;
}
#else
f32 PlaneDistance(Vec3f *normal, f32 offset, Vec3f *point) {
    return point->z * normal->z + (normal->x * point->x + normal->y * point->y) + offset;
}

#endif

void GetBodyPointVelocity(Vec3f *out, RigidBody *body, Vec3f *point) {
    Vec3f copy;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    long double turn[3];
#else
    f32 turn[3];
#endif
    Vec3f arm;

    copy.x = point->x;
    copy.y = point->y;
    copy.z = point->z;
    Mtx3x3MulVec3f(&arm, body->orientation, &copy);
    COPY_BODY_VELOCITY(out, body);
    SPIN_VELOCITY(turn, body, arm);
    out->x = turn[0] + out->x;
    out->y = turn[1] + out->y;
    out->z = turn[2] + out->z;
}
