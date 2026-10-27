#include "physics.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#define FORCE_PRODUCT(a, b) ((long double)(a) * (b))
#else
#define FORCE_PRODUCT(a, b) ((a) * (b))
#endif

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
