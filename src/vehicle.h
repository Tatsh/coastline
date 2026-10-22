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

/* The size of the championship. */
#define SEASONS 6      /*!< The seasons of the championship. */
#define SEASON_RACES 4 /*!< The most races of a season. */

/** The laps of a race whose times a vehicle keeps. */
#define VEHICLE_LAP_TIMES 5

/** The tracks a profile keeps records for: the five tracks and their mirrored versions. */
#define PROFILE_RECORD_TRACKS 10

/** The wheels of a vehicle. */
#define VEHICLE_WHEELS 4

/** The sides around a car that dent apart, by the direction of the blow. */
#define VEHICLE_DENT_SIDES 8

/* The detail of the model of a car. */
#define VEHICLE_LODS 3           /*!< The levels of detail of the model, the nearest first. */
#define VEHICLE_DISPLAY_LISTS 10 /*!< The display lists of each level of detail. */

/** The display lists of a wheel of a car: the plain wheel, one not drawn, and one in the snow. */
enum {
    VEHICLE_WHEEL_DL_NORMAL,     /*!< The plain wheel. */
    VEHICLE_WHEEL_DL_1,          /*!< A wheel the renderer does not draw. */
    VEHICLE_WHEEL_DL_SNOW,       /*!< The wheel in the snow. */
    VEHICLE_WHEEL_DISPLAY_LISTS, /*!< The number of display lists of a wheel. */
};

/* The textures whose palettes give the colors of a car. */
#define VEHICLE_MATERIALS 12            /*!< The textures of the materials of a car. */
#define VEHICLE_MATERIAL_PAINT 2        /*!< The texture whose palette is the paint. */
#define VEHICLE_MATERIAL_HEADLIGHTS 10  /*!< The texture whose palette is the headlights. */
#define VEHICLE_MATERIAL_REAR_LIGHTS 11 /*!< The texture whose palette is the rear lights. */

/** The display lists of the model of a car that the renderer draws. */
enum {
    VEHICLE_DL_NEAR_DETAIL = 3, /*!< Details drawn only near the camera, at the nearest level. */
    VEHICLE_DL_BODY,            /*!< The paint, under the reflection of the sky. */
    VEHICLE_DL_HEADLIGHT_BEAMS, /*!< The beams of the headlights, at night or in the rain. */
    VEHICLE_DL_UNDERSIDE,       /*!< The underside, when the camera is below the car. */
    VEHICLE_DL_TEXTURED = 8,    /*!< Lit, textured parts, cut out by the alpha of their textures. */
    VEHICLE_DL_HIGHLIGHTED,     /*!< The parts that take the specular highlight. */
};

/**
 * The saved settings and progress of a player. Its fields join this structure as the routines that
 * use them are reconstructed.
 */
typedef struct {
    s32 championshipFlags; /*!< The progress of the championship across its years, negative when
                                never set, with bit 0 set while the seasons race the tracks
                                mirrored. */
    u8 season;             /*!< The season of the championship being raced, from 0. */
    u8 round;              /*!< The race of the season being raced, from 0. */
    u8 seasonPlaces[SEASONS][SEASON_RACES]; /*!< The place of each race of each season, from 0 for
                                                 the winner. */
    u16 seasonPoints[SEASONS];              /*!< The points scored in each season. */
    u8 unreconstructed2A[0x2C - 0x2A];      /*!< Padding before seasonTimes. */
    f32 seasonTimes[SEASONS][SEASON_RACES]; /*!< The time of each race of each season, in
                                                 seconds. */
    f32 lapRecords[PROFILE_RECORD_TRACKS];  /*!< The best lap on each track, in seconds, or 0 when
                                                 none was set. */
    u8 unreconstructedB4[0xCC - 0xB4];      /*!< Not read by the routines reconstructed so far. */
    u16 unlocks;          /*!< The unlocked cars, a bit for each from bit 0, and further unlocks
                               above them. */
    u16 unlockedTracks;   /*!< The unlocked tracks, a bit for each. */
    u16 unlockedWeathers; /*!< The weathers the championship has raced in, a bit for each. */
    u8 unreconstructedD2[0xD4 - 0xD2]; /*!< Padding before handling. */
    s32 handling;         /*!< The handling option the player chose last, the steering mode of
                               the vehicle from 0. */
    s32 transmission;     /*!< The gearbox option the player chose last, nonzero for the
                               automatic. */
    s32 tires;            /*!< The tyre option the player chose last; the grip falls by 0.002 for
                               each option up. */
    s32 suspension;       /*!< The suspension option the player chose last, from 0 for the
                               softest springs. */
    s32 controllerLayout; /*!< The button layout the player chose on the controller screen. */
    f32 raceRecords[PROFILE_RECORD_TRACKS]; /*!< The best race on each track, in seconds, or 0 when
                                                 none was set. */
    u8 unreconstructed110[0x128 - 0x110];   /*!< Not read by the routines reconstructed so far. */
} PlayerProfile;

/**
 * The profile of the player being played.
 *
 * @ghidraAddress USA: 0x8031c5bc
 * @ghidraAddress JPN: 0x8031d6ec
 * @ghidraAddress EUR: 0x8031eabc
 */
extern PlayerProfile *g_pActiveProfile;

/**
 * The profiles of the two players a controller pak saves.
 *
 * @ghidraAddress USA: 0x80324300
 * @ghidraAddress EUR: 0x80326840
 * @ghidraAddress JPN: 0x80325470
 */
extern PlayerProfile g_aSeasonSaveBuffers[2];

/**
 * The color of each player's vehicle, as red, green and blue.
 *
 * @ghidraAddress USA: 0x8028b904
 * @ghidraAddress JPN: 0x8028d5e4
 * @ghidraAddress EUR: 0x8028dbb4
 */
extern u8 g_abPlayerColorsRgb[][3];

/**
 * The model of a car as it is loaded from the cartridge. Its fields join this structure as they are
 * reconstructed.
 */
typedef struct {
    u32 triangleCount; /*!< The triangles of the model, counted into the statistics of the frame. */
    u8 unreconstructed4[0x10 - 0x4]; /*!< Not read by the routines reconstructed so far. */
    s32 textureCount;                /*!< The number of textures of the model. */
    ModelTexture *textures;          /*!< The textures of the model, textureCount of them. */
    Gfx *displayLists[VEHICLE_LODS][VEHICLE_DISPLAY_LISTS]; /*!< The display lists of each level of
                                                                 detail, by the VEHICLE_DL_
                                                                 values. */
    void *unreconstructed90; /*!< An offset turned into a pointer with the rest, not read by the
                                  routines reconstructed so far. */
    void *unreconstructed94; /*!< An offset turned into a pointer with the rest, not read by the
                                  routines reconstructed so far. */
    u8 unreconstructed98[0xB0 - 0x98]; /*!< Not read by the routines reconstructed so far. */
    f32 cockpitForward; /*!< How far forward of the vehicle the cockpit camera sits. */
    f32 mirrorForward;  /*!< Half of how far forward of the vehicle the mirror camera sits. */
    f32 cockpitUp;      /*!< How far above the vehicle the cockpit and mirror cameras sit. */
    Gfx *wheelDisplayLists[VEHICLE_LODS][VEHICLE_WHEEL_DISPLAY_LISTS]; /*!< The display lists of a
                                                                            wheel at each level of
                                                                            detail, by the
                                                                            VEHICLE_WHEEL_DL_
                                                                            values. */
    Vec3f wheels[VEHICLE_WHEELS]; /*!< Where the four wheels are drawn. The physics takes the
                                       second and the third as a front and a rear wheel of one side
                                       and mirrors them across, and the shadow is sized by the x
                                       and y of the first. */
    u8 materialTextures[VEHICLE_MATERIALS]; /*!< The textures whose palettes the colors of the car
                                                 are set in, by the VEHICLE_MATERIAL_ values. */
    u8 **decalMasks; /*!< The masks of the materials that the paint shop paints, one bit a texel
                          with a set bit protecting it, or NULL where every texel can be
                          painted. */
    u8 unreconstructed120[0xDF88 - 0x120]; /*!< The rest of the loaded model, not read as fields by
                                                the routines reconstructed so far. */
} VehicleModel;

/** How a car shows the revolutions of its engine. */
enum {
    TACHOMETER_NEEDLE,     /*!< A needle over the dashboard. */
    TACHOMETER_BAR_DIMMED, /*!< A bar of palette colors whose segments past the revolutions dim. */
    TACHOMETER_BAR_HIDDEN, /*!< A bar of palette colors whose segments past the revolutions
                                disappear. */
};

/** A car the players can choose. Its fields join this structure as they are reconstructed. */
typedef struct {
    const char *name;                /*!< The name of the car, in capitals. */
    u8 unreconstructed4[0x14 - 0x4]; /*!< Not read by the routines reconstructed so far. */
    u32 modelRom;     /*!< Where the compressed model of the car starts on the cartridge. */
    u32 modelRomEnd;  /*!< Where the asset after the model starts on the cartridge, or 0 for a car
                           without a model. */
    u32 modelSize;    /*!< The size of the model once decompressed, in bytes. */
    u32 dashboardRom; /*!< Where the dashboard starts on the cartridge (the palettes of day and
                           night, 256 colors each, the face, and a cell for each gear, reverse
                           first). */
    u8 unreconstructed24[0x28 - 0x24]; /*!< Not read by the routines reconstructed so far. */
    u8 dashboardWidth;                 /*!< The width of the face of the dashboard, in texels. */
    u8 dashboardHeight;                /*!< The height of the face of the dashboard, in texels. */
    s8 gearX;                          /*!< The x of the gear cell on the face, in pixels. */
    s8 gearY;                          /*!< The y of the gear cell on the face, in pixels. */
    u8 gearWidth;                      /*!< The width of the gear cell, in texels. */
    u8 gearHeight;                     /*!< The height of the gear cell, in texels. */
    u8 tachometer;                     /*!< The TACHOMETER_ kind. */
    union {
        u8 needleX;     /*!< The x on the face the needle turns about, in pixels. */
        u8 barSegments; /*!< The segments of a bar, the last colors of the palette. */
    }; /*!< The needle or the bar of the tachometer, by its kind. */
    u8 needleY;                        /*!< The y on the face the needle turns about, in pixels. */
    u8 unreconstructed31[0x34 - 0x31]; /*!< Padding before needleFullAngle. */
    f32 needleFullAngle; /*!< The angle of the needle at 8000 revolutions a minute, in radians. */
    f32 needleRestAngle; /*!< The angle of the needle at rest, in radians. */
    s32 driveType;       /*!< The wheels driven, as the index of their name. */
    s32 engineLayout;    /*!< The layout of the engine, as the index of its name. */
    f32 acceleration;    /*!< The acceleration the car selection screen shows, a full bar at 500. */
    f32 maxSpeed;        /*!< The top speed the car selection screen shows, a full bar at 10. */
    u8 unreconstructed4C[0x54 - 0x4C]; /*!< Not read by the routines reconstructed so far. */
    s32 gearCount;                     /*!< The gears of the gearbox. */
    f32 shadowLength;    /*!< The length of the shadow of the car on the stage of a menu. */
    f32 shadowHalfWidth; /*!< Half the width of the shadow of the car on the stage of a menu. */
} CarModel;

/** The cars the players can choose. */
#define CAR_MODELS 13

/**
 * @ghidraAddress USA: 0x8028ae0c
 * @ghidraAddress JPN: 0x8028caec
 * @ghidraAddress EUR: 0x8028d0bc
 */
extern CarModel g_aCarModels[CAR_MODELS];

/**
 * The number of cars in g_aCarModels.
 *
 * @ghidraAddress USA: 0x8028ae08
 * @ghidraAddress JPN: 0x8028cae8
 * @ghidraAddress EUR: 0x8028d0b8
 */
extern s32 g_dwCarCount;

/** The gears of a gearbox, neutral first. */
#define CAR_GEAR_RATIOS 7

/** The physics of a car. */
typedef struct {
    f32 gearRatios[CAR_GEAR_RATIOS]; /*!< The ratio of each gear, from 0 for neutral. */
    f32 torqueCurve[4]; /*!< The torque of the engine as a cubic of its revolutions, the highest
                             power first. */
    f32 finalDrive;     /*!< The ratio of the final drive. */
    s32 gearCount;      /*!< The number of forward gears. */
    s32 rearWheelDrive; /*!< Whether the throttle drives the rear wheels alone, rather than all
                             four. */
    Vec3f boxSize;      /*!< The size of the box the track collides with. */
    f32 boxCenterZ;     /*!< The height of the middle of the box. */
    s32 tableIndex;     /*!< The class of the car among the arcade times of each track. */
} CarPhysicsSpec;

/**
 * @ghidraAddress USA: 0x8028b330
 * @ghidraAddress JPN: 0x8028d010
 * @ghidraAddress EUR: 0x8028d5e0
 */
extern CarPhysicsSpec g_aCarPhysicsSpecs[CAR_MODELS];

/**
 * The models of the cars the vehicles drive, in slots. The link script places them.
 *
 * @ghidraAddress USA: 0x803c8000
 * @ghidraAddress JPN: 0x803c8000
 * @ghidraAddress EUR: 0x803c8000
 */
extern VehicleModel g_aVehicleModels[];

/**
 * The car whose model each slot holds.
 *
 * @ghidraAddress USA: 0x8031b238
 * @ghidraAddress EUR: 0x8031d738
 */
extern s32 g_adwVehicleModelSlotCars[];

/** The decompression of the model of a car into a slot. */
typedef struct {
    DecompressJob job; /*!< The decompression from the cartridge. */
    s32 slot;          /*!< The slot of g_aVehicleModels the model loads into. */
    s32 car;           /*!< The index of the car in g_aCarModels. */
} VehicleModelLoad;

/** The slots the models of the cars of the menus load into. */
#define VEHICLE_MODEL_SLOTS 4

/** The cameras of a vehicle. */
enum {
    VEHICLE_CAMERA_COCKPIT,   /*!< The view from the driver's seat. */
    VEHICLE_CAMERA_CHASE,     /*!< The camera that follows behind the vehicle. */
    VEHICLE_CAMERA_BUMPER,    /*!< The cockpit camera moved forward to the bumper. */
    VEHICLE_CAMERA_TRACKSIDE, /*!< A camera by the track that watches the vehicle and zooms with its
                                   distance. */
    VEHICLE_CAMERA_4,         /*!< A camera the routines reconstructed so far do not use. */
    VEHICLE_CAMERA_MIRROR,    /*!< The rear-view mirror. */
    VEHICLE_CAMERAS,          /*!< The number of cameras of a vehicle. */
};

/** The racers of a race. */
#define RACER_SLOTS 20

/**
 * A racer of a race. A racer with a vehicle copies the progress of the vehicle; one without moves
 * along the route by itself, and keeps the progress a vehicle takes up when it gets one.
 */
typedef struct {
    Vec3f position;                    /*!< The position of the racer. */
    Vec3f previousPosition;            /*!< The position of the last frame. */
    Vec3f velocity;                    /*!< How far the racer moves a second. */
    f32 speedKph;                      /*!< The speed of the racer, in kilometres an hour. */
    TrackSection *section;             /*!< The section of the route the racer is on. */
    s32 pointIndex;                    /*!< The point of the racer in its section. */
    f32 lapTime;                       /*!< The time of the lap being driven, in seconds. */
    f32 bestLapTime;                   /*!< The time of the best lap, in seconds. */
    f32 totalTime;                     /*!< The time of the race so far, in seconds. */
    f32 timeLeft;                      /*!< The time remaining, in seconds. */
    s32 lap;                           /*!< The laps finished. */
    s32 distanceLaps;                  /*!< The laps the distance travelled has counted. */
    s32 maxGate;                       /*!< The farthest gate passed, counted across laps. */
    s32 gate;                          /*!< The gate last crossed, counted across laps. */
    f32 distance;                      /*!< The distance travelled along the route. */
    s32 racePosition;                  /*!< The place in the race, from 0 for the leader. */
    u8 unreconstructed58[0x5C - 0x58]; /*!< Not read by the routines reconstructed so far. */
    u8 color[3];                       /*!< The color of the vehicle the racer drives. */
    struct Vehicle *vehicle;           /*!< The vehicle of the racer, or NULL. */
    s32 index;                         /*!< The index of the racer in g_aRacerSlots. */
    u32 status; /*!< The RACER_ bits. The clock of the racer runs while the bits of
                     RACER_CLOCK_STOPPED are clear. */
    u8 unreconstructed6C[0x74 - 0x6C]; /*!< Not read by the routines reconstructed so far. */
    s32 straggler; /*!< Set for a racer past the thirteenth after the players. The championship
                        slows the vehicle of a straggler more, and the racers of each kind share
                        separate vehicles. */
} RacerSlot;

/**
 * The racers of the race.
 *
 * @ghidraAddress USA: 0x803239a0
 * @ghidraAddress JPN: 0x80324ad0
 * @ghidraAddress EUR: 0x80325ea0
 */
extern RacerSlot g_aRacerSlots[];

/**
 * The racers in the race.
 *
 * @ghidraAddress USA: 0x8028b7f0
 * @ghidraAddress JPN: 0x8028d4d0
 * @ghidraAddress EUR: 0x8028daa0
 */
extern s32 g_dwNumVehicles;

/**
 * The vehicles of g_aVehicles the race uses.
 *
 * @ghidraAddress USA: 0x8028b7f4
 * @ghidraAddress JPN: 0x8028d4d4
 * @ghidraAddress EUR: 0x8028daa4
 */
extern s32 g_dwNumActiveVehicles;

/** The bits of the status of a racer that stop its clock. */
#define RACER_CLOCK_STOPPED 0x3

/** The bit of the status of a racer whose vehicle is held out of gear. */
#define RACER_HELD 0x1

/** The bit of the status of a racer that finished the race. */
#define RACER_FINISHED 0x2

/** The ghost mode of a vehicle that other vehicles pass through and that fades back in. */
#define VEHICLE_GHOST 2

/** The axes and the position of a wheel, the rows of its matrix. */
typedef struct {
    Vec3f forward;  /*!< The forward axis of the wheel. */
    f32 forwardW;   /*!< 0, the last column of the matrix. */
    Vec3f side;     /*!< The sideways axis of the wheel. */
    f32 sideW;      /*!< 0, the last column of the matrix. */
    Vec3f up;       /*!< The upward axis of the wheel. */
    f32 upW;        /*!< 0, the last column of the matrix. */
    Vec3f position; /*!< The position of the wheel. */
    f32 positionW;  /*!< 1, the last column of the matrix. */
} WheelFrame;

/** The segments of the spray a wheel throws. */
#define SPRAY_SEGMENTS 9
/** The vertices of each segment of the spray, of which the first two are drawn. */
#define SPRAY_SEGMENT_VERTICES 4

/** The forces on the bodies of a vehicle. */
enum {
    VEHICLE_FORCE_SPRING_0, /*!< The spring of the rear wheel of the first side, first in the list
                                 of the body for its first pass. */
    VEHICLE_FORCE_SPRING_1, /*!< The spring of the front wheel of the first side. */
    VEHICLE_FORCE_SPRING_2, /*!< The spring of the rear wheel of the second side. */
    VEHICLE_FORCE_SPRING_3, /*!< The spring of the front wheel of the second side. */
    VEHICLE_FORCE_DAMPER_0, /*!< The damper of the rear wheel of the first side, first in the list
                                 of the body for its second pass. */
    VEHICLE_FORCE_DAMPER_1, /*!< The damper of the front wheel of the first side. */
    VEHICLE_FORCE_DAMPER_2, /*!< The damper of the rear wheel of the second side. */
    VEHICLE_FORCE_DAMPER_3, /*!< The damper of the front wheel of the second side. */
    VEHICLE_FORCE_GRAVITY,  /*!< The gravity of the body, last in the list of its first pass. */
    VEHICLE_FORCE_FRONT_WHEEL_GRAVITY, /*!< The gravity of the front wheels. */
    VEHICLE_FORCE_REAR_WHEEL_GRAVITY,  /*!< The gravity of the rear wheels. */
    VEHICLE_FORCE_DRAG,    /*!< The drag of the body, last in the list of its second pass. */
    VEHICLE_FORCE_WHEEL_0, /*!< The drive of wheelBodies[0], before the gravity of its wheel. */
    VEHICLE_FORCE_WHEEL_1, /*!< The drive of wheelBodies[1], before the gravity of its wheel. */
    VEHICLE_FORCE_WHEEL_2, /*!< The drive of wheelBodies[2], before the gravity of its wheel. */
    VEHICLE_FORCE_WHEEL_3, /*!< The drive of wheelBodies[3], before the gravity of its wheel. */
    VEHICLE_FORCES,        /*!< The number of forces of a vehicle. */
};

/**
 * A vehicle. The game prints its size, 0x2090 bytes, at start-up. Its fields join this structure as
 * the routines that use them are reconstructed.
 */
typedef struct Vehicle {
    union {
        CameraFrame frame; /*!< The axes and position of the vehicle. */
        struct {
            f32 matrix[4][4]; /*!< The frame as a matrix. */
            union {
                f32 wheelMatrices[VEHICLE_WHEELS][4][4]; /*!< The matrix of each wheel. */
                WheelFrame wheelFrames[VEHICLE_WHEELS];  /*!< The axes and position of each
                                                              wheel. */
            }; /*!< The frame of each wheel, as a matrix or as its axes. */
        }; /*!< The frame of the vehicle and of its wheels, as matrices. */
    }; /*!< The frame of the vehicle, as axes or as matrices. */
    s32 index;      /*!< The index of the vehicle in g_aVehicles. The players have the first. */
    s16 skidFrames; /*!< The frames the tyres have skidded, up to 16, swelling the sound of the
                         skid. */
    RigidBody body; /*!< The rigid body of the vehicle. */
    RigidBody wheelBodies[VEHICLE_WHEELS]; /*!< The bodies of the wheels, the rear and the front
                                                wheel of one side, then of the other. The body
                                                lists them as the rear two, then the front two. */
    RigidBodyForce forces[VEHICLE_FORCES]; /*!< The forces on the bodies, by the VEHICLE_FORCE_
                                                values. */
    u8 unreconstructedD70[0xDF0 - 0xD70];  /*!< Not read by the routines reconstructed so far. */
    f32 steerAngle;                  /*!< The angle the front wheels are steered to, in radians. */
    f32 engineRpm;                   /*!< The revolutions of the engine a minute. */
    f32 gearRatios[CAR_GEAR_RATIOS]; /*!< The ratio of each gear, from the physics spec of the
                                          car. */
    f32 torqueCurve[4];              /*!< The torque curve, from the physics spec of the car. */
    f32 finalDrive;          /*!< The ratio of the final drive, from the physics spec of the car. */
    s32 gearCount;           /*!< The number of forward gears, from the physics spec of the car. */
    s32 rearWheelDrive;      /*!< Whether the rear wheels alone are driven, from the physics spec
                                  of the car. */
    s32 transmission;        /*!< The gearbox the player chose, nonzero for the automatic. */
    s32 tableIndex;          /*!< The class of the car among the arcade times of each track. */
    f32 throttle;            /*!< The throttle, below 0 in reverse. */
    f32 brake;               /*!< The brake, below 0 when braking. */
    s32 gear;                /*!< The gear, from 0 for neutral. */
    f32 frontLoad;           /*!< The load the front tyres bore in the last step. */
    u8 frontSliding;         /*!< Whether the front tyres slid in the last step; sliding tyres grip
                                  less. */
    f32 rearLoad;            /*!< The load the rear tyres bore in the last step. */
    u8 rearSliding;          /*!< Whether the rear tyres slid in the last step; sliding tyres grip
                                  less. */
    s8 steerReturn;          /*!< The way the steering returns towards the middle, 1 or -1, or 0
                                  while it does not return. */
    s32 skipDrive;           /*!< Whether the next step does not drive the wheels. */
    s32 customDecals;        /*!< Whether the player chose the custom decals of the car. */
    PlayerProfile *profile;  /*!< The profile of the player who drives the vehicle. */
    s32 tireType;            /*!< The tyres; the grip falls by 0.002 for each type up. */
    s32 suspensionStiffness; /*!< The stiffness of the springs, from 0 for the softest. */
    s32 steeringMode;        /*!< How the steering responds to the stick, from 0. */
    s32 chosenTransmission;  /*!< The gearbox of the recording or the settings, taken by
                                  ResetVehicleState(). */
    s32 forcedForwardFrames; /*!< The remaining frames a computer's vehicle drives forward to get
                                  unstuck, whatever else it wants. */
    s32 forcedReverseFrames; /*!< The remaining frames a computer's vehicle reverses to get
                                  unstuck, whatever else it wants. */
    s32 reverseFrames;       /*!< The frames a computer's vehicle has reversed since it last
                                  checked whether it is stuck. */
    s32 forwardFrames;       /*!< The frames a computer's vehicle has driven forward since it last
                                  checked whether it is stuck. */
    RacerSlot *nearestRacers[RACER_SLOTS]; /*!< The racers nearest a player's vehicle, the nearest
                                                first, as the vehicles were shared out. */
    RacerSlot *racer;                      /*!< The racer the vehicle drives for. */
    s32 slotCooldown; /*!< The frames before a computer's vehicle that departed its racer may be
                           taken from another. */
    void (*driver)(struct Vehicle *vehicle); /*!< The routine that drives the vehicle each frame,
                                                  the player's or the computer's, or NULL. */
    Vec3f aiTarget;                          /*!< The point a computer's vehicle steers towards. */
    Vec3f trackPoint;                        /*!< The point of the route nearest the vehicle. */
    Vec3f trackForward;                      /*!< The axis of the route at trackPoint along it. */
    Vec3f trackSide;       /*!< The axis of the route at trackPoint across it to the left. */
    Vec3f trackUp;         /*!< The axis of the route at trackPoint upwards. */
    f32 trackSideDistance; /*!< How far the vehicle is from the middle of the route,
                                trackSideOffset without its sign. */
    Vec3f trackOffset;     /*!< The offset of the vehicle from trackPoint. */
    f32 trackSideOffset;   /*!< How far the vehicle is from the middle of the route along
                                trackSide, below 0 to the right. */
    Vec3f previousListenerPosition; /*!< Where the camera that heard the vehicle was the last
                                         frame. */
    s32 effectSound;    /*!< The sound effect the vehicle plays, from 0 for none. A louder or more
                             important sound takes its place. */
    s32 effectVolume;   /*!< How loud the sound effect plays. */
    s32 effectPriority; /*!< How much the sound effect matters. */
    f32 dopplerFactor;  /*!< How much the motion of the vehicle and the camera raise its sounds. */
    s32 cameraChanged;  /*!< Set when the active camera changes, to 1 when a view is chosen and to
                             2 by the cameras. */
    s32 flyMode;        /*!< Whether the vehicle flies freely with the controls, without physics,
                             with the cameras fixed rigidly to its frame. */
    Vec3f previousPosition;          /*!< The position of the last frame. */
    TrackSection *section;           /*!< The section of the route the vehicle is on. */
    s32 pointIndex;                  /*!< The nearest point of the vehicle in its section. */
    Vec3f trackDirection;            /*!< The direction of the track at pointIndex. */
    s32 maxGate;                     /*!< The farthest gate passed, counted across laps. */
    s32 gate;                        /*!< The gate last crossed, counted across laps. */
    s32 lap;                         /*!< The laps finished. */
    s32 distanceLaps;                /*!< The laps the distance travelled has counted. */
    f32 lapTime;                     /*!< The time of the lap being driven, in seconds. */
    f32 lapTimes[VEHICLE_LAP_TIMES]; /*!< The time of each lap finished, in seconds. */
    f32 bestLapTime;                 /*!< The time of the best lap, in seconds. */
    s32 bestLap;                     /*!< The best lap, from 0. */
    f32 totalTime;                   /*!< The time of the race so far, in seconds. */
    f32 timeLeft;                    /*!< The time remaining, in seconds. */
    f32 distance;                    /*!< The distance travelled along the route. */
    s32 racePosition;                /*!< The place in the race, from 0 for the leader. */
    const char *message;    /*!< The message shown large in the middle of the view, or NULL. */
    f32 messageTime;        /*!< The remaining time of message, in seconds. */
    const char *subMessage; /*!< The message shown smaller when there is no large one, or NULL. */
    f32 subMessageTime;     /*!< The remaining time of subMessage, in seconds. */
    char messageText[0xFD4 - 0xFC0]; /*!< The text a message may show, such as the time of a
                                          record. */
    s32 groundHits;        /*!< The triangles of the track below the vehicle this frame. */
    Vec3f placedVelocity;  /*!< The velocity SetVehicleVelocity() last gave. */
    f32 speedKph;          /*!< The speed the display shows, in kilometres an hour. */
    f32 odometer;          /*!< The distance the speedometer has counted. */
    Vec3f joltPosition;    /*!< Where a weight on a spring at a corner of the vehicle is. Its jolts
                                damage the vehicle. */
    Vec3f joltVelocity;    /*!< How far the jolt weight moves a frame. */
    Vec3f joltOffset;      /*!< How far the jolt weight hangs from the corner. */
    f32 exhaustTimer;      /*!< The time towards the next puff of exhaust. */
    Vec3f exhaustPosition; /*!< Where the last puff departed the exhaust. */
    f32 smokeTimers[VEHICLE_WHEELS]; /*!< The time towards the next puff of smoke of each wheel, the
                                          front two first. */
    Vec3f smokePositions[VEHICLE_WHEELS]; /*!< Where the last puff of smoke of each wheel was, the
                                               front two first. */
    f32 sprayFramesLeft[VEHICLE_WHEELS];  /*!< The frames the spray of each wheel goes on after the
                                               wheel lifts off the ground. */
    s32 spraySurfaces[VEHICLE_WHEELS];    /*!< The surface the spray of each wheel is thrown
                                               from. */
    f32 sprayPhases[VEHICLE_WHEELS];      /*!< The time towards the next segment of each spray. */
    s32 sprayAdvanced[VEHICLE_WHEELS];    /*!< Whether a segment of each spray began this frame. */
    Vec3f sprayPositions[VEHICLE_WHEELS]; /*!< Where the newest segment of each spray was thrown
                                               from. */
    Vtx sprayVertices[VEHICLE_WHEELS][SPRAY_SEGMENTS]
                     [SPRAY_SEGMENT_VERTICES]; /*!< The vertices of the segments of each spray, the
                                                    newest first, in 127ths of a unit from
                                                    sprayOrigin. */
    s16 sprayVelocities[VEHICLE_WHEELS][SPRAY_SEGMENTS][SPRAY_SEGMENT_VERTICES]
                       [3]; /*!< How far the vertices of each spray move a second, in 127ths of a
                                 unit. */
    s16 spraySegmentSurfaces[VEHICLE_WHEELS][SPRAY_SEGMENTS]; /*!< The surface each segment was
                                                                   thrown from, or -1 when it was
                                                                   thrown from none. */
    Vec3f sprayOrigin; /*!< The point the spray vertices are relative to, put at the vehicle by
                            SetVehiclePosition(). */
    u8 unreconstructed1D84[0x1D88 - 0x1D84]; /*!< Padding before the 8-byte alignment of
                                                  sprayMtx. */
    Mtx sprayMtx;      /*!< The matrix of the spray, the identity until it is set. */
    s32 inTunnel;      /*!< Whether the placed model under the vehicle is a tunnel, darkening the
                            vehicle. */
    f32 cameraHeading; /*!< The heading of the camera of the vehicle, in radians, turning the
                            reflection of the sky. */
    u8 unreconstructed1DD0[0x1DD4 - 0x1DD0]; /*!< Not read by the routines reconstructed so far. */
    f32 steering;                            /*!< The steering control. */
    f32 pitchInput;                          /*!< The pitch control of the fly mode. */
    f32 accelerator;                         /*!< The accelerator control. */
    f32 rollInput;                           /*!< The roll control of the fly mode, 1, -1, or 0. */
    f32 fogFactor; /*!< How thick the fog is at the vehicle, from 0 to 1, fading its shadow. */
    CameraFrame *activeCamera;   /*!< The member of cameras the view of the vehicle is drawn
                                      from. */
    CameraFrame *previousCamera; /*!< The camera of the last frame. */
    CameraFrame cameras[VEHICLE_CAMERAS]; /*!< The cameras, by the VEHICLE_CAMERA_ values. */
    f32 chaseSpeed;                       /*!< The speed the chase camera follows, smoothed. */
    f32 chaseLag;                         /*!< The lag of the chase camera, smoothed. */
    f32 lookAhead;                        /*!< How far the look-at point leads the vehicle. */
    Vec3f lookAt;                         /*!< The point the chase camera looks at. */
    Vec3f chasePosition; /*!< Where the chase camera was before the track pushed it. */
    f32 collisionPull;   /*!< How hard the track has been pushing the chase camera. */
    u8 unreconstructed1FB0[0x1FB4 - 0x1FB0]; /*!< Not read by the routines reconstructed so far. */
    Vec3f initialChasePosition;              /*!< Where the chase camera started. */
    u16 groundInstances[RAY_INSTANCES]; /*!< The placed models of the ground below the vehicle, the
                                             nearest first, from CastRayAgainstTrack(). */
    s32 groundInstanceCount; /*!< The number of groundInstances to read, 1 once the ground is
                                  found. */
    u16 nearbyInstances[RAY_INSTANCES]; /*!< The placed models near the ground, the latest
                                             first. */
    s32 nearbyInstanceCount;            /*!< The number of nearbyInstances. */
    f32 groundDistance;                 /*!< How far the ground is below the vehicle. */
    s32 groundSurface;                  /*!< The index of the triangle of the ground. */
    s16 shadowCorners[2][2]; /*!< The corners of the rectangle of the screen the shadow of the
                                  vehicle covers in the view being drawn, the left and the lower
                                  edge, then the right and the upper edge. */
    s32 selectedCar;         /*!< The car under the cursor of the car selection screen. */
    s32 carModel;            /*!< The index of the car in g_aCarModels. */
    u8 color[3];             /*!< The color of the paint of the car, as red, green, and blue. */
    u8 ghostMode;  /*!< VEHICLE_GHOST while the vehicle fades back in after it is put back on the
                        track. */
    f32 opacity;   /*!< How solid the vehicle is drawn, from 0 to 1. */
    s32 otherLook; /*!< Whether the car is drawn in its other look, without its textured parts or
                        the reflection of the sky. A button switches it once an unlock allows
                        it. */
    s32 wrongWayFrames; /*!< The frames the vehicle has faced against the track. */
    u8 gridX;           /*!< The x of the cell of the 32-unit grid of the track the vehicle is
                             in. */
    u8 gridY;           /*!< The y of the cell of the 32-unit grid of the track the vehicle is
                             in. */
    u8 unreconstructed2072[0x2074 - 0x2072]; /*!< Padding before input. */
    PlayerInputState *input;                 /*!< The input that drives the vehicle. */
    VehicleModel *model;           /*!< The model of the car, in a slot of g_aVehicleModels. */
    s16 dents[VEHICLE_DENT_SIDES]; /*!< How deeply each of eight sides around the car has been
                                        dented; up to 0x100 it dents more. */
    s16 dentJitter; /*!< A small offset, from -4 to 3, that changes with each dent to vary the
                         dents. */
    u8 unreconstructed208E[0x2090 - 0x208E]; /*!< Padding to the end of the vehicle. */
} Vehicle;

/**
 * @ghidraAddress USA: 0x8031b760
 * @ghidraAddress JPN: 0x8031c890
 * @ghidraAddress EUR: 0x8031dc60
 */
extern Vehicle g_aVehicles[MAX_VEHICLES];

/**
 * Find the nearest point of the route to a vehicle and add how far it went along the route to its
 * distance. A vehicle that has not crossed out of its point and stays near it keeps it.
 *
 * @param vehicle The vehicle.
 * @return 0 if no point is near enough, 1 otherwise.
 * @ghidraAddress USA: 0x8021eb50
 * @ghidraAddress JPN: 0x8021f640
 * @ghidraAddress EUR: 0x8021fe50
 */
s32 UpdateVehicleTrackPosition(Vehicle *vehicle);

/**
 * Run the clocks of a vehicle as the mode of play keeps them.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x8021f0a8
 * @ghidraAddress JPN: 0x8021fb98
 * @ghidraAddress EUR: 0x802203a8
 */
void UpdateRaceTimers(Vehicle *vehicle);

/**
 * Count down the time of the message of a vehicle, or failing one its smaller message, and clear it
 * at the end.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x8021f158
 * @ghidraAddress JPN: 0x8021fc48
 * @ghidraAddress EUR: 0x80220458
 */
void UpdateHudMessageTimers(Vehicle *vehicle);

/**
 * Show "WRONG WAY", flashing, once a vehicle has faced against the track for a while.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x8021f1f0
 * @ghidraAddress JPN: 0x8021fce0
 * @ghidraAddress EUR: 0x802204f0
 */
void CheckWrongWayWarning(Vehicle *vehicle);

/**
 * Find the cell of the grid a vehicle is in, then its place on the route.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x8021f2d8
 * @ghidraAddress JPN: 0x8021fdc8
 * @ghidraAddress EUR: 0x802205d8
 */
void UpdateVehicleGridPosition(Vehicle *vehicle);

/**
 * Run a frame of a vehicle: fly it, or put it on the ground, drive its wheels and step its
 * physics. Then place its wheels, set its speed, and update its cameras if a view follows it.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x8021f998
 * @ghidraAddress JPN: 0x80220488
 * @ghidraAddress EUR: 0x80220c98
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
 * @ghidraAddress JPN: 0x802208f4
 * @ghidraAddress EUR: 0x80221104
 */
void SetVehiclePosition(Vehicle *vehicle, f32 x, f32 y, f32 z);

/**
 * Turn a vehicle upright to face an angle about the vertical.
 *
 * @param vehicle The vehicle.
 * @param angle The angle from the x axis, in radians.
 * @ghidraAddress USA: 0x8021fe80
 * @ghidraAddress JPN: 0x80220970
 * @ghidraAddress EUR: 0x80221180
 */
void InitVehicleOrientationFromAngle(Vehicle *vehicle, f32 angle);

/**
 * Set the frame of a vehicle and the orientation of its body from a matrix.
 *
 * @param vehicle The vehicle.
 * @param m The matrix, whose rows are the axes and the position.
 * @ghidraAddress USA: 0x80220150
 * @ghidraAddress JPN: 0x80220c40
 * @ghidraAddress EUR: 0x80221450
 */
void SetVehicleFrameMatrix(Vehicle *vehicle, f32 (*m)[4]);

/**
 * Set the velocity of a vehicle.
 *
 * @param vehicle The vehicle.
 * @param x The x of the velocity.
 * @param y The y of the velocity.
 * @param z The z of the velocity.
 * @ghidraAddress USA: 0x802201c8
 * @ghidraAddress JPN: 0x80220cb8
 * @ghidraAddress EUR: 0x802214c8
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
 * @ghidraAddress JPN: 0x80220d0c
 * @ghidraAddress EUR: 0x8022151c
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
 * @ghidraAddress JPN: 0x80220e48
 * @ghidraAddress EUR: 0x80221658
 */
void SetVehicleTurnRate(Vehicle *vehicle, f32 x, f32 y, f32 z);

/**
 * Paint the model of a vehicle its colour and set up its physics. The Japanese release only sets
 * up the physics.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x80220398
 * @ghidraAddress JPN: 0x80220e88
 * @ghidraAddress EUR: 0x80221698
 */
void InitVehicleColorAndPhysics(Vehicle *vehicle);

/**
 * Give a vehicle the model of a slot.
 *
 * @param vehicle The vehicle.
 * @param slot The slot of g_aVehicleModels.
 * @ghidraAddress USA: 0x802203f0
 * @ghidraAddress JPN: 0x80220ea8
 * @ghidraAddress EUR: 0x802216f0
 */
void SetVehicleModelSlot(Vehicle *vehicle, s32 slot);

/**
 * Load the model of a car into a slot and give it to a vehicle. The Japanese release then takes
 * the colour of the vehicle from the model.
 *
 * @param vehicle The vehicle.
 * @param slot The slot of g_aVehicleModels.
 * @param car The index of the car in g_aCarModels.
 * @ghidraAddress USA: 0x80220438
 * @ghidraAddress JPN: 0x80220ef0
 * @ghidraAddress EUR: 0x80221738
 */
void InstallVehicleModel(Vehicle *vehicle, s32 slot, s32 car);

#if TGR_REGION == TGR_REGION_EU
/**
 * Load the model of a car into a slot and give it to a vehicle, as InstallVehicleModel() does.
 *
 * @param vehicle The vehicle.
 * @param slot The slot of g_aVehicleModels.
 * @param car The index of the car in g_aCarModels.
 * @ghidraAddress EUR: 0x80221774
 */
void InstallVehicleModelCopy(Vehicle *vehicle, s32 slot, s32 car);
#endif

#if TGR_REGION == TGR_REGION_JP
/**
 * Load the model of a car into a slot, give it to a vehicle, and paint it the color of the
 * vehicle.
 *
 * @param vehicle The vehicle.
 * @param slot The slot of g_aVehicleModels.
 * @param car The index of the car in g_aCarModels.
 * @ghidraAddress JPN: 0x80220f38
 */
void LoadAndActivateTrackWithPaintColor(Vehicle *vehicle, s32 slot, s32 car);
#endif

/**
 * Start loading the model of a car into a slot through a buffer of g_aDmaWorkBuffers.
 *
 * @param load The load, which this starts.
 * @param slot The slot of g_aVehicleModels.
 * @param car The index of the car in g_aCarModels.
 * @param buffer The index of the buffer.
 * @ghidraAddress USA: 0x80220474
 * @ghidraAddress JPN: 0x80220fa8
 * @ghidraAddress EUR: 0x802217b0
 */
void LoadVehicleModel(VehicleModelLoad *load, s32 slot, s32 car, s32 buffer);

/**
 * Whether the load of a model is done.
 *
 * @param load The load.
 * @return Whether no compressed bytes are left to read.
 * @ghidraAddress USA: 0x80220534
 * @ghidraAddress JPN: 0x80221068
 * @ghidraAddress EUR: 0x80221870
 */
s32 IsVehicleModelLoaded(VehicleModelLoad *load);

/**
 * Continue the load of a model, and once it is done, record the car of its slot and fix up its
 * pointers.
 *
 * @param load The load.
 * @return The compressed bytes left to read.
 * @ghidraAddress USA: 0x80220544
 * @ghidraAddress JPN: 0x80221078
 * @ghidraAddress EUR: 0x80221880
 */
u32 PollVehicleModelDma(VehicleModelLoad *load);

/**
 * Reset the frames, cameras, velocity, place on the route and gearbox of a vehicle for its car.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x80220620
 * @ghidraAddress JPN: 0x80221154
 * @ghidraAddress EUR: 0x8022195c
 */
void ResetVehicleState(Vehicle *vehicle);

/**
 * Give a vehicle the input of the player of the same index.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x802207a4
 * @ghidraAddress JPN: 0x802212d8
 * @ghidraAddress EUR: 0x80221ae0
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
 * @ghidraAddress JPN: 0x8021dc10
 * @ghidraAddress EUR: 0x8021e340
 */
void SetMaterialColor(VehicleModel *model, s32 red, s32 green, s32 blue);

/**
 * Set the colour of the paint of a vehicle from the first colour of the palette of the paint
 * texture of a model, when that texture has a palette and is of the format 1.
 *
 * @param vehicle The vehicle.
 * @param model The model.
 * @ghidraAddress USA: 0x8021d2a0
 * @ghidraAddress JPN: 0x8021dd70
 * @ghidraAddress EUR: 0x8021e4a0
 */
void ExtractVehicleBaseColor(Vehicle *vehicle, VehicleModel *model);

/**
 * Whether the model of a car is on the cartridge.
 *
 * @param car The index of the car in g_aCarModels.
 * @return Whether its model has an offset on the cartridge.
 * @ghidraAddress USA: 0x8021d6b8
 * @ghidraAddress JPN: 0x8021e188
 * @ghidraAddress EUR: 0x8021e8b8
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
 * @ghidraAddress JPN: 0x80226ab8
 * @ghidraAddress EUR: 0x802272c8
 */
void ChooseVehicleCar(Vehicle *vehicle);

/**
 * The driver of a computer's vehicle, which runs its behaviour for the frame.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x802288b4
 * @ghidraAddress JPN: 0x80229408
 * @ghidraAddress EUR: 0x80229c2c
 */
void DriveAiVehicle(Vehicle *vehicle);

/**
 * Turn the offsets of a loaded model into pointers.
 *
 * @param model The model.
 * @ghidraAddress USA: 0x8021d32c
 * @ghidraAddress JPN: 0x8021ddfc
 * @ghidraAddress EUR: 0x8021e52c
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
 * @ghidraAddress JPN: 0x8021e0b4
 * @ghidraAddress EUR: 0x8021e7e4
 */
void LoadOrCopyVehicleModel(s32 slot, s32 car, void *source);

/**
 * Decompress the model of a car from the cartridge, and keep its size.
 *
 * @param model Where to decompress it.
 * @param car The index of the car in g_aCarModels.
 * @ghidraAddress USA: 0x8021d534
 * @ghidraAddress JPN: 0x8021e004
 * @ghidraAddress EUR: 0x8021e734
 */
void LoadVehicleModelFromRom(VehicleModel *model, s32 car);

/**
 * How the steering answers the stick, as the steering mode of a vehicle sets it: the lock at rest
 * and how much of it the speed takes, in degrees, how fast the wheels turn a frame, in radians,
 * and the angle each unit of the stick steers, in degrees.
 *
 * @ghidraAddress USA: 0x8028b720
 * @ghidraAddress JPN: 0x8028d400
 * @ghidraAddress EUR: 0x8028d9d0
 */
extern f32 g_fSteerLock;

/**
 * @ghidraAddress USA: 0x8028b724
 * @ghidraAddress JPN: 0x8028d404
 * @ghidraAddress EUR: 0x8028d9d4
 */
extern f32 g_fSteerRate;

/**
 * @ghidraAddress USA: 0x8028b728
 * @ghidraAddress JPN: 0x8028d408
 * @ghidraAddress EUR: 0x8028d9d8
 */
extern f32 g_fSteerGain;

/**
 * @ghidraAddress USA: 0x8028b72c
 * @ghidraAddress JPN: 0x8028d40c
 * @ghidraAddress EUR: 0x8028d9dc
 */
extern f32 g_fSteerLockLoss;

/**
 * How much the speed halves the gain, which the modes leave at 0.
 *
 * @ghidraAddress USA: 0x8028b730
 * @ghidraAddress JPN: 0x8028d410
 * @ghidraAddress EUR: 0x8028d9e0
 */
extern s16 g_wSteerGainSpeedShare;

/**
 * Step the physics of a vehicle on the ground for a frame: drive its wheels, push it by its
 * springs, grip and dampers, collide it with the track and the other vehicles, and hang its
 * wheels to the ground.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x80221940
 * @ghidraAddress JPN: 0x80222470
 * @ghidraAddress EUR: 0x80222c80
 */
void SimulateVehiclePhysicsSubstep(Vehicle *vehicle);

/**
 * Place the matrix of a vehicle, leant into its skid, and the matrices of its wheels, turned by
 * their roll and the front ones by their steering.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x80221e0c
 * @ghidraAddress JPN: 0x8022293c
 * @ghidraAddress EUR: 0x8022314c
 */
void TransformAllWheelMatrices(Vehicle *vehicle);

/**
 * Steer a vehicle by the stick, change its gears, and run its engine: its revolutions follow the
 * front wheels in gear, and its throttle the torque at them.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x80222050
 * @ghidraAddress JPN: 0x80222b80
 * @ghidraAddress EUR: 0x80223390
 */
void UpdateVehicleEngineAndGearbox(Vehicle *vehicle);

/**
 * Set up the body of a vehicle and the bodies of its wheels, and their forces, for the model of its
 * car.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x80222d54
 * @ghidraAddress JPN: 0x80223884
 * @ghidraAddress EUR: 0x80224094
 */
void InitVehiclePhysics(Vehicle *vehicle);

/**
 * Bind a vehicle to the input of its player and reset it for a race.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x80226100
 * @ghidraAddress JPN: 0x80226c30
 * @ghidraAddress EUR: 0x80227440
 */
void InitPlayerVehicleSlot(Vehicle *vehicle);

/**
 * Reset a vehicle, paint it the colour of its player, and leave it solid and undriven.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x802260a0
 * @ghidraAddress JPN: 0x80226bd0
 * @ghidraAddress EUR: 0x802273e0
 */
void InitVehicleColor(Vehicle *vehicle);

/**
 * Drive a vehicle by the input of its player for a frame.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x80226d7c
 * @ghidraAddress JPN: 0x802278d0
 * @ghidraAddress EUR: 0x802280e0
 */
void UpdatePhysicsStep(Vehicle *vehicle);

/**
 * Put a vehicle that has been upside down too long, or has fallen below the track, back on the
 * route at its point, as a ghost that fades back in.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x80226248
 * @ghidraAddress JPN: 0x80226d78
 * @ghidraAddress EUR: 0x80227588
 */
void RespawnVehicle(Vehicle *vehicle);

/**
 * Clear the smoke and the spray of the wheels of a vehicle.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x802288d4
 * @ghidraAddress JPN: 0x80229428
 * @ghidraAddress EUR: 0x80229c4c
 */
void ResetVehicleEffects(Vehicle *vehicle);

/**
 * Clear the dents of a vehicle.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x80228a3c
 * @ghidraAddress JPN: 0x80229590
 * @ghidraAddress EUR: 0x80229db4
 */
void ClearVehicleDamageState(Vehicle *vehicle);

/**
 * Put a vehicle on the grid for the start of the race, with its model, cameras and effects reset.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x80228a6c
 * @ghidraAddress JPN: 0x802295c0
 * @ghidraAddress EUR: 0x80229de4
 */
void SetupVehicleForRaceStart(Vehicle *vehicle);

/**
 * Do nothing. Nothing calls it.
 *
 * @param unused Not read.
 * @ghidraAddress USA: 0x80228e44
 */
void UnusedRaceStartStub(s32 unused);

/**
 * Set up a racer slot for the start of the race: attach its vehicle, or place it on the route of
 * the track when it has none.
 *
 * @param slot The slot of g_aRacerSlots.
 * @ghidraAddress USA: 0x802291f8
 * @ghidraAddress JPN: 0x80229d4c
 * @ghidraAddress EUR: 0x8022a570
 */
void InitRacerSlot(RacerSlot *slot);

/**
 * Give each racer its place in the race, by how far along the route it is.
 *
 * @ghidraAddress USA: 0x80229550
 * @ghidraAddress JPN: 0x8022a080
 * @ghidraAddress EUR: 0x8022a8c8
 */
void SortRacersByPosition(void);

/**
 * Move the vehicles the computer drives to the racers nearest a player, so that the racers around
 * the player are driven in full.
 *
 * @param player The vehicle of the player.
 * @ghidraAddress USA: 0x80229700
 * @ghidraAddress JPN: 0x8022a230
 * @ghidraAddress EUR: 0x8022aa78
 */
void ManageAiRacerSlots(Vehicle *player);

/**
 * Clear the flag of a vehicle that marks the change of its camera.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x8022bcac
 * @ghidraAddress JPN: 0x8022c974
 * @ghidraAddress EUR: 0x8022d1bc
 */
void ClearCameraChanged(Vehicle *vehicle);

/**
 * Move a racer for a frame: drive its vehicle, or advance it along the route of the track when it
 * has none.
 *
 * @param slot The slot of g_aRacerSlots.
 * @ghidraAddress USA: 0x8022c9fc
 * @ghidraAddress JPN: 0x8022d6c4
 * @ghidraAddress EUR: 0x8022df0c
 */
void UpdateRacerSlotMotion(RacerSlot *slot);

/**
 * Update the race of a racer's vehicle for a frame: its timers, messages, progress and velocity.
 *
 * @param slot The slot of g_aRacerSlots.
 * @ghidraAddress USA: 0x8022cf88
 * @ghidraAddress JPN: 0x8022dc50
 * @ghidraAddress EUR: 0x8022e498
 */
void UpdateRacerSlotRaceState(RacerSlot *slot);

#endif
