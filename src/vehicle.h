#ifndef COASTLINE_VEHICLE_H
#define COASTLINE_VEHICLE_H

/**
 * @file
 * The vehicles of a race.
 */

#include <PR/ultratypes.h>

#include "render.h"
#include "vecmath.h"

/** The number of vehicles a race can hold. */
#define MAX_VEHICLES 4

/**
 * The replay state of a vehicle. Its fields join this structure as the routines that use them are
 * reconstructed.
 */
typedef struct {
    u8 unreconstructed0[0x44];
    /** The ghost recording the vehicle plays back, or NULL. */
    u8 *ghostData;
    u8 unreconstructed48[0x4];
    u32 ghostSize;
} ReplayState;

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

/** The model of a car. Its fields join this structure as they are reconstructed. */
typedef struct {
    u8 unreconstructed0[0xB0];
    /** How far forward of the vehicle the cockpit camera sits. */
    f32 cockpitForward;
    /** Half of how far forward of the vehicle the mirror camera sits. */
    f32 mirrorForward;
    /** How far above the vehicle the cockpit and mirror cameras sit. */
    f32 cockpitUp;
} VehicleSpec;

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
    u8 unreconstructed4[0x20 - 0x4];
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

/**
 * A vehicle. The game prints its size, 0x2090 bytes, at start-up. Its fields join this structure as
 * the routines that use them are reconstructed.
 */
typedef struct {
    /** The axes and position of the vehicle. */
    CameraFrame frame;
    u8 unreconstructed44[0x1CC - 0x44];
    /** A vector whose length slows the chase camera. */
    Vec3f chaseLagSource;
    u8 unreconstructed1D8[0x1E8 - 0x1D8];
    Vec3f velocity;
    u8 unreconstructed1F4[0xDF4 - 0x1F4];
    /** The revolutions of the engine a minute. */
    f32 engineRpm;
    u8 unreconstructedDF8[0xE38 - 0xDF8];
    /** The speed along the vehicle, below 0 in reverse. */
    f32 forwardSpeed;
    u8 unreconstructedE3C[0xE40 - 0xE3C];
    /** The forward gear, from 0. */
    s32 gear;
    u8 unreconstructedE44[0xE5C - 0xE44];
    /** The profile of the player who drives the vehicle. */
    PlayerProfile *profile;
    u8 unreconstructedE60[0xF48 - 0xE60];
    /** Set to 2 when the active camera changes. */
    s32 cameraChanged;
    /** Whether the cameras hold rigidly to the frame of the vehicle. */
    s32 cameraLocked;
    u8 unreconstructedF50[0xF78 - 0xF50];
    /** The laps finished. */
    s32 lap;
    u8 unreconstructedF7C[0xF80 - 0xF7C];
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
    u8 unreconstructedFA8[0xFAC - 0xFA8];
    /** The place in the race, from 0 for the leader. */
    s32 racePosition;
    /** The message shown large in the middle of the view, or NULL. */
    const char *message;
    u8 unreconstructedFB4[0xFB8 - 0xFB4];
    /** The message shown smaller when there is no large one, or NULL. */
    const char *subMessage;
    u8 unreconstructedFBC[0xFE4 - 0xFBC];
    /** The speed the display shows, in kilometers an hour. */
    f32 speedKph;
    u8 unreconstructedFE8[0x1DE8 - 0xFE8];
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
    u8 unreconstructed1FC0[0x205C - 0x1FC0];
    /** The index of the car in g_aCarModels. */
    s32 carModel;
    u8 unreconstructed2060[0x2074 - 0x2060];
    ReplayState *replay;
    VehicleSpec *spec;
    u8 unreconstructed207C[0x2090 - 0x207C];
} Vehicle;

/** @ghidraAddress USA: 0x8031b760 */
extern Vehicle g_aVehicles[MAX_VEHICLES];

/**
 * Bind a vehicle to the input of its player and reset it for a race.
 *
 * @ghidraAddress USA: 0x80226100
 */
void InitPlayerVehicleSlot(Vehicle *vehicle);

#endif
