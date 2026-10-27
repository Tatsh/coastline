#ifndef COASTLINE_RIGIDBODY_H
#define COASTLINE_RIGIDBODY_H

/**
 * @file
 * The rigid bodies of the physics: their state, forces and inertia, and the quaternion and 3 by 3
 * matrix arithmetic they use. A 3 by 3 matrix is packed in nine words; the rotation part of a 4
 * by 4 matrix is its first three rows.
 */

#include "types.h"
#include "vecmath.h"

#ifdef __cplusplus
extern "C" {
#endif

/** A rotation as a unit quaternion. */
typedef struct {
    f32 w; /*!< The scalar part, the cosine of half the angle of the rotation. */
    f32 x; /*!< The x component of the vector part. */
    f32 y; /*!< The y component of the vector part. */
    f32 z; /*!< The z component of the vector part. */
} Quaternion;

/** The motion of a rigid body at an instant. */
typedef struct {
    Vec3f position;         /*!< The position of the body in the world. */
    Vec3f velocity;         /*!< The velocity of the body, in world units a second. */
    Quaternion orientation; /*!< The orientation of the body. */
    Vec3f angularVelocity;  /*!< The angular velocity, in radians a second about the world axes. */
    Quaternion spin;        /*!< The rate of change of the orientation. */
} RigidBodyState;

/** The frames a force can be given in. */
enum {
    RIGID_BODY_FORCE_WORLD, /*!< The vector of the force is in world axes. */
    RIGID_BODY_FORCE_BODY,  /*!< The vector of the force is in the axes of the body. */
};

/** A force on a rigid body, in a list. */
typedef struct RigidBodyForce {
    struct RigidBodyForce *next; /*!< The next force on the body, or NULL at the end of the list. */
    s32 frame;                   /*!< The frame of the vector, a RIGID_BODY_FORCE_ value. */
    Vec3f vector;                /*!< The direction and size of the force. */
    Vec3f point;                 /*!< Where the force acts, in the frame of the body. */
} RigidBodyForce;

/** The kinds of rigid body. A box may turn; a body of the last kind does not. */
enum {
    RIGID_BODY_BOX,         /*!< A box that turns, with the inertia of its sides. */
    RIGID_BODY_BOX_1,       /*!< A box like the first kind, used for the body of a vehicle. */
    RIGID_BODY_NO_ROTATION, /*!< A body that does not turn, used for a wheel. */
};

/**
 * A rigid body: the body of a vehicle or of one of its wheels. A wheel uses the ground, steering,
 * spin and suspension fields, the body of a vehicle the springs, box and collision fields.
 */
typedef struct RigidBody {
    u32 unreconstructed0;        /*!< Of unknown use; cleared on setup. */
    struct RigidBody *wheels[4]; /*!< The wheel bodies of the body of a vehicle, the rear two
                                      first and the front two after. */
    u32 unreconstructed14;       /*!< Of unknown use; cleared on setup. */
    RigidBodyForce *forces;      /*!< The list of forces applied at each step. */
    s32 kind;                    /*!< The kind of body, a RIGID_BODY_ value. */
    Vec3f size;                  /*!< The sides of the box the inertia is computed from. */
    f32 mass;                    /*!< The mass, 1000 for the body of a vehicle and 0 for a wheel. */
    f32 inertia[3][3];           /*!< The inertia tensor in the frame of the body. */
    f32 inverseInertia[3][3];    /*!< The inverse inertia tensor; setup computes only its
                                      diagonal. */
    RigidBodyState state;        /*!< The motion of the body at the current step. */
    f32 orientation[4][4];       /*!< The matrix of the orientation and position of the state. */
    Vec3f force;  /*!< The sum of the forces of a step, then the linear acceleration it gives. */
    Vec3f torque; /*!< The sum of the torques of a step, then the angular acceleration it gives. */
    RigidBodyState stepStates[2]; /*!< The states of the steps of the integration. */
    struct TrackTriangle *ground; /*!< The triangle of the track under a wheel, or NULL. */
    u8 surface;         /*!< The surface of the ground triangle, a TRACK_SURFACE_ value. */
    Vec3f groundNormal; /*!< The normal of the plane of the ground triangle. */
    f32 groundOffset;   /*!< The offset of the plane of the ground triangle along its normal. */
    s32 contactFrames;  /*!< The frames a wheel has touched the ground, up to 100, or 0 off the
                             ground. */
    f32 springRate; /*!< The rate of the suspension springs of the body of a vehicle, (4 times the
                         suspension stiffness plus 20) times 16000. */
    f32 damping;    /*!< The damping of the suspension of the body of a vehicle, -3000. */
    f32 steerAngle; /*!< The angle a front wheel is steered to, in radians. */
    f32 spinRate;   /*!< How fast a wheel spins, in radians a second. */
    f32 radius;     /*!< The radius of a wheel, 0.5 from setup. */
    f32 drive;      /*!< The drive the throttle gives a wheel. */
    f32 brake;      /*!< The braking of a wheel. */
    f32 rollAngle;  /*!< How far a wheel has turned, in degrees; how far the body of a vehicle
                         leans in a skid. */
    f32 suspension; /*!< How far below the body of a vehicle the ground is under a wheel. */
    Vec3f boxSize;  /*!< The box of the body of a vehicle the track collides with. */
    f32 boxCenterZ; /*!< The height of the middle of the collision box. */
    Vec3f impactPoint;      /*!< Where the last hard hit was. */
    s32 uprightFrames;      /*!< 35 while the vehicle is upright, then counting down each step it is
                                 upside down. */
    u8 impactSpeed;         /*!< How fast the last hit was, from 10 on. */
    u8 tireType;            /*!< The tyres of the car, from 1; a higher type has less grip. */
    u8 vehicleImpactVolume; /*!< The loudest hit of another vehicle since the sounds played. */
    u8 impactVolume;        /*!< The loudest hit of the track since the sounds played. */
    u8 framesSinceImpact;   /*!< The steps since the track was hit, up to 40. */
    u8 framesSinceVehicleImpact; /*!< The steps since another vehicle was hit. */
    u8 unreconstructed202;       /*!< Of unknown use; the vehicle sounds clear it while no
                                      effect other than a skid or a roll plays. */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    u8 unreconstructed203; /*!< An effect byte sampled by Windows replays. */
    u8 unreconstructed204; /*!< An effect byte sampled by Windows replays. */
    u8 replayEffects[3];   /*!< Additional effects sampled by Windows replays. */
#endif
    u8 landed;   /*!< Set to 0x80 when a wheel lands, then cleared when the sound plays. */
    u8 skidding; /*!< Set to 0x80 when the tyres skid. */
} RigidBody;

/**
 * An empty hook the inertia setup calls.
 *
 * @param data The inertia of the body, ignored.
 * @ghidraAddress USA: 0x80258070
 * @ghidraAddress EUR: 0x80259640
 * @ghidraAddress JPN: 0x80258fd0
 */
void StubPhysics1(void *data);

/**
 * The unit quaternion of the rotation of a matrix, from the largest of its diagonal terms.
 *
 * @param m The matrix.
 * @param q Receives the quaternion.
 * @ghidraAddress USA: 0x8021ff90
 * @ghidraAddress EUR: 0x80221290
 * @ghidraAddress JPN: 0x80220a80
 * @ghidraAddress BRD3D: 0x100765e0
 */
void MatrixToQuaternion(f32 m[4][4], Quaternion *q);

/**
 * An empty hook QuaternionToMatrix() calls.
 *
 * @param data The matrix just built, ignored.
 * @ghidraAddress USA: 0x80258078
 * @ghidraAddress EUR: 0x80259648
 * @ghidraAddress JPN: 0x80258fd8
 */
void StubPhysics2(void *data);

/**
 * The product of two quaternions.
 *
 * @param out Receives a times b, and may be either factor.
 * @param a The left factor.
 * @param b The right factor.
 * @ghidraAddress USA: 0x80258080
 * @ghidraAddress EUR: 0x80259650
 * @ghidraAddress JPN: 0x80258fe0
 * @ghidraAddress BRD3D: 0x10074090
 * @ghidraAddress BRGlide: 0x1006d2e0
 * @stub{BRGlide}
 */
void QuaternionMultiply(Quaternion *out, Quaternion *a, Quaternion *b);

/**
 * Make a quaternion a unit quaternion.
 *
 * @param q The quaternion, normalised in place.
 * @ghidraAddress USA: 0x8025813c
 * @ghidraAddress EUR: 0x8025970c
 * @ghidraAddress JPN: 0x8025909c
 * @ghidraAddress BRD3D: 0x100741b0
 * @ghidraAddress BRGlide: 0x1006d410
 */
void QuaternionNormalize(Quaternion *q);

/**
 * Make a vector a unit vector.
 *
 * @param v The non-zero vector, normalised in place.
 * @ghidraAddress USA: 0x802581cc
 * @ghidraAddress EUR: 0x8025979c
 * @ghidraAddress JPN: 0x8025912c
 * @ghidraAddress BRD3D: 0x10074250
 * @ghidraAddress BRGlide: 0x1006d4b0
 */
void Vec3fNormalizeFast(Vec3f *v);

/**
 * The rate of change of the orientation from the angular velocity: half of the product of the
 * angular velocity and the orientation.
 *
 * @param state The state whose spin is set.
 * @ghidraAddress USA: 0x80258248
 * @ghidraAddress EUR: 0x80259818
 * @ghidraAddress JPN: 0x802591a8
 * @ghidraAddress BRD3D: 0x100742d0
 * @ghidraAddress BRGlide: 0x1006d530
 */
void ComputeDeltaQuaternion(RigidBodyState *state);

/**
 * Add the force and torque of a body over a time to the velocity and angular velocity of a state.
 *
 * @param state The state to change.
 * @param body The body whose force and torque are added.
 * @param dt The time over which they act.
 * @ghidraAddress USA: 0x80258324
 * @ghidraAddress EUR: 0x802598f4
 * @ghidraAddress JPN: 0x80259284
 * @ghidraAddress BRD3D: 0x100743a0
 * @ghidraAddress BRGlide: 0x1006d600
 */
void ApplyRigidBodyImpulse(RigidBodyState *state, RigidBody *body, f32 dt);

/**
 * The 4 by 4 matrix of the orientation and position of a state.
 *
 * @param out Receives the matrix.
 * @param state The state.
 * @ghidraAddress USA: 0x802583dc
 * @ghidraAddress EUR: 0x802599ac
 * @ghidraAddress JPN: 0x8025933c
 * @ghidraAddress BRD3D: 0x10074450
 * @ghidraAddress BRGlide: 0x1006d6b0
 */
void QuaternionToMatrix(f32 out[4][4], RigidBodyState *state);

/**
 * Advance a state by a time, keeping its velocities.
 *
 * @param out Receives the advanced state.
 * @param in The state to advance.
 * @param dt The time to advance by.
 * @ghidraAddress USA: 0x80258530
 * @ghidraAddress EUR: 0x80259b00
 * @ghidraAddress JPN: 0x80259490
 * @ghidraAddress BRD3D: 0x100745f0
 * @ghidraAddress BRGlide: 0x1006d850
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
 * @ghidraAddress EUR: 0x80259c50
 * @ghidraAddress JPN: 0x802595e0
 * @ghidraAddress BRD3D: 0x100746e0
 * @ghidraAddress BRGlide: 0x1006d940
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
 * @ghidraAddress EUR: 0x80259c90
 * @ghidraAddress JPN: 0x80259620
 * @ghidraAddress BRD3D: 0x10074720
 * @ghidraAddress BRGlide: 0x1006d980
 */
void Mtx4fInverseRotate(Vec3f *out, f32 m[4][4], Vec3f *v);

/**
 * A vector times the rotation of a matrix, which turns a vector of the frame into the world.
 *
 * @param out Receives the vector in the world.
 * @param m The matrix.
 * @param v The vector in the frame of the matrix.
 * @ghidraAddress USA: 0x80258758
 * @ghidraAddress EUR: 0x80259d28
 * @ghidraAddress JPN: 0x802596b8
 * @ghidraAddress BRD3D: 0x10074770
 * @ghidraAddress BRGlide: 0x1006d9d0
 */
void Mtx3x3MulVec3f(Vec3f *out, f32 m[4][4], Vec3f *v);

/**
 * A point times a matrix, with its translation.
 *
 * @param out Receives the transformed point.
 * @param m The matrix.
 * @param v The point.
 * @ghidraAddress USA: 0x802587e8
 * @ghidraAddress EUR: 0x80259db8
 * @ghidraAddress JPN: 0x80259748
 * @ghidraAddress BRD3D: 0x100747c0
 * @ghidraAddress BRGlide: 0x1006da20
 */
void Mtx4x4TransformPoint(Vec3f *out, f32 m[4][4], Vec3f *v);

/**
 * A 3 by 3 matrix times a column vector.
 *
 * @param out Receives the product.
 * @param m The matrix.
 * @param v The vector.
 * @ghidraAddress USA: 0x802588b8
 * @ghidraAddress EUR: 0x80259e88
 * @ghidraAddress JPN: 0x80259818
 * @ghidraAddress BRD3D: 0x10074830
 * @ghidraAddress BRGlide: 0x1006da90
 */
void Mtx3fMulVec3f(Vec3f *out, f32 m[3][3], Vec3f *v);

/**
 * Turn the summed force and torque of a body into its accelerations.
 *
 * @param body The body whose sums become its accelerations.
 * @ghidraAddress USA: 0x80258950
 * @ghidraAddress EUR: 0x80259f20
 * @ghidraAddress JPN: 0x802598b0
 */
void ComputeRigidBodyAcceleration(RigidBody *body);

/**
 * Add a force and its torque to the sums of a body, and print both on the debug output.
 *
 * @param body The body.
 * @param force The force to add.
 * @ghidraAddress USA: 0x802589f4
 * @ghidraAddress EUR: 0x80259fc4
 * @ghidraAddress JPN: 0x80259954
 */
void ApplySingleForceWithDebug(RigidBody *body, RigidBodyForce *force);

/**
 * Add each force of the list of a body to its sums of force and torque, printing each on the
 * debug output.
 *
 * @param body The body.
 * @ghidraAddress USA: 0x80258bdc
 * @ghidraAddress EUR: 0x8025a1ac
 * @ghidraAddress JPN: 0x80259b3c
 */
void ApplyAllForcesToBody(RigidBody *body);

/**
 * Reset a body, and set the inertia of a box from its size and mass.
 *
 * @param body The body, with its kind, size, and mass already set.
 * @ghidraAddress USA: 0x80258c24
 * @ghidraAddress EUR: 0x8025a1f4
 * @ghidraAddress JPN: 0x80259b84
 * @ghidraAddress BRD3D: 0x10074870
 * @ghidraAddress BRGlide: 0x1006dad0
 */
void InitRigidBody(RigidBody *body);

/**
 * The matrix of the cross product with a vector.
 *
 * @param out Receives the matrix.
 * @param v The vector.
 * @ghidraAddress USA: 0x80258db0
 * @ghidraAddress EUR: 0x8025a380
 * @ghidraAddress JPN: 0x80259d10
 * @ghidraAddress BRD3D: 0x100749d0
 * @ghidraAddress BRGlide: 0x1006dc30
 */
void BuildSkewSymmetricMatrix(f32 out[3][3], Vec3f *v);

/**
 * Copy the rotation of a matrix, and its transpose.
 *
 * @param transpose Receives the transpose of the rotation.
 * @param copy Receives the rotation.
 * @param m The matrix.
 * @ghidraAddress USA: 0x80258e04
 * @ghidraAddress EUR: 0x8025a3d4
 * @ghidraAddress JPN: 0x80259d64
 * @ghidraAddress BRD3D: 0x10074a10
 * @ghidraAddress BRGlide: 0x1006dc70
 */
void TransposeCopyMatrix3x3(f32 transpose[3][3], f32 copy[3][3], f32 m[4][4]);

/**
 * The transpose of the rotation of a matrix.
 *
 * @param out Receives the transpose.
 * @param m The matrix.
 * @ghidraAddress USA: 0x80258e64
 * @ghidraAddress EUR: 0x8025a434
 * @ghidraAddress JPN: 0x80259dc4
 * @ghidraAddress BRD3D: 0x10074a50
 * @ghidraAddress BRGlide: 0x1006dcb0
 */
void Transpose4x4Into3x3(f32 out[3][3], f32 m[4][4]);

/**
 * Copy the rotation of a matrix.
 *
 * @param out Receives the rotation.
 * @param m The matrix.
 * @ghidraAddress USA: 0x80258eac
 * @ghidraAddress EUR: 0x8025a47c
 * @ghidraAddress JPN: 0x80259e0c
 * @ghidraAddress BRD3D: 0x10074a90
 * @ghidraAddress BRGlide: 0x1006dcf0
 */
void CopyMatrix4x4To3x3(f32 out[3][3], f32 m[4][4]);

/**
 * The product of two 3 by 3 matrices.
 *
 * @param out Receives a times b, and must be neither factor.
 * @param a The left factor.
 * @param b The right factor.
 * @ghidraAddress USA: 0x80258ef8
 * @ghidraAddress EUR: 0x8025a4c8
 * @ghidraAddress JPN: 0x80259e58
 * @ghidraAddress BRD3D: 0x10074ac0
 * @ghidraAddress BRGlide: 0x1006dd20
 * @stub{BRGlide}
 */
void Mtx3fMultiply(f32 out[3][3], f32 a[3][3], f32 b[3][3]);

/**
 * The difference of two 3 by 3 matrices.
 *
 * @param out Receives a minus b.
 * @param a The matrix subtracted from.
 * @param b The matrix subtracted.
 * @ghidraAddress USA: 0x80258f70
 * @ghidraAddress EUR: 0x8025a540
 * @ghidraAddress JPN: 0x80259ed0
 * @ghidraAddress BRD3D: 0x10074b20
 * @ghidraAddress BRGlide: 0x1006dd80
 */
void SubtractMatrix3x3(f32 out[3][3], f32 a[3][3], f32 b[3][3]);

/**
 * The inverse of a rigid transform whose axes are scaled.
 *
 * @param m The transform.
 * @param out Receives the inverse.
 * @param scale The factors the columns of the inverted rotation are scaled by.
 * @ghidraAddress USA: 0x80258fd4
 * @ghidraAddress EUR: 0x8025a5a4
 * @ghidraAddress JPN: 0x80259f34
 * @ghidraAddress BRD3D: 0x10074b70
 * @ghidraAddress BRGlide: 0x1006ddd0
 */
void MtxBuildScaledInverse(f32 m[4][4], f32 out[4][4], Vec3f *scale);

/**
 * Solve a system of three linear equations by Cramer's rule.
 *
 * @param out Receives the solution.
 * @param a The coefficients.
 * @param b The constants.
 * @ghidraAddress USA: 0x802590e8
 * @ghidraAddress EUR: 0x8025a6b8
 * @ghidraAddress JPN: 0x8025a048
 * @ghidraAddress BRD3D: 0x10074c10
 * @ghidraAddress BRGlide: 0x1006de70
 */
void SolveLinearSystem3x3(Vec3f *out, f32 a[3][3], Vec3f *b);

#ifdef __cplusplus
}
#endif

#endif
