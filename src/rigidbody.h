#ifndef COASTLINE_RIGIDBODY_H
#define COASTLINE_RIGIDBODY_H

/**
 * @file
 * The rigid bodies of the physics: their state, forces and inertia, and the quaternion and 3 by 3
 * matrix arithmetic they use. A 3 by 3 matrix is packed in nine words; the rotation part of a 4
 * by 4 matrix is its first three rows.
 */

#include <PR/ultratypes.h>

#include "vecmath.h"

/** A rotation as a unit quaternion. */
typedef struct {
    f32 w;
    f32 x;
    f32 y;
    f32 z;
} Quaternion;

/** The motion of a rigid body at an instant. */
typedef struct {
    Vec3f position;
    Vec3f velocity;
    Quaternion orientation;
    Vec3f angularVelocity;
    /** The rate of change of the orientation. */
    Quaternion spin;
} RigidBodyState;

/** The frames a force can be given in. */
enum {
    RIGID_BODY_FORCE_WORLD,
    RIGID_BODY_FORCE_BODY,
};

/** A force on a rigid body, in a list. */
typedef struct RigidBodyForce {
    struct RigidBodyForce *next;
    s32 frame;
    Vec3f vector;
    /** Where the force acts, in the frame of the body. */
    Vec3f point;
} RigidBodyForce;

/** The kinds of rigid body. A box may turn; a body of the last kind does not. */
enum {
    RIGID_BODY_BOX,
    RIGID_BODY_BOX_1,
    RIGID_BODY_NO_ROTATION,
};

/** A rigid body. Its fields join this structure as they are reconstructed. */
typedef struct RigidBody {
    u32 unreconstructed0;
    /** The bodies of the wheels of the body of a vehicle. */
    struct RigidBody *wheels[4];
    u32 unreconstructed14;
    RigidBodyForce *forces;
    s32 kind;
    /** The sides of the box. */
    Vec3f size;
    f32 mass;
    f32 inertia[3][3];
    f32 inverseInertia[3][3];
    RigidBodyState state;
    /** The matrix of the orientation and position of the state. */
    f32 orientation[4][4];
    /** The sums of the forces and torques of a step. */
    Vec3f force;
    Vec3f torque;
    /** The states of the steps of the integration. */
    RigidBodyState stepStates[2];
    s32 resetWord19C;
    u8 unreconstructed1A0[0x1B4 - 0x1A0];
    s32 resetWord1B4;
    u8 unreconstructed1B8[0x1C0 - 0x1B8];
    f32 friction;
    f32 resetFloat1C4;
    f32 resetFloat1C8;
    /** The drive the throttle gives a wheel. */
    f32 drive;
    /** The braking of a wheel. */
    f32 brake;
    f32 resetFloat1D4;
    f32 resetFloat1D8;
} RigidBody;

/** An empty hook the inertia setup calls. @ghidraAddress USA: 0x80258070 */
void StubPhysics1(void *data);

/**
 * The unit quaternion of the rotation of a matrix, from the largest of its diagonal terms.
 *
 * @ghidraAddress USA: 0x8021ff90
 */
void MatrixToQuaternion(f32 m[4][4], Quaternion *q);

/** An empty hook QuaternionToMatrix() calls. @ghidraAddress USA: 0x80258078 */
void StubPhysics2(void *data);

/** @ghidraAddress USA: 0x80258080 */
void QuaternionMultiply(Quaternion *out, Quaternion *a, Quaternion *b);

/** Make a quaternion a unit quaternion. @ghidraAddress USA: 0x8025813c */
void QuaternionNormalize(Quaternion *q);

/** Make a vector a unit vector, which must not be zero. @ghidraAddress USA: 0x802581cc */
void Vec3fNormalizeFast(Vec3f *v);

/**
 * The rate of change of the orientation from the angular velocity: half of the product of the
 * angular velocity and the orientation.
 *
 * @ghidraAddress USA: 0x80258248
 */
void ComputeDeltaQuaternion(RigidBodyState *state);

/**
 * Add the force and torque of a body over a time to the velocity and angular velocity of a state.
 *
 * @ghidraAddress USA: 0x80258324
 */
void ApplyRigidBodyImpulse(RigidBodyState *state, RigidBody *body, f32 dt);

/**
 * The 4 by 4 matrix of the orientation and position of a state.
 *
 * @ghidraAddress USA: 0x802583dc
 */
void QuaternionToMatrix(f32 out[4][4], RigidBodyState *state);

/**
 * Advance a state by a time, keeping its velocities.
 *
 * @ghidraAddress USA: 0x80258530
 */
void IntegrateRigidBodyState(RigidBodyState *out, RigidBodyState *in, f32 dt);

/** @ghidraAddress USA: 0x80258680 */
void SetRigidBodyForce(
    RigidBodyForce *force, f32 x, f32 y, f32 z, f32 pointX, f32 pointY, f32 pointZ, s32 frame);

/**
 * The dot products of the rotation rows of a matrix with a vector, which turns a vector of the
 * world into the frame of the matrix.
 *
 * @ghidraAddress USA: 0x802586c0
 */
void Mtx4fInverseRotate(Vec3f *out, f32 m[4][4], Vec3f *v);

/**
 * A vector times the rotation of a matrix, which turns a vector of the frame into the world.
 *
 * @ghidraAddress USA: 0x80258758
 */
void Mtx3x3MulVec3f(Vec3f *out, f32 m[4][4], Vec3f *v);

/**
 * A point times a matrix, with its translation.
 *
 * @ghidraAddress USA: 0x802587e8
 */
void Mtx4x4TransformPoint(Vec3f *out, f32 m[4][4], Vec3f *v);

/** A 3 by 3 matrix times a column vector. @ghidraAddress USA: 0x802588b8 */
void Mtx3fMulVec3f(Vec3f *out, f32 m[3][3], Vec3f *v);

/**
 * Turn the summed force and torque of a body into its accelerations.
 *
 * @ghidraAddress USA: 0x80258950
 */
void ComputeRigidBodyAcceleration(RigidBody *body);

/**
 * Add a force and its torque to the sums of a body, and print both on the debug output.
 *
 * @ghidraAddress USA: 0x802589f4
 */
void ApplySingleForceWithDebug(RigidBody *body, RigidBodyForce *force);

/** @ghidraAddress USA: 0x80258bdc */
void ApplyAllForcesToBody(RigidBody *body);

/**
 * Reset a body, and set the inertia of a box from its size and mass.
 *
 * @ghidraAddress USA: 0x80258c24
 */
void InitRigidBody(RigidBody *body);

/** The matrix of the cross product with a vector. @ghidraAddress USA: 0x80258db0 */
void BuildSkewSymmetricMatrix(f32 out[3][3], Vec3f *v);

/**
 * Copy the rotation of a matrix, and its transpose.
 *
 * @ghidraAddress USA: 0x80258e04
 */
void TransposeCopyMatrix3x3(f32 transpose[3][3], f32 copy[3][3], f32 m[4][4]);

/** The transpose of the rotation of a matrix. @ghidraAddress USA: 0x80258e64 */
void Transpose4x4Into3x3(f32 out[3][3], f32 m[4][4]);

/** @ghidraAddress USA: 0x80258eac */
void CopyMatrix4x4To3x3(f32 out[3][3], f32 m[4][4]);

/** @ghidraAddress USA: 0x80258ef8 */
void Mtx3fMultiply(f32 out[3][3], f32 a[3][3], f32 b[3][3]);

/** @ghidraAddress USA: 0x80258f70 */
void SubtractMatrix3x3(f32 out[3][3], f32 a[3][3], f32 b[3][3]);

/**
 * The inverse of a rigid transform whose axes are scaled.
 *
 * @ghidraAddress USA: 0x80258fd4
 */
void MtxBuildScaledInverse(f32 m[4][4], f32 out[4][4], Vec3f *scale);

/**
 * Solve a system of three linear equations by Cramer's rule.
 *
 * @ghidraAddress USA: 0x802590e8
 */
void SolveLinearSystem3x3(Vec3f *out, f32 a[3][3], Vec3f *b);

#endif
