#ifndef COASTLINE_VEHICLE_H
#define COASTLINE_VEHICLE_H

/**
 * @file
 * The vehicles of a race, the cars they are, and their place on the track.
 */

#include <PR/gbi.h>
#include <PR/ultratypes.h>

#include "input.h"
#include "render.h"
#include "rigidbody.h"
#include "system.h"
#include "track.h"
#include "vecmath.h"

/** The number of vehicles a race can hold. */
#define MAX_VEHICLES 4

/** The seasons of the championship, and the most races of a season. */
#define SEASONS 6
#define SEASON_RACES 4

/** The laps of a race whose times a vehicle keeps. */
#define VEHICLE_LAP_TIMES 5

/** The tracks a profile keeps records for: the five tracks and their mirrored versions. */
#define PROFILE_RECORD_TRACKS 10

/** The wheels of a vehicle. */
#define VEHICLE_WHEELS 4

/** The sides around a car that dent apart, by the direction of the blow. */
#define VEHICLE_DENT_SIDES 8

/** The levels of detail of the model of a car, the nearest first, and its display lists in each. */
#define VEHICLE_LODS 3
#define VEHICLE_DISPLAY_LISTS 10

/** The display lists of the model of a car that the renderer draws. */
enum {
    /** The details drawn only near the camera, at the nearest level. */
    VEHICLE_DL_NEAR_DETAIL = 3,
    /** The paint, under the reflection of the sky. */
    VEHICLE_DL_BODY,
    /** The beams of the headlights, at night or in the rain. */
    VEHICLE_DL_HEADLIGHT_BEAMS,
    /** The underside, when the camera is below the car. */
    VEHICLE_DL_UNDERSIDE,
    /** The lit, textured parts, cut out by the alpha of their textures. */
    VEHICLE_DL_TEXTURED = 8,
    /** The parts that take the specular highlight. */
    VEHICLE_DL_HIGHLIGHTED,
};

/**
 * The saved settings and progress of a player. Its fields join this structure as the routines that
 * use them are reconstructed.
 */
typedef struct {
    /**
     * The state of the championship across its years: negative when it was never set, and bit 0
     * set while the seasons race the tracks mirrored.
     */
    s32 championshipFlags;
    /** The season of the championship being raced, from 0. */
    u8 season;
    /** The race of the season being raced, from 0. */
    u8 round;
    /** The place of each race of each season, from 0 for the winner. */
    u8 seasonPlaces[SEASONS][SEASON_RACES];
    /** The points scored in each season. */
    u16 seasonPoints[SEASONS];
    u8 unreconstructed2A[0x2C - 0x2A];
    /** The time of each race of each season, in seconds. */
    f32 seasonTimes[SEASONS][SEASON_RACES];
    /** The best lap on each track, in seconds, or 0 when none was set. */
    f32 lapRecords[PROFILE_RECORD_TRACKS];
    u8 unreconstructedB4[0xCC - 0xB4];
    /** The unlocked cars, a bit for each from bit 0, and further unlocks above them. */
    u16 unlocks;
    /** The unlocked tracks, a bit for each. */
    u16 unlockedTracks;
    u8 unreconstructedD0[0xD4 - 0xD0];
    /** The options of the car the player chose last: the handling, gearbox, tyres and suspension.
     */
    s32 handling;
    s32 transmission;
    s32 tires;
    s32 suspension;
    /** The button layout the player chose on the controller screen. */
    s32 controllerLayout;
    /** The best race on each track, in seconds, or 0 when none was set. */
    f32 raceRecords[PROFILE_RECORD_TRACKS];
    u8 unreconstructed110[0x128 - 0x110];
} PlayerProfile;

/** The profile of the player being played. @ghidraAddress USA: 0x8031c5bc */
extern PlayerProfile *g_pActiveProfile;

/**
 * The model of a car as it is loaded from the cartridge. Its fields join this structure as they are
 * reconstructed.
 */
typedef struct {
    /** The triangles of the model, which the statistics of the frame count. */
    u32 triangleCount;
    u8 unreconstructed4[0x14 - 0x4];
    /** The textures of the model. */
    ModelTexture *textures;
    /** The display lists of each level of detail, by the VEHICLE_DL_ values. */
    Gfx *displayLists[VEHICLE_LODS][VEHICLE_DISPLAY_LISTS];
    u8 unreconstructed90[0xB0 - 0x90];
    /** How far forward of the vehicle the cockpit camera sits. */
    f32 cockpitForward;
    /** Half of how far forward of the vehicle the mirror camera sits. */
    f32 mirrorForward;
    /** How far above the vehicle the cockpit and mirror cameras sit. */
    f32 cockpitUp;
    /** The display list of a wheel, and of a wheel in the snow. */
    Gfx *wheelDisplayList;
    u8 unreconstructedC0[0xC4 - 0xC0];
    Gfx *snowWheelDisplayList;
    u8 unreconstructedC8[0xE0 - 0xC8];
    /**
     * Where the four wheels are drawn. The physics takes the second and the third as a front and a
     * rear wheel of one side and mirrors them across, and the shadow is sized by the x and y of the
     * first.
     */
    Vec3f wheels[VEHICLE_WHEELS];
    u8 unreconstructed110[0x112 - 0x110];
    /** The texture whose palette holds the colour of the paint. */
    u8 paintTexture;
    u8 unreconstructed113[0x11A - 0x113];
    /** The textures whose palettes hold the colors of the headlights and of the rear lights. */
    u8 headlightTexture;
    u8 rearLightTexture;
    u8 unreconstructed11C[0xDF88 - 0x11C];
} VehicleModel;

/** How a car shows the revolutions of its engine. */
enum {
    /** A needle over the dashboard. */
    TACHOMETER_NEEDLE,
    /** A bar of palette colors whose segments past the revolutions dim. */
    TACHOMETER_BAR_DIMMED,
    /** A bar of palette colors whose segments past the revolutions disappear. */
    TACHOMETER_BAR_HIDDEN,
};

/** A car the players can choose. Its fields join this structure as they are reconstructed. */
typedef struct {
    const char *name;
    u8 unreconstructed4[0x14 - 0x4];
    /** Where the model of the car is on the cartridge, and where the next asset starts. */
    u32 modelRom;
    u32 modelRomEnd;
    /** The size of the model once decompressed. */
    u32 modelSize;
    /**
     * Where the dashboard starts on the cartridge: the palettes of day and night, 256 colors
     * each, the face, and a cell for each gear, reverse first.
     */
    u32 dashboardRom;
    u8 unreconstructed24[0x28 - 0x24];
    u8 dashboardWidth;
    u8 dashboardHeight;
    /** Where the gear cell sits on the face. */
    s8 gearX;
    s8 gearY;
    u8 gearWidth;
    u8 gearHeight;
    /** The TACHOMETER_ kind. */
    u8 tachometer;
    union {
        /** Where the needle turns on the face. */
        u8 needleX;
        /** The segments of a bar, the last colors of the palette. */
        u8 barSegments;
    };
    u8 needleY;
    u8 unreconstructed31[0x34 - 0x31];
    /** The angles of the needle at 8000 revolutions a minute and at rest, in radians. */
    f32 needleFullAngle;
    f32 needleRestAngle;
    /** The wheels driven and the layout of the engine, as indices of their names. */
    s32 driveType;
    s32 engineLayout;
    /** The acceleration and the top speed, as the car selection screen shows them. */
    f32 acceleration;
    f32 maxSpeed;
    u8 unreconstructed4C[0x54 - 0x4C];
    /** The gears of the gearbox. */
    s32 gearCount;
    /** The length and half the width of the shadow of the car on the stage of a menu. */
    f32 shadowLength;
    f32 shadowHalfWidth;
} CarModel;

/** The cars the players can choose. */
#define CAR_MODELS 13

/** @ghidraAddress USA: 0x8028ae0c */
extern CarModel g_aCarModels[CAR_MODELS];

/** The number of cars in g_aCarModels. @ghidraAddress USA: 0x8028ae08 */
extern s32 g_dwCarCount;

/** The gears of a gearbox, neutral first. */
#define CAR_GEAR_RATIOS 7

/** The physics of a car. */
typedef struct {
    f32 gearRatios[CAR_GEAR_RATIOS];
    /** The torque of the engine as a cubic of its revolutions, the highest power first. */
    f32 torqueCurve[4];
    f32 finalDrive;
    s32 gearCount;
    /** Whether the throttle drives the rear wheels alone, rather than all four. */
    s32 rearWheelDrive;
    /** The box the track collides with, and the height of its middle. */
    Vec3f boxSize;
    f32 boxCenterZ;
    /** An index that picks an entry of tables kept for each track. */
    s32 tableIndex;
} CarPhysicsSpec;

/** @ghidraAddress USA: 0x8028b330 */
extern CarPhysicsSpec g_aCarPhysicsSpecs[CAR_MODELS];

/**
 * The models of the cars the vehicles drive, in slots. The link script places them.
 *
 * @ghidraAddress USA: 0x803c8000
 */
extern VehicleModel g_aVehicleModels[];

/** The car whose model each slot holds. @ghidraAddress USA: 0x8031b238 */
extern s32 g_adwVehicleModelSlotCars[];

/** The decompression of the model of a car into a slot. */
typedef struct {
    DecompressJob job;
    s32 slot;
    s32 car;
} VehicleModelLoad;

/** The slots the models of the cars of the menus load into. */
#define VEHICLE_MODEL_SLOTS 4

/** The cameras of a vehicle. */
enum {
    VEHICLE_CAMERA_COCKPIT,
    VEHICLE_CAMERA_CHASE,
    /** The cockpit camera moved forward to the bumper. */
    VEHICLE_CAMERA_BUMPER,
    /** A camera by the track that watches the vehicle and zooms with its distance. */
    VEHICLE_CAMERA_TRACKSIDE,
    VEHICLE_CAMERA_4,
    /** The rear-view mirror. */
    VEHICLE_CAMERA_MIRROR,
    VEHICLE_CAMERAS,
};

/** A racer of a race. Its fields join this structure as they are reconstructed. */
typedef struct {
    u8 unreconstructed0[0x60];
    /** The vehicle of the racer, or NULL. */
    struct Vehicle *vehicle;
    u8 unreconstructed64[0x68 - 0x64];
    /** The clock of the racer runs while the bits of RACER_CLOCK_STOPPED are clear. */
    u32 status;
    u8 unreconstructed6C[0x78 - 0x6C];
} RacerSlot;

/** The racers of the race. @ghidraAddress USA: 0x803239a0 */
extern RacerSlot g_aRacerSlots[];

/** The racers in the race. @ghidraAddress USA: 0x8028b7f0 */
extern s32 g_dwNumVehicles;

/** The vehicles of g_aVehicles the race uses. @ghidraAddress USA: 0x8028b7f4 */
extern s32 g_dwNumActiveVehicles;

/** The bits of the status of a racer that stop its clock. */
#define RACER_CLOCK_STOPPED 0x3

/** The bit of the status of a racer whose vehicle is held out of gear. */
#define RACER_HELD 0x1

/** The ghost mode of a vehicle that other vehicles pass through and that fades back in. */
#define VEHICLE_GHOST 2

/** The axes and the position of a wheel, the rows of its matrix. */
typedef struct {
    Vec3f forward;
    f32 forwardW;
    Vec3f side;
    f32 sideW;
    Vec3f up;
    f32 upW;
    Vec3f position;
    f32 positionW;
} WheelFrame;

/**
 * The segments of the spray a wheel throws, and the vertices of each, of which the first two are
 * drawn.
 */
#define SPRAY_SEGMENTS 9
#define SPRAY_SEGMENT_VERTICES 4

/** The forces on the bodies of a vehicle. */
enum {
    /** The springs of the four wheels, then gravity: the list of the body for its first pass. */
    VEHICLE_FORCE_SPRING_0,
    VEHICLE_FORCE_SPRING_1,
    VEHICLE_FORCE_SPRING_2,
    VEHICLE_FORCE_SPRING_3,
    /** The dampers of the four wheels, then the drag: the list for its second pass. */
    VEHICLE_FORCE_DAMPER_0,
    VEHICLE_FORCE_DAMPER_1,
    VEHICLE_FORCE_DAMPER_2,
    VEHICLE_FORCE_DAMPER_3,
    VEHICLE_FORCE_GRAVITY,
    /** The gravity of the front and of the rear wheels. */
    VEHICLE_FORCE_FRONT_WHEEL_GRAVITY,
    VEHICLE_FORCE_REAR_WHEEL_GRAVITY,
    VEHICLE_FORCE_DRAG,
    /** The drive of each wheel, before its gravity in its list. */
    VEHICLE_FORCE_WHEEL_0,
    VEHICLE_FORCE_WHEEL_1,
    VEHICLE_FORCE_WHEEL_2,
    VEHICLE_FORCE_WHEEL_3,
    VEHICLE_FORCES,
};

/**
 * A vehicle. The game prints its size, 0x2090 bytes, at start-up. Its fields join this structure as
 * the routines that use them are reconstructed.
 */
typedef struct Vehicle {
    union {
        /** The axes and position of the vehicle. */
        CameraFrame frame;
        struct {
            /** The frame as a matrix. */
            f32 matrix[4][4];
            union {
                f32 wheelMatrices[VEHICLE_WHEELS][4][4];
                WheelFrame wheelFrames[VEHICLE_WHEELS];
            };
        };
    };
    /** The index of the vehicle in g_aVehicles. The players have the first. */
    s32 index;
    u8 unreconstructed144[0x148 - 0x144];
    RigidBody body;
    /**
     * The bodies of the wheels: the rear and the front wheel of one side, then of the other. The
     * body lists them as the rear two, then the front two.
     */
    RigidBody wheelBodies[VEHICLE_WHEELS];
    /** The forces on the bodies, by the VEHICLE_FORCE_ values. */
    RigidBodyForce forces[VEHICLE_FORCES];
    u8 unreconstructedD70[0xDF0 - 0xD70];
    /** The angle the front wheels are steered to, in radians. */
    f32 steerAngle;
    /** The revolutions of the engine a minute. */
    f32 engineRpm;
    /** The gearbox and engine, from the physics spec of the car. */
    f32 gearRatios[CAR_GEAR_RATIOS];
    f32 torqueCurve[4];
    f32 finalDrive;
    s32 gearCount;
    s32 rearWheelDrive;
    /** The gearbox the player chose. */
    s32 transmission;
    s32 tableIndex;
    /** The throttle, below 0 in reverse. */
    f32 throttle;
    /** The brake, below 0 when braking. */
    f32 brake;
    /** The forward gear, from 0. */
    s32 gear;
    /**
     * The loads the front and rear tyres carried in the last step, and whether they slid; while
     * they slid, they grip less.
     */
    f32 frontLoad;
    u8 frontSliding;
    f32 rearLoad;
    u8 rearSliding;
    /** The way the steering returns towards the middle: 1, -1, or 0 while it does not. */
    s8 steerReturn;
    /** Whether the next step leaves the wheels undriven. */
    s32 skipDrive;
    /** Whether the player chose the custom decals of the car. */
    s32 customDecals;
    /** The profile of the player who drives the vehicle. */
    PlayerProfile *profile;
    /** The tyres, the stiffness of the springs, and how the steering answers the stick. */
    s32 tyreType;
    s32 suspensionStiffness;
    s32 steeringMode;
    /** The gearbox of the recording or the settings, which ResetVehicleState() takes. */
    s32 chosenTransmission;
    u8 unreconstructedE70[0xED0 - 0xE70];
    RacerSlot *racer;
    u8 unreconstructedED4[0xED8 - 0xED4];
    /** The routine that drives the vehicle each frame, the player's or the computer's, or NULL. */
    void (*driver)(struct Vehicle *vehicle);
    u8 unreconstructedEDC[0xF48 - 0xEDC];
    /** Set to 2 when the active camera changes. */
    s32 cameraChanged;
    /**
     * Whether the vehicle flies freely with the controls, without physics, and the cameras hold
     * rigidly to its frame.
     */
    s32 flyMode;
    /** The position of the last frame. */
    Vec3f previousPosition;
    /** The section of the route the vehicle is on, and its nearest point there. */
    TrackSection *section;
    s32 pointIndex;
    /** The direction of the track at that point. */
    Vec3f trackDirection;
    u8 unreconstructedF70[0xF78 - 0xF70];
    /** The laps finished. */
    s32 lap;
    /** The laps the distance travelled has counted. */
    s32 distanceLaps;
    /** The time of the lap being driven, in seconds. */
    f32 lapTime;
    /** The time of each lap finished, in seconds. */
    f32 lapTimes[VEHICLE_LAP_TIMES];
    /** The time of the best lap, in seconds, and which lap it was, from 0. */
    f32 bestLapTime;
    s32 bestLap;
    /** The time of the race so far, in seconds. */
    f32 totalTime;
    /** The time left, in seconds. */
    f32 timeLeft;
    /** The distance travelled along the route. */
    f32 distance;
    /** The place in the race, from 0 for the leader. */
    s32 racePosition;
    /** The message shown large in the middle of the view, or NULL, and the time it has left. */
    const char *message;
    f32 messageTime;
    /** The message shown smaller when there is no large one, or NULL, and the time it has left. */
    const char *subMessage;
    f32 subMessageTime;
    u8 unreconstructedFC0[0xFD4 - 0xFC0];
    /** The triangles of the track below the vehicle this frame. */
    s32 groundHits;
    /** The velocity SetVehicleVelocity() last gave. */
    Vec3f placedVelocity;
    /** The speed the display shows, in kilometers an hour. */
    f32 speedKph;
    u8 unreconstructedFE8[0x1010 - 0xFE8];
    /** The time towards the next puff of exhaust, and where the last left the exhaust. */
    f32 exhaustTimer;
    Vec3f exhaustPosition;
    /**
     * The effects of each wheel number the wheels as the front two, then the rear two. The time
     * towards the next puff of smoke of each wheel, and where the last puff was.
     */
    f32 smokeTimers[VEHICLE_WHEELS];
    Vec3f smokePositions[VEHICLE_WHEELS];
    /** The frames the spray of each wheel goes on after it leaves the ground, and its surface. */
    f32 sprayFramesLeft[VEHICLE_WHEELS];
    s32 spraySurfaces[VEHICLE_WHEELS];
    /** The time towards the next segment of each spray, and whether one began this frame. */
    f32 sprayPhases[VEHICLE_WHEELS];
    s32 sprayAdvanced[VEHICLE_WHEELS];
    /** Where the newest segment of each spray was thrown from. */
    Vec3f sprayPositions[VEHICLE_WHEELS];
    /**
     * The segments of each spray, the newest first: their vertices, in 127ths of a unit from
     * sprayOrigin, how far those move a second, in the same units, and the surface each segment
     * was thrown from, or -1 when it was thrown from none.
     */
    Vtx sprayVertices[VEHICLE_WHEELS][SPRAY_SEGMENTS][SPRAY_SEGMENT_VERTICES];
    s16 sprayVelocities[VEHICLE_WHEELS][SPRAY_SEGMENTS][SPRAY_SEGMENT_VERTICES][3];
    s16 spraySegmentSurfaces[VEHICLE_WHEELS][SPRAY_SEGMENTS];
    /** The point the spray is kept around, which SetVehiclePosition() puts at the vehicle. */
    Vec3f sprayOrigin;
    u8 unreconstructed1D84[0x1D88 - 0x1D84];
    /** The matrix of the spray, the identity until it is set. */
    Mtx sprayMtx;
    /** Whether the placed model under the vehicle is a tunnel, which darkens it. */
    s32 inTunnel;
    /** The heading of the camera of the vehicle, in radians, which turns the reflection of the sky.
     */
    f32 cameraHeading;
    u8 unreconstructed1DD0[0x1DD4 - 0x1DD0];
    /** The controls: the steering, the pitch, the accelerator and the roll. */
    f32 steering;
    f32 pitchInput;
    f32 accelerator;
    f32 rollInput;
    /** How thick the fog is at the vehicle, from 0 to 1, which fades its shadow. */
    f32 fogFactor;
    CameraFrame *activeCamera;
    /** The camera of the last frame. */
    CameraFrame *previousCamera;
    CameraFrame cameras[VEHICLE_CAMERAS];
    /** The speed the chase camera follows, smoothed. */
    f32 chaseSpeed;
    /** The lag of the chase camera, smoothed. */
    f32 chaseLag;
    /** How far the look-at point leads the vehicle. */
    f32 lookAhead;
    /** The point the chase camera looks at. */
    Vec3f lookAt;
    /** Where the chase camera was before the track pushed it. */
    Vec3f chasePosition;
    /** How hard the track has been pushing the chase camera. */
    f32 collisionPull;
    u8 unreconstructed1FB0[0x1FB4 - 0x1FB0];
    /** Where the chase camera started. */
    Vec3f initialChasePosition;
    /** What CastRayAgainstTrack() found below the vehicle. */
    u16 groundInstances[RAY_INSTANCES];
    s32 rayDone;
    u16 nearbyInstances[RAY_INSTANCES];
    s32 nearbyInstanceCount;
    f32 groundDistance;
    s32 groundSurface;
    /**
     * The corners of the rectangle of the screen the shadow of the vehicle covers in the view being
     * drawn: the left and the lower edge, then the right and the upper edge.
     */
    s16 shadowCorners[2][2];
    /** The car under the cursor of the car selection screen. */
    s32 selectedCar;
    /** The index of the car in g_aCarModels. */
    s32 carModel;
    /** The color of the paint of the car. */
    u8 color[3];
    /** VEHICLE_GHOST while the vehicle fades back in after it is put back on the track. */
    u8 ghostMode;
    /** How solid the vehicle is drawn, from 0 to 1. */
    f32 opacity;
    /**
     * Whether the car is drawn in its other look, without its textured parts or the reflection of
     * the sky, which a button switches once an unlock allows it.
     */
    s32 otherLook;
    /** The frames the vehicle has faced against the track. */
    s32 wrongWayFrames;
    /** The cell of the 32-unit grid of the track the vehicle is in. */
    u8 gridX;
    u8 gridY;
    u8 unreconstructed2072[0x2074 - 0x2072];
    PlayerInputState *input;
    VehicleModel *model;
    /** How deeply each of eight sides around the car has been dented; up to 0x100 it dents more. */
    s16 dents[VEHICLE_DENT_SIDES];
    /** A small offset, from -4 to 3, that changes with each dent so that dents vary. */
    s16 dentJitter;
    u8 unreconstructed208E[0x2090 - 0x208E];
} Vehicle;

/** @ghidraAddress USA: 0x8031b760 */
extern Vehicle g_aVehicles[MAX_VEHICLES];

/**
 * Find the nearest point of the route to a vehicle and add how far it went along the route to its
 * distance. A vehicle that has not crossed out of its point and stays near it keeps it.
 *
 * @param vehicle The vehicle.
 * @return 0 if no point is near enough, 1 otherwise.
 * @ghidraAddress USA: 0x8021eb50
 */
s32 UpdateVehicleTrackPosition(Vehicle *vehicle);

/**
 * Run the clocks of a vehicle as the mode of play keeps them.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x8021f0a8
 */
void UpdateRaceTimers(Vehicle *vehicle);

/**
 * Count down the time of the message of a vehicle, or failing one its smaller message, and clear it
 * at the end.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x8021f158
 */
void UpdateHudMessageTimers(Vehicle *vehicle);

/**
 * Show "WRONG WAY", flashing, once a vehicle has faced against the track for a while.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x8021f1f0
 */
void CheckWrongWayWarning(Vehicle *vehicle);

/**
 * Find the cell of the grid a vehicle is in, then its place on the route.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x8021f2d8
 */
void UpdateVehicleGridPosition(Vehicle *vehicle);

/**
 * Run a frame of a vehicle: fly it, or put it on the ground, drive its wheels and step its
 * physics. Then place its wheels, set its speed, and update its cameras if a view follows it.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x8021f998
 */
void UpdateVehiclePhysicsFrame(Vehicle *vehicle);

/**
 * Put a vehicle at a position, with its body and the states of its steps.
 *
 * @param vehicle The vehicle.
 * @param x The x of the position.
 * @param y The y of the position.
 * @param z The z of the position.
 * @ghidraAddress USA: 0x8021fe04
 */
void SetVehiclePosition(Vehicle *vehicle, f32 x, f32 y, f32 z);

/**
 * Turn a vehicle upright to face an angle about the vertical.
 *
 * @param vehicle The vehicle.
 * @param angle The angle from the x axis, in radians.
 * @ghidraAddress USA: 0x8021fe80
 */
void InitVehicleOrientationFromAngle(Vehicle *vehicle, f32 angle);

/**
 * Set the frame of a vehicle and the orientation of its body from a matrix.
 *
 * @param vehicle The vehicle.
 * @param m The matrix, whose rows are the axes and the position.
 * @ghidraAddress USA: 0x80220150
 */
void SetVehicleFrameMatrix(Vehicle *vehicle, f32 m[4][4]);

/**
 * Set the velocity of a vehicle.
 *
 * @param vehicle The vehicle.
 * @param x The x of the velocity.
 * @param y The y of the velocity.
 * @param z The z of the velocity.
 * @ghidraAddress USA: 0x802201c8
 */
void SetVehicleVelocity(Vehicle *vehicle, f32 x, f32 y, f32 z);

/**
 * Turn the body of a vehicle by angles about its vertical, sideways and forward axes, in turn.
 *
 * @param vehicle The vehicle.
 * @param yaw The angle about the vertical, in radians.
 * @param pitch The angle about the sideways axis, in radians.
 * @param roll The angle about the forward axis, in radians.
 * @ghidraAddress USA: 0x8022021c
 */
void RotateVehicleByEuler(Vehicle *vehicle, f32 yaw, f32 pitch, f32 roll);

/**
 * Set the angular velocity of a vehicle.
 *
 * @param vehicle The vehicle.
 * @param x The angular velocity about the x axis.
 * @param y The angular velocity about the y axis.
 * @param z The angular velocity about the z axis.
 * @ghidraAddress USA: 0x80220358
 */
void SetVehicleTurnRate(Vehicle *vehicle, f32 x, f32 y, f32 z);

/**
 * Paint the model of a vehicle its colour and set up its physics.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x80220398
 */
void InitVehicleColorAndPhysics(Vehicle *vehicle);

/**
 * Give a vehicle the model of a slot.
 *
 * @param vehicle The vehicle.
 * @param slot The slot of g_aVehicleModels.
 * @ghidraAddress USA: 0x802203f0
 */
void SetVehicleModelSlot(Vehicle *vehicle, s32 slot);

/**
 * Load the model of a car into a slot and give it to a vehicle.
 *
 * @param vehicle The vehicle.
 * @param slot The slot of g_aVehicleModels.
 * @param car The index of the car in g_aCarModels.
 * @ghidraAddress USA: 0x80220438
 */
void InstallVehicleModel(Vehicle *vehicle, s32 slot, s32 car);

/**
 * Start loading the model of a car into a slot through a buffer of g_aDmaWorkBuffers.
 *
 * @param load The load, which this starts.
 * @param slot The slot of g_aVehicleModels.
 * @param car The index of the car in g_aCarModels.
 * @param buffer The index of the buffer.
 * @ghidraAddress USA: 0x80220474
 */
void LoadVehicleModel(VehicleModelLoad *load, s32 slot, s32 car, s32 buffer);

/**
 * Whether the load of a model is done.
 *
 * @param load The load.
 * @return Whether no compressed bytes are left to read.
 * @ghidraAddress USA: 0x80220534
 */
s32 IsVehicleModelLoaded(VehicleModelLoad *load);

/**
 * Continue the load of a model, and once it is done, record the car of its slot and fix up its
 * pointers.
 *
 * @param load The load.
 * @return The compressed bytes left to read.
 * @ghidraAddress USA: 0x80220544
 */
u32 PollVehicleModelDma(VehicleModelLoad *load);

/**
 * Reset the frames, cameras, velocity, place on the route and gearbox of a vehicle for its car.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x80220620
 */
void ResetVehicleState(Vehicle *vehicle);

/**
 * Give a vehicle the input of the player of the same index.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x802207a4
 */
void BindPlayerInputToVehicle(Vehicle *vehicle);

/**
 * Paint the materials of a model a colour of 5 bits a channel.
 *
 * @param model The model.
 * @param red The red of the colour, from 0 to 31.
 * @param green The green of the colour, from 0 to 31.
 * @param blue The blue of the colour, from 0 to 31.
 * @ghidraAddress USA: 0x8021d140
 */
void SetMaterialColor(VehicleModel *model, s32 red, s32 green, s32 blue);

/**
 * Set the colour of the paint of a vehicle from the first colour of the palette of the paint
 * texture of a model, when that texture has a palette and is of the format 1.
 *
 * @param vehicle The vehicle.
 * @param model The model.
 * @ghidraAddress USA: 0x8021d2a0
 */
void ExtractVehicleBaseColor(Vehicle *vehicle, VehicleModel *model);

/**
 * Whether the model of a car is on the cartridge.
 *
 * @param car The index of the car in g_aCarModels.
 * @return Whether its model has an offset on the cartridge.
 * @ghidraAddress USA: 0x8021d6b8
 */
s32 IsVehicleModelValid(s32 car);

/**
 * Choose the car of a vehicle. A player's vehicle takes the first car of the season in the
 * championship and keeps its own otherwise. A computer's vehicle takes the car of the first player
 * in the arcade; otherwise the first takes the first car of the season, and the others the lowest
 * car of a set the season allows.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x80225f88
 */
void ChooseVehicleCar(Vehicle *vehicle);

/**
 * The driver of a computer's vehicle, which runs its behaviour for the frame.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x802288b4
 */
void DriveAiVehicle(Vehicle *vehicle);

/**
 * Turn the offsets of a loaded model into pointers.
 *
 * @param model The model.
 * @ghidraAddress USA: 0x8021d32c
 */
void FixupVehicleModelPointers(VehicleModel *model);

/**
 * Load the model of a car into a slot from the cartridge, or copy it from memory that holds it,
 * then fix up its pointers and record the car of the slot.
 *
 * @param slot The slot of g_aVehicleModels.
 * @param car The index of the car in g_aCarModels.
 * @param source The decompressed model to copy, or NULL to load it from the cartridge.
 * @ghidraAddress USA: 0x8021d5e4
 */
void LoadOrCopyVehicleModel(s32 slot, s32 car, void *source);

/**
 * How the steering answers the stick, as the steering mode of a vehicle sets it: the lock at rest
 * and how much of it the speed takes, in degrees, how fast the wheels turn a frame, in radians,
 * and the angle each unit of the stick steers, in degrees.
 *
 * @ghidraAddress USA: 0x8028b720
 */
extern f32 g_fSteerLock;

/** @ghidraAddress USA: 0x8028b724 */
extern f32 g_fSteerRate;

/** @ghidraAddress USA: 0x8028b728 */
extern f32 g_fSteerGain;

/** @ghidraAddress USA: 0x8028b72c */
extern f32 g_fSteerLockLoss;

/**
 * How much the speed halves the gain, which the modes leave at 0.
 *
 * @ghidraAddress USA: 0x8028b730
 */
extern s16 g_wSteerGainSpeedShare;

/**
 * Step the physics of a vehicle on the ground for a frame: drive its wheels, push it by its
 * springs, grip and dampers, collide it with the track and the other vehicles, and hang its
 * wheels to the ground.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x80221940
 */
void SimulateVehiclePhysicsSubstep(Vehicle *vehicle);

/**
 * Place the matrix of a vehicle, leant into its skid, and the matrices of its wheels, turned by
 * their roll and the front ones by their steering.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x80221e0c
 */
void TransformAllWheelMatrices(Vehicle *vehicle);

/**
 * Steer a vehicle by the stick, change its gears, and run its engine: its revolutions follow the
 * front wheels in gear, and its throttle the torque at them.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x80222050
 */
void UpdateVehicleEngineAndGearbox(Vehicle *vehicle);

/**
 * Set up the body of a vehicle and the bodies of its wheels, and their forces, for the model of its
 * car.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x80222d54
 */
void InitVehiclePhysics(Vehicle *vehicle);

/**
 * Bind a vehicle to the input of its player and reset it for a race.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x80226100
 */
void InitPlayerVehicleSlot(Vehicle *vehicle);

/**
 * Reset a vehicle, paint it the colour of its player, and leave it solid and undriven.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x802260a0
 */
void InitVehicleColor(Vehicle *vehicle);

/**
 * Drive a vehicle by the input of its player for a frame.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x80226d7c
 */
void UpdatePhysicsStep(Vehicle *vehicle);

#endif
