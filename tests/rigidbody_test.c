/* Host check of src/rigidbody.c. */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "rigidbody.h"

int osSyncPrintf(const char *format, ...) {
    return 0;
}

static int near(f32 a, f32 b) {
    return fabsf(a - b) < 1e-4f;
}

int main(void) {
    static RigidBody body;
    RigidBodyState state = {{1, 2, 3}, {0, 0, 0}, {1, 0, 0, 0}, {0, 0, 0}, {0, 0, 0, 0}};
    RigidBodyForce push;
    Quaternion a = {0.7071068f, 0.7071068f, 0, 0}, out;
    f32 m[4][4], system[3][3] = {{2, 1, 0}, {1, 3, 1}, {0, 1, 4}};
    Vec3f solution, rhs = {3, 5, 5}, v = {0, 1, 0}, rotated;

    /* Two quarter turns about x make a half turn. */
    QuaternionMultiply(&out, &a, &a);
    assert(near(out.w, 0) && near(out.x, 1));

    QuaternionToMatrix(m, &state);
    assert(m[0][0] == 1 && m[1][1] == 1 && m[2][2] == 1 && m[3][0] == 1 && m[3][2] == 3);
    state.orientation = a;
    QuaternionToMatrix(m, &state);
    Mtx3x3MulVec3f(&rotated, m, &v);
    assert(near(rotated.y, 0) && near(fabsf(rotated.z), 1));

    SolveLinearSystem3x3(&solution, system, &rhs);
    assert(near(solution.x, 1) && near(solution.y, 1) && near(solution.z, 1));

    body.kind = RIGID_BODY_BOX;
    body.mass = 12;
    body.size.x = 1;
    body.size.y = 2;
    body.size.z = 3;
    InitRigidBody(&body);
    assert(near(body.inertia[0][0], 13) && near(body.inertia[1][1], 10) &&
           near(body.inertia[2][2], 5));
    assert(near(body.inverseInertia[2][2], 0.2f) && near(body.steerAngle, 0.174f));

    /* A force along y at x = 1 turns the body about z. */
    memset(body.orientation, 0, sizeof(body.orientation));
    body.orientation[0][0] = body.orientation[1][1] = body.orientation[2][2] = 1;
    SetRigidBodyForce(&push, 0, 12, 0, 1, 0, 0, RIGID_BODY_FORCE_WORLD);
    push.next = NULL;
    body.forces = &push;
    ApplyAllForcesToBody(&body);
    assert(body.force.y == 12 && near(body.torque.z, 12));
    ComputeRigidBodyAcceleration(&body);
    assert(near(body.force.y, 1) && near(body.torque.z, 12 * 0.2f));

    state.orientation = (Quaternion){1, 0, 0, 0};
    state.velocity = (Vec3f){1, 0, 0};
    state.angularVelocity = (Vec3f){0, 0, 2};
    ComputeDeltaQuaternion(&state);
    assert(near(state.spin.z, 1));
    IntegrateRigidBodyState(&state, &state, 0.5f);
    assert(near(state.position.x, 1.5f));
    assert(near(
        state.orientation.w * state.orientation.w + state.orientation.z * state.orientation.z, 1));
    puts("rigidbody ok");
    return 0;
}
