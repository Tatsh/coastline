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

/**
 * The saved settings and progress of a player. Its fields join this structure as the routines that
 * use them are reconstructed.
 */
typedef struct {
    u8 unreconstructed0[0xCC];
    /** The unlocked cars, a bit for each from bit 0, and further unlocks above them. */
    u16 unlocks;
    /** The unlocked tracks, a bit for each. */
    u16 unlockedTracks;
    u8 unreconstructedD0[0xE4 - 0xD0];
    /** The button layout the player chose on the controller screen. */
    s32 controllerLayout;
} PlayerProfile;

/**
 * The model of a car as it is loaded from the cartridge. Its fields join this structure as they are
 * reconstructed.
 */
typedef struct {
    u8 unreconstructed0[0xB0];
    /** How far forward of the vehicle the cockpit camera sits. */
    f32 cockpitForward;
    /** Half of how far forward of the vehicle the mirror camera sits. */
    f32 mirrorForward;
    /** How far above the vehicle the cockpit and mirror cameras sit. */
    f32 cockpitUp;
    u8 unreconstructedBC[0xEC - 0xBC];
    /** A front and a rear wheel of one side; those of the other side mirror them across. */
    Vec3f frontWheel;
    Vec3f rearWheel;
    u8 unreconstructed104[0xDF88 - 0x104];
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
    u8 unreconstructed3C[0x60 - 0x3C];
} CarModel;

/** The cars the players can choose. */
#define CAR_MODELS 13

/** @ghidraAddress USA: 0x8028ae0c */
extern CarModel g_aCarModels[CAR_MODELS];

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

/** The bits of the status of a racer that stop its clock. */
#define RACER_CLOCK_STOPPED 0x3

/** The bit of the status of a racer whose vehicle is held out of gear. */
#define RACER_HELD 0x1

/** The wheels of a vehicle: the front two, then the rear two. */
#define VEHICLE_WHEELS 4

/** The ghost mode of a vehicle that other vehicles pass through and that fades back in. */
#define VEHICLE_GHOST 2

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
            f32 wheelMatrices[VEHICLE_WHEELS][4][4];
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
    u8 unreconstructedE58[0xE5C - 0xE58];
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
    u8 unreconstructedED4[0xF48 - 0xED4];
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
    u8 unreconstructedF84[0xF98 - 0xF84];
    /** The time of the best lap, in seconds. */
    f32 bestLapTime;
    u8 unreconstructedF9C[0xFA0 - 0xF9C];
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
    u8 unreconstructedFE8[0x1D78 - 0xFE8];
    /** The position SetVehiclePosition() last gave. */
    Vec3f placedPosition;
    u8 unreconstructed1D84[0x1D88 - 0x1D84];
    /** A matrix of the vehicle for the display lists, the identity until it is set. */
    Mtx transformMtx;
    u8 unreconstructed1DC8[0x1DD4 - 0x1DC8];
    /** The controls: the steering, the pitch, the accelerator and the roll. */
    f32 steering;
    f32 pitchInput;
    f32 accelerator;
    f32 rollInput;
    u8 unreconstructed1DE4[0x1DE8 - 0x1DE4];
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
    u8 unreconstructed2050[0x205C - 0x2050];
    /** The index of the car in g_aCarModels. */
    s32 carModel;
    /** The color of the paint of the car. */
    u8 color[3];
    /** VEHICLE_GHOST while the vehicle fades back in after it is put back on the track. */
    u8 ghostMode;
    /** How solid the vehicle is drawn, from 0 to 1. */
    f32 opacity;
    u8 unreconstructed2068[0x206C - 0x2068];
    /** The frames the vehicle has faced against the track. */
    s32 wrongWayFrames;
    /** The cell of the 32-unit grid of the track the vehicle is in. */
    u8 gridX;
    u8 gridY;
    u8 unreconstructed2072[0x2074 - 0x2072];
    PlayerInputState *input;
    VehicleModel *model;
    u8 unreconstructed207C[0x2090 - 0x207C];
} Vehicle;

/** @ghidraAddress USA: 0x8031b760 */
extern Vehicle g_aVehicles[MAX_VEHICLES];

/**
 * Find the nearest point of the route to a vehicle and add how far it went along the route to its
 * distance. A vehicle that has not crossed out of its point and stays near it keeps it.
 *
 * @return 0 if no point is near enough, 1 otherwise.
 * @ghidraAddress USA: 0x8021eb50
 */
s32 UpdateVehicleTrackPosition(Vehicle *vehicle);

/**
 * Run the clocks of a vehicle as the mode of play keeps them.
 *
 * @ghidraAddress USA: 0x8021f0a8
 */
void UpdateRaceTimers(Vehicle *vehicle);

/**
 * Count down the time of the message of a vehicle, or failing one its smaller message, and clear it
 * at the end.
 *
 * @ghidraAddress USA: 0x8021f158
 */
void UpdateHudMessageTimers(Vehicle *vehicle);

/**
 * Show "WRONG WAY", flashing, once a vehicle has faced against the track for a while.
 *
 * @ghidraAddress USA: 0x8021f1f0
 */
void CheckWrongWayWarning(Vehicle *vehicle);

/**
 * Find the cell of the grid a vehicle is in, then its place on the route.
 *
 * @ghidraAddress USA: 0x8021f2d8
 */
void UpdateVehicleGridPosition(Vehicle *vehicle);

/**
 * Run a frame of a vehicle: fly it, or put it on the ground, drive its wheels and step its
 * physics. Then place its wheels, set its speed, and update its cameras if a view follows it.
 *
 * @ghidraAddress USA: 0x8021f998
 */
void UpdateVehiclePhysicsFrame(Vehicle *vehicle);

/**
 * Put a vehicle at a position, with its body and the states of its steps.
 *
 * @ghidraAddress USA: 0x8021fe04
 */
void SetVehiclePosition(Vehicle *vehicle, f32 x, f32 y, f32 z);

/**
 * Turn a vehicle upright to face an angle about the vertical.
 *
 * @ghidraAddress USA: 0x8021fe80
 */
void InitVehicleOrientationFromAngle(Vehicle *vehicle, f32 angle);

/**
 * Set the frame of a vehicle and the orientation of its body from a matrix.
 *
 * @ghidraAddress USA: 0x80220150
 */
void SetVehicleFrameMatrix(Vehicle *vehicle, f32 m[4][4]);

/**
 * Set the velocity of a vehicle.
 *
 * @ghidraAddress USA: 0x802201c8
 */
void SetVehicleVelocity(Vehicle *vehicle, f32 x, f32 y, f32 z);

/**
 * Turn the body of a vehicle by angles about its vertical, sideways and forward axes, in turn.
 *
 * @ghidraAddress USA: 0x8022021c
 */
void RotateVehicleByEuler(Vehicle *vehicle, f32 yaw, f32 pitch, f32 roll);

/**
 * Set the angular velocity of a vehicle.
 *
 * @ghidraAddress USA: 0x80220358
 */
void SetVehicleTurnRate(Vehicle *vehicle, f32 x, f32 y, f32 z);

/**
 * Paint the model of a vehicle its color and set up its physics.
 *
 * @ghidraAddress USA: 0x80220398
 */
void InitVehicleColorAndPhysics(Vehicle *vehicle);

/**
 * Give a vehicle the model of a slot.
 *
 * @ghidraAddress USA: 0x802203f0
 */
void SetVehicleModelSlot(Vehicle *vehicle, s32 slot);

/**
 * Load the model of a car into a slot and give it to a vehicle.
 *
 * @ghidraAddress USA: 0x80220438
 */
void InstallVehicleModel(Vehicle *vehicle, s32 slot, s32 car);

/**
 * Start loading the model of a car into a slot through a buffer of g_aDmaWorkBuffers.
 *
 * @ghidraAddress USA: 0x80220474
 */
void LoadVehicleModel(VehicleModelLoad *load, s32 slot, s32 car, s32 buffer);

/**
 * Whether the load of a model is done.
 *
 * @ghidraAddress USA: 0x80220534
 */
s32 IsVehicleModelLoaded(VehicleModelLoad *load);

/**
 * Continue the load of a model, and once it is done, record the car of its slot and fix up its
 * pointers.
 *
 * @return The compressed bytes left to read.
 * @ghidraAddress USA: 0x80220544
 */
u32 PollVehicleModelDma(VehicleModelLoad *load);

/**
 * Reset the frames, cameras, velocity, place on the route and gearbox of a vehicle for its car.
 *
 * @ghidraAddress USA: 0x80220620
 */
void ResetVehicleState(Vehicle *vehicle);

/**
 * Give a vehicle the input of the player of the same index.
 *
 * @ghidraAddress USA: 0x802207a4
 */
void BindPlayerInputToVehicle(Vehicle *vehicle);

/**
 * Paint the materials of a model a color of 5 bits a channel.
 *
 * @ghidraAddress USA: 0x8021d140
 */
void SetMaterialColor(VehicleModel *model, s32 red, s32 green, s32 blue);

/**
 * Turn the offsets of a loaded model into pointers.
 *
 * @ghidraAddress USA: 0x8021d32c
 */
void FixupVehicleModelPointers(VehicleModel *model);

/**
 * Load the model of a car into a slot, or copy it from a slot that holds it.
 *
 * @ghidraAddress USA: 0x8021d5e4
 */
void LoadOrCopyVehicleModel(s32 slot, s32 car, s32 unused);

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
 * @ghidraAddress USA: 0x80221940
 */
void SimulateVehiclePhysicsSubstep(Vehicle *vehicle);

/**
 * Place the matrix of a vehicle, leant into its skid, and the matrices of its wheels, turned by
 * their roll and the front ones by their steering.
 *
 * @ghidraAddress USA: 0x80221e0c
 */
void TransformAllWheelMatrices(Vehicle *vehicle);

/**
 * Steer a vehicle by the stick, change its gears, and run its engine: its revolutions follow the
 * front wheels in gear, and its throttle the torque at them.
 *
 * @ghidraAddress USA: 0x80222050
 */
void UpdateVehicleEngineAndGearbox(Vehicle *vehicle);

/**
 * Set up the body of a vehicle and the bodies of its wheels, and their forces, for the model of its
 * car.
 *
 * @ghidraAddress USA: 0x80222d54
 */
void InitVehiclePhysics(Vehicle *vehicle);

/**
 * Bind a vehicle to the input of its player and reset it for a race.
 *
 * @ghidraAddress USA: 0x80226100
 */
void InitPlayerVehicleSlot(Vehicle *vehicle);

#endif
