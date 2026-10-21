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

/**
 * A rigid body: the body of a vehicle or of one of its wheels. A wheel uses the ground, steering,
 * spin and suspension fields, the body of a vehicle the springs, box and collision fields.
 */
typedef struct RigidBody {
    u32 unreconstructed0;
    /** The bodies of the wheels of the body of a vehicle: the rear two, then the front two. */
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
    /** The sums of the forces and torques of a step, then the accelerations they give. */
    Vec3f force;
    Vec3f torque;
    union {
        /** The states of the steps of the integration. */
        RigidBodyState states[2];
        /**
         * The index in g_aVehicles of the vehicle whose chassis this is, which shares the memory
         * of the spin of the first step state with the physics.
         */
        struct {
            u8 unreconstructed0[0x2C];
            s32 vehicleIndex;
        } owner;
    } steps;
    /** The triangle of the track under a wheel, or NULL. */
    struct TrackTriangle *ground;
    /** The surface of that triangle, and its plane. */
    u8 surface;
    Vec3f groundNormal;
    f32 groundOffset;
    /** The frames a wheel has touched the ground, up to 100, or 0 off the ground. */
    s32 contactFrames;
    /** The springs and dampers of the suspension of the body of a vehicle. */
    f32 springRate;
    f32 damping;
    /** The angle a front wheel is steered to, in radians. */
    f32 steerAngle;
    /** How fast a wheel spins, in radians a second. */
    f32 spinRate;
    f32 radius;
    /** The drive the throttle gives a wheel. */
    f32 drive;
    /** The braking of a wheel. */
    f32 brake;
    /** How far a wheel has turned, in degrees; how far the body of a vehicle leans in a skid. */
    f32 rollAngle;
    /** How far below the body of a vehicle the ground is under a wheel. */
    f32 suspension;
    /** The box of the body of a vehicle the track collides with, and the height of its middle. */
    Vec3f boxSize;
    f32 boxCenterZ;
    /** Where the last hard hit was. */
    Vec3f impactPoint;
    /** 35 while the vehicle is upright, then counting down each step it is upside down. */
    s32 uprightFrames;
    /** How fast the last hit was, from 10 on. */
    u8 impactSpeed;
    /** The tyres of the car, from 1, which lose grip as they rise. */
    u8 tireType;
    /** The loudest hit of another vehicle and of the track since the sounds played. */
    u8 vehicleImpactVolume;
    u8 impactVolume;
    /** The steps since the track was hit, up to 40, and since another vehicle was. */
    u8 framesSinceImpact;
    u8 framesSinceVehicleImpact;
    u8 unreconstructed202;
    /** Set to 0x80 when a wheel lands, and when the tyres skid. */
    u8 landed;
    u8 skidding;
} RigidBody;

/**
 * An empty hook the inertia setup calls.
 *
 * @param data The inertia of the body, ignored.
 * @ghidraAddress USA: 0x80258070
 */
void StubPhysics1(void *data);

/**
 * The unit quaternion of the rotation of a matrix, from the largest of its diagonal terms.
 *
 * @param m The matrix.
 * @param q Receives the quaternion.
 * @ghidraAddress USA: 0x8021ff90
 */
void MatrixToQuaternion(f32 m[4][4], Quaternion *q);

/**
 * An empty hook QuaternionToMatrix() calls.
 *
 * @param data The matrix just built, ignored.
 * @ghidraAddress USA: 0x80258078
 */
void StubPhysics2(void *data);

/**
 * The product of two quaternions.
 *
 * @param out Receives a times b, and may be either factor.
 * @param a The left factor.
 * @param b The right factor.
 * @ghidraAddress USA: 0x80258080
 */
void QuaternionMultiply(Quaternion *out, Quaternion *a, Quaternion *b);

/**
 * Make a quaternion a unit quaternion.
 *
 * @param q The quaternion, normalised in place.
 * @ghidraAddress USA: 0x8025813c
 */
void QuaternionNormalize(Quaternion *q);

/**
 * Make a vector a unit vector.
 *
 * @param v The non-zero vector, normalised in place.
 * @ghidraAddress USA: 0x802581cc
 */
void Vec3fNormalizeFast(Vec3f *v);

/**
 * The rate of change of the orientation from the angular velocity: half of the product of the
 * angular velocity and the orientation.
 *
 * @param state The state whose spin is set.
 * @ghidraAddress USA: 0x80258248
 */
void ComputeDeltaQuaternion(RigidBodyState *state);

/**
 * Add the force and torque of a body over a time to the velocity and angular velocity of a state.
 *
 * @param state The state to change.
 * @param body The body whose force and torque are added.
 * @param dt The time over which they act.
 * @ghidraAddress USA: 0x80258324
 */
void ApplyRigidBodyImpulse(RigidBodyState *state, RigidBody *body, f32 dt);

/**
 * The 4 by 4 matrix of the orientation and position of a state.
 *
 * @param out Receives the matrix.
 * @param state The state.
 * @ghidraAddress USA: 0x802583dc
 */
void QuaternionToMatrix(f32 out[4][4], RigidBodyState *state);

/**
 * Advance a state by a time, keeping its velocities.
 *
 * @param out Receives the advanced state.
 * @param in The state to advance.
 * @param dt The time to advance by.
 * @ghidraAddress USA: 0x80258530
 */
void IntegrateRigidBodyState(RigidBodyState *out, RigidBodyState *in, f32 dt);

/**
 * Set the vector, the point, and the frame of a force.
 *
 * @param force The force to set.
 * @param x The x of the vector.
 * @param y The y of the vector.
 * @param z The z of the vector.
 * @param pointX The x of the point the force acts at, in the frame of the body.
 * @param pointY The y of the point.
 * @param pointZ The z of the point.
 * @param frame The frame of the vector, RIGID_BODY_FORCE_WORLD or RIGID_BODY_FORCE_BODY.
 * @ghidraAddress USA: 0x80258680
 */
void SetRigidBodyForce(
    RigidBodyForce *force, f32 x, f32 y, f32 z, f32 pointX, f32 pointY, f32 pointZ, s32 frame);

/**
 * The dot products of the rotation rows of a matrix with a vector, which turns a vector of the
 * world into the frame of the matrix.
 *
 * @param out Receives the vector in the frame of the matrix.
 * @param m The matrix.
 * @param v The vector in the world.
 * @ghidraAddress USA: 0x802586c0
 */
void Mtx4fInverseRotate(Vec3f *out, f32 m[4][4], Vec3f *v);

/**
 * A vector times the rotation of a matrix, which turns a vector of the frame into the world.
 *
 * @param out Receives the vector in the world.
 * @param m The matrix.
 * @param v The vector in the frame of the matrix.
 * @ghidraAddress USA: 0x80258758
 */
void Mtx3x3MulVec3f(Vec3f *out, f32 m[4][4], Vec3f *v);

/**
 * A point times a matrix, with its translation.
 *
 * @param out Receives the transformed point.
 * @param m The matrix.
 * @param v The point.
 * @ghidraAddress USA: 0x802587e8
 */
void Mtx4x4TransformPoint(Vec3f *out, f32 m[4][4], Vec3f *v);

/**
 * A 3 by 3 matrix times a column vector.
 *
 * @param out Receives the product.
 * @param m The matrix.
 * @param v The vector.
 * @ghidraAddress USA: 0x802588b8
 */
void Mtx3fMulVec3f(Vec3f *out, f32 m[3][3], Vec3f *v);

/**
 * Turn the summed force and torque of a body into its accelerations.
 *
 * @param body The body whose sums become its accelerations.
 * @ghidraAddress USA: 0x80258950
 */
void ComputeRigidBodyAcceleration(RigidBody *body);

/**
 * Add a force and its torque to the sums of a body, and print both on the debug output.
 *
 * @param body The body.
 * @param force The force to add.
 * @ghidraAddress USA: 0x802589f4
 */
void ApplySingleForceWithDebug(RigidBody *body, RigidBodyForce *force);

/**
 * Add each force of the list of a body to its sums of force and torque, printing each on the
 * debug output.
 *
 * @param body The body.
 * @ghidraAddress USA: 0x80258bdc
 */
void ApplyAllForcesToBody(RigidBody *body);

/**
 * Reset a body, and set the inertia of a box from its size and mass.
 *
 * @param body The body, with its kind, size, and mass already set.
 * @ghidraAddress USA: 0x80258c24
 */
void InitRigidBody(RigidBody *body);

/**
 * The matrix of the cross product with a vector.
 *
 * @param out Receives the matrix.
 * @param v The vector.
 * @ghidraAddress USA: 0x80258db0
 */
void BuildSkewSymmetricMatrix(f32 out[3][3], Vec3f *v);

/**
 * Copy the rotation of a matrix, and its transpose.
 *
 * @param transpose Receives the transpose of the rotation.
 * @param copy Receives the rotation.
 * @param m The matrix.
 * @ghidraAddress USA: 0x80258e04
 */
void TransposeCopyMatrix3x3(f32 transpose[3][3], f32 copy[3][3], f32 m[4][4]);

/**
 * The transpose of the rotation of a matrix.
 *
 * @param out Receives the transpose.
 * @param m The matrix.
 * @ghidraAddress USA: 0x80258e64
 */
void Transpose4x4Into3x3(f32 out[3][3], f32 m[4][4]);

/**
 * Copy the rotation of a matrix.
 *
 * @param out Receives the rotation.
 * @param m The matrix.
 * @ghidraAddress USA: 0x80258eac
 */
void CopyMatrix4x4To3x3(f32 out[3][3], f32 m[4][4]);

/**
 * The product of two 3 by 3 matrices.
 *
 * @param out Receives a times b, and must be neither factor.
 * @param a The left factor.
 * @param b The right factor.
 * @ghidraAddress USA: 0x80258ef8
 */
void Mtx3fMultiply(f32 out[3][3], f32 a[3][3], f32 b[3][3]);

/**
 * The difference of two 3 by 3 matrices.
 *
 * @param out Receives a minus b.
 * @param a The matrix subtracted from.
 * @param b The matrix subtracted.
 * @ghidraAddress USA: 0x80258f70
 */
void SubtractMatrix3x3(f32 out[3][3], f32 a[3][3], f32 b[3][3]);

/**
 * The inverse of a rigid transform whose axes are scaled.
 *
 * @param m The transform.
 * @param out Receives the inverse.
 * @param scale The factors the columns of the inverted rotation are scaled by.
 * @ghidraAddress USA: 0x80258fd4
 */
void MtxBuildScaledInverse(f32 m[4][4], f32 out[4][4], Vec3f *scale);

/**
 * Solve a system of three linear equations by Cramer's rule.
 *
 * @param out Receives the solution.
 * @param a The coefficients.
 * @param b The constants.
 * @ghidraAddress USA: 0x802590e8
 */
void SolveLinearSystem3x3(Vec3f *out, f32 a[3][3], Vec3f *b);

#endif
