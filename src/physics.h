#ifndef COASTLINE_PHYSICS_H
#define COASTLINE_PHYSICS_H

/**
 * @file
 * The physics of the vehicles: the forces on their bodies and wheels, the grip of the tyres, the
 * suspension, and the collisions with the track and with each other.
 *
 * A collision with the track works in the frame of the box of the body scaled to a unit cube: the
 * triangles of the track in the cache are moved into it, and those that cross the cube are kept in
 * a list for the steps of the frame.
 */

#include <PR/ultratypes.h>

#include "rigidbody.h"
#include "tricache.h"
#include "vecmath.h"

/** The weathers the grip of a surface differs in: dry, rain and snow. */
#define GRIP_WEATHERS 3

/** The surfaces of the track. */
#define TRACK_SURFACES 8

/** A node of the list of the triangles that cross the box of a body. */
typedef struct CollisionTriangleNode {
    TrackTriangle *triangle;            /*!< The triangle. */
    struct CollisionTriangleNode *next; /*!< The node pushed before this one, or NULL. */
} CollisionTriangleNode;

/** The nodes of the list. */
#define COLLISION_TRIANGLE_NODES 200

/** @ghidraAddress USA: 0x80379940 */
extern CollisionTriangleNode g_aCollisionTriangleNodes[COLLISION_TRIANGLE_NODES];

/** The list, the latest first. @ghidraAddress USA: 0x802a4a20 */
extern CollisionTriangleNode *g_pCollisionTriangleList;

/** The next free node. @ghidraAddress USA: 0x802a4a24 */
extern CollisionTriangleNode *g_pNextCollisionTriangleNode;

/**
 * How the last collision with a triangle is resolved: 1 against its plane alone, 2 at the face of
 * the box nearest the triangle, otherwise at the corner of the box.
 *
 * @ghidraAddress USA: 0x802a4a28
 */
extern s32 g_dwCollisionContactType;

/** The triangle of the last collision. @ghidraAddress USA: 0x802a4a2c */
extern TrackTriangle *g_pCollisionTriangle;

/**
 * Whether the triangles of the cache are collected from the last.
 *
 * @ghidraAddress USA: 0x802a4a30
 */
extern s32 g_bTrackCollisionFrameToggle;

/**
 * The grip of the tyres on each surface in each weather, the load below which they hold, and the
 * load above which they hold no less.
 *
 * @ghidraAddress USA: 0x802a4a38
 */
extern f32 g_afSurfaceGrip[GRIP_WEATHERS][TRACK_SURFACES];

/** @ghidraAddress USA: 0x802a4a98 */
extern f32 g_afSurfaceMaxLoad[GRIP_WEATHERS][TRACK_SURFACES];

/** @ghidraAddress USA: 0x802a4af8 */
extern f32 g_afSurfaceSlipLoad[GRIP_WEATHERS][TRACK_SURFACES];

/** The axes. @ghidraAddress USA: 0x802a4b58 */
extern Vec3f g_vecUnitX;

/** @ghidraAddress USA: 0x802a4b64 */
extern Vec3f g_vecUnitY;

/** @ghidraAddress USA: 0x802a4b70 */
extern Vec3f g_vecUnitZ;

/** @ghidraAddress USA: 0x802a4b94 */
extern Vec3f g_vecNegUnitZ;

/** The half extents of the box two vehicles collide by. @ghidraAddress USA: 0x802a4ba0 */
extern Vec3f g_vVehicleObbHalfExtents;

/**
 * Where the last collision with the track touched the body, in its frame.
 *
 * @ghidraAddress USA: 0x8037eaa8
 */
extern Vec3f g_vCollisionContact;

/**
 * The word after g_vCollisionContact, which ComputeCollisionPenetration() scales.
 *
 * @ghidraAddress USA: 0x8037eab4
 */
extern f32 g_fCollisionContactW;

/** How far the last collision put the body into the triangle. @ghidraAddress USA: 0x8037eac8 */
extern Vec3f g_vCollisionPenetration;

/**
 * Empty the cache of the triangles and the list of the triangles that cross the box.
 *
 * @ghidraAddress USA: 0x80259350
 */
void ResetCollisionCaches(void);

/**
 * Add the forces of the list of a body to its sums of force and torque.
 *
 * @param body The body whose forces are summed.
 * @ghidraAddress USA: 0x802594bc
 */
void SumBodyForces(RigidBody *body);

/**
 * Add the forces of a wheel, in the frame of the body, to the force of the wheel, and the turn of
 * their part along the ground to the torque of the body while the wheel touches the ground.
 *
 * @param body The body of the vehicle.
 * @param wheel The wheel whose forces are summed.
 * @ghidraAddress USA: 0x80259634
 */
void SumWheelForces(RigidBody *body, RigidBody *wheel);

/**
 * Turn the sums of the force and torque of the body of a vehicle, and the forces of its wheels,
 * into its accelerations, left in the sums.
 *
 * @param body The body of the vehicle.
 * @ghidraAddress USA: 0x8025980c
 */
void ComputeVehicleAccelerations(RigidBody *body);

/**
 * Sum the forces of the body of a vehicle and of its wheels, and turn them into its accelerations.
 *
 * @param body The body of the vehicle.
 * @ghidraAddress USA: 0x8025993c
 */
void ComputeVehicleForces(RigidBody *body);

/**
 * The velocity of the point of a body over a wheel at the height of the body, in the frame of the
 * body.
 *
 * @param out Receives the velocity, in the frame of the body.
 * @param body The body of the vehicle.
 * @param wheel The wheel the point lies over.
 * @ghidraAddress USA: 0x80259a18
 */
void ComputeWheelLocalVelocity(Vec3f *out, RigidBody *body, RigidBody *wheel);

/**
 * The velocity of the point of a body at a wheel.
 *
 * @param out Receives the velocity, in the world.
 * @param body The body of the vehicle.
 * @param wheel The wheel at whose position the velocity is taken.
 * @ghidraAddress USA: 0x80259b1c
 */
void GetWheelWorldVelocity(Vec3f *out, RigidBody *body, RigidBody *wheel);

/**
 * The velocity of a point of a body, given in its frame.
 *
 * @param out Receives the velocity, in the world.
 * @param body The body.
 * @param point The point, in the frame of the body.
 * @ghidraAddress USA: 0x80259c18
 */
void GetBodyPointVelocity(Vec3f *out, RigidBody *body, Vec3f *point);

/**
 * Cancel the sideways slip of the rear and the front of the body of a vehicle as far as the grip of
 * the tyres allows on their surfaces, and lean the body as it skids.
 *
 * @param body The body of the vehicle.
 * @param dt The time of the step.
 * @param rearLoad The load on the rear tyres, to which DriveWheel() adds.
 * @param frontLoad The load on the front tyres, to which DriveWheel() adds.
 * @param rearSliding Whether the rear tyres slid, updated for the next step.
 * @param frontSliding Whether the front tyres slid, updated for the next step.
 * @ghidraAddress USA: 0x80259d14
 */
void ApplySideGrip(
    RigidBody *body, f32 dt, f32 *rearLoad, f32 *frontLoad, u8 *rearSliding, u8 *frontSliding);

/**
 * Drive a wheel on the ground: push the body along the way the wheel rolls by its drive, as far as
 * the weight on it lets it, and spin and turn the wheel.
 *
 * @param body The body of the vehicle.
 * @param wheel The wheel to drive.
 * @param load The load of its tyres, to which half the push is added.
 * @param sliding Whether its tyres slid, in which case the push is cut by a tenth.
 * @param dt The time of the step.
 * @ghidraAddress USA: 0x8025ac9c
 */
void DriveWheel(RigidBody *body, RigidBody *wheel, f32 *load, u8 *sliding, f32 dt);

/**
 * Whether a point lies within a triangle of the track, seen along the main axis of its normal.
 *
 * @param triangle The triangle.
 * @param point The point, on the plane of the triangle.
 * @return 1 when the point lies within the triangle, otherwise 0.
 * @ghidraAddress USA: 0x8025b3b0
 */
s16 IsPointInTrackTriangle(TrackTriangle *triangle, Vec3f *point);

/**
 * The distance of a point from a plane.
 *
 * @param normal The unit normal of the plane.
 * @param offset The constant term of the equation of the plane.
 * @param point The point.
 * @return The signed distance, positive on the side the normal points to.
 * @ghidraAddress USA: 0x8025b704
 */
f32 PlaneDistance(Vec3f *normal, f32 offset, Vec3f *point);

/**
 * Stop the next step of a body moving into a plane, with a little bounce, record a hard hit, and
 * damp its spin about the plane.
 *
 * @param body The body whose next step bounces.
 * @param normal The unit normal of the plane, pointing out of it.
 * @param damp Whether the velocity is damped as well.
 * @param spin Whether the spin is damped.
 * @return Whether the body moved into the plane.
 * @ghidraAddress USA: 0x8025b73c
 */
s32 BounceOffPlane(RigidBody *body, Vec3f *normal, s32 damp, s32 spin);

/**
 * An empty collision response.
 *
 * @param body A body this routine ignores.
 * @param normal A normal this routine ignores.
 * @param damp A flag this routine ignores.
 * @return Always 1.
 * @ghidraAddress USA: 0x8025bba4
 */
s32 UnusedCollisionResponseStub(void *body, void *normal, void *damp);

/**
 * Give the next step of a body the impulse at a point of it, in its frame, that stops the point
 * moving into a surface, record a hard hit, and keep a part of the slide along the surface.
 *
 * @param body The body whose next step receives the impulse.
 * @param point The point of contact, in the frame of the body.
 * @param normal The unit normal of the surface.
 * @param friction Whether a fifth of the slide is retained, rather than none.
 * @param restitution How much the point bounces, more than the 1.05 it always does.
 * @return Whether the point moved into the surface.
 * @ghidraAddress USA: 0x8025bbb8
 */
s32 ApplyCollisionImpulse(
    RigidBody *body, Vec3f *point, Vec3f *normal, s32 friction, f32 restitution);

/**
 * Add a triangle to the list of those that cross the box.
 *
 * @param triangle The triangle to add.
 * @ghidraAddress USA: 0x8025c24c
 */
void PushCollisionTriangle(TrackTriangle *triangle);

/**
 * Print the length of the list on the debug output.
 *
 * @ghidraAddress USA: 0x8025c27c
 */
void PrintCollisionTriangleCount(void);

/**
 * Sort out a triangle against the unit cube by its corners and its edges.
 *
 * @param triangle The corners of the triangle in the frame of the unit cube.
 * @return 1 when a corner is inside the cube, 0 when the triangle is wholly outside it, and -1
 * when it is not yet known.
 * @ghidraAddress USA: 0x8025c2c8
 */
s32 ClassifyTriangleInUnitCube(Vec3f triangle[3]);

/**
 * How many times a triangle winds about a point of its plane, seen along the main axis of its
 * normal.
 *
 * @param triangle The corners of the triangle.
 * @param normal The normal of the triangle.
 * @param point The point, on the plane of the triangle.
 * @return The winding number, 0 for a point outside the triangle.
 * @ghidraAddress USA: 0x8025c8dc
 */
s32 CountTriangleWinding(Vec3f triangle[3], Vec3f *normal, Vec3f *point);

/**
 * Whether a segment passes through the unit cube.
 *
 * @param start One end of the segment.
 * @param end The other end of the segment.
 * @return 1 when the segment passes through the cube, otherwise 0.
 * @ghidraAddress USA: 0x8025cbf8
 */
s32 DoesSegmentCrossUnitCube(Vec3f *start, Vec3f *end);

/**
 * Whether an edge of a triangle, or a diagonal of the unit cube, crosses the other.
 *
 * @param triangle The corners of the triangle in the frame of the unit cube.
 * @param normal The normal of the triangle.
 * @return Nonzero when they cross, otherwise 0.
 * @ghidraAddress USA: 0x8025ce28
 */
s32 DoesTriangleCrossUnitCube(Vec3f triangle[3], Vec3f *normal);

/**
 * Whether a triangle crosses the unit cube.
 *
 * @param triangle The corners of the triangle in the frame of the unit cube.
 * @param normal The normal of the triangle.
 * @return Nonzero when the triangle crosses the cube, otherwise 0.
 * @ghidraAddress USA: 0x8025d018
 */
s32 IsTriangleInUnitCube(Vec3f triangle[3], Vec3f *normal);

/**
 * List the triangles of the cache under a body that cross its box.
 *
 * @param body The body whose position selects the cell of the cache.
 * @param box The matrix that takes the track into the unit cube of the box.
 * @return The triangles listed.
 * @ghidraAddress USA: 0x8025d060
 */
s32 CollectTrianglesInBox(RigidBody *body, f32 box[4][4]);

/**
 * An empty collision of a body with the track.
 *
 * @param body A body this routine ignores.
 * @return Always 0.
 * @ghidraAddress USA: 0x8025d35c
 */
s32 UnusedTrackCollisionStub(void *body);

/**
 * Roll a vehicle that stands still balanced on one or two wheels off its balance, printing what
 * it finds on the debug output.
 *
 * @param body The body of the vehicle.
 * @return Whether it was rolled.
 * @ghidraAddress USA: 0x8025d368
 */
s32 TipBalancedVehicle(RigidBody *body);

/**
 * Find how far the box of a body is into a triangle.
 *
 * @param body The body whose box scales the contact.
 * @param normal The unit normal of the triangle in the unit cube.
 * @param offset The distance of the plane of the triangle from the middle of the cube.
 * @param contact The corner or the face of the box in the unit cube.
 * @param triangle The corners of the triangle in the unit cube.
 * @ghidraAddress USA: 0x8025dcb8
 */
void ComputeCollisionPenetration(
    RigidBody *body, Vec3f *normal, f32 offset, Vec3f *contact, Vec3f triangle[3]);

/**
 * Collide the next step of a body with the listed triangles that cross its box, and move it back
 * out of those it hit.
 *
 * @param body The body to collide.
 * @param box The matrix that takes the track into the unit cube of the box.
 * @return Whether it hit one.
 * @ghidraAddress USA: 0x8025dfcc
 */
s32 CollideBodyWithTriangles(RigidBody *body, f32 box[4][4]);

/**
 * Step the body of a vehicle through the time of a frame in parts, colliding it with the track
 * and the other vehicles, and count how long it has been upside down.
 *
 * @param body The body of the vehicle.
 * @ghidraAddress USA: 0x8025e55c
 */
void StepVehicleCollisions(RigidBody *body);

/**
 * Set a force to the drag on the body of a vehicle, which grows on the surface that slows it.
 *
 * @param body The body of the vehicle.
 * @param force The force to set.
 * @ghidraAddress USA: 0x8025e820
 */
void ComputeDragForce(RigidBody *body, RigidBodyForce *force);

/**
 * Find the ground below a wheel in the cache of the triangles.
 *
 * @param body The body of the vehicle.
 * @param wheel The wheel, whose ground, surface, and plane are set to those found.
 * @return How far below the body the ground is, or 100 when there is none.
 * @ghidraAddress USA: 0x8025e96c
 */
f32 CastWheelRay(RigidBody *body, RigidBody *wheel);

/**
 * Find the ground below each wheel of a vehicle, and hang each wheel to it, no more than 0.4 down.
 *
 * @param body The body of the vehicle.
 * @ghidraAddress USA: 0x8025ec7c
 */
void UpdateWheelHeights(RigidBody *body);

/**
 * Set the springs of the wheels of a vehicle, and count the frames each wheel touches the ground.
 *
 * @param body The body of the vehicle.
 * @ghidraAddress USA: 0x8025edbc
 */
void ComputeSpringForces(RigidBody *body);

/**
 * Set the dampers of the wheels of a vehicle on the ground.
 *
 * @param body The body of the vehicle.
 * @ghidraAddress USA: 0x8025ef70
 */
void ComputeDampingForces(RigidBody *body);

/**
 * Whether two boxes overlap, by the separating axes of their faces and of the products of their
 * edges.
 *
 * @param rotation The rotation of the second box in the frame of the first.
 * @param offset The second box from the first, in the frame of the first.
 * @param halfA The half extents of the first box.
 * @param halfB The half extents of the second box.
 * @return 1 when the boxes overlap, otherwise 0.
 * @ghidraAddress USA: 0x8025f4f8
 */
s32 TestObbIntersection(f32 rotation[3][3], Vec3f *offset, Vec3f *halfA, Vec3f *halfB);

/**
 * Collide each pair of the vehicles of the race that are near, sharing their speed along the line
 * between them and pushing them apart. It stops at the first pair whose boxes do not meet.
 *
 * @ghidraAddress USA: 0x8025fde4
 */
void CollideVehicles(void);

#endif
