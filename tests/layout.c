#include <stddef.h>

#include <PR/os_cont.h>

#include "common/audiomix.h"
#include "common/effects.h"
#include "common/input.h"
#include "common/model.h"
#include "common/physics.h"
#include "common/race.h"
#include "common/track.h"
#include "common/trackrender.h"
#include "common/vehicle.h"
#include "decaleditor.h"
#include "options.h"
#include "paintshop.h"
#include "racemenu.h"
#include "sprite.h"

#ifdef __sgi
/*
 * IDO is C89, where a condition that fails gives a bit-field of a negative width, which IDO
 * rejects. An array of a negative size is only a warning to IDO.
 */
#define LAYOUT_JOIN(a, b) a##b
#define LAYOUT_NAME(line) LAYOUT_JOIN(layout_check_, line)
#define _Static_assert(condition, message)                                                         \
    typedef struct {                                                                               \
        int field : (condition) ? 1 : -1;                                                          \
    } LAYOUT_NAME(__LINE__)
#endif

/* The layouts the physics reads, against the offsets in the disassembly. */

_Static_assert(sizeof(RigidBody) == 0x208, "body");
_Static_assert(offsetof(RigidBody, stepStates) == 0x114, "steps");
_Static_assert(offsetof(Vehicle, index) == 0x140 && offsetof(Vehicle, body) == 0x148,
               "vehicleindex");
_Static_assert(offsetof(RigidBody, ground) == 0x19C, "ground");
_Static_assert(offsetof(RigidBody, surface) == 0x1A0, "surface");
_Static_assert(offsetof(RigidBody, groundNormal) == 0x1A4, "normal");
_Static_assert(offsetof(RigidBody, groundOffset) == 0x1B0, "offset");
_Static_assert(offsetof(RigidBody, contactFrames) == 0x1B4, "contact");
_Static_assert(offsetof(RigidBody, springRate) == 0x1B8, "spring");
_Static_assert(offsetof(RigidBody, damping) == 0x1BC, "damping");
_Static_assert(offsetof(RigidBody, steerAngle) == 0x1C0, "steer");
_Static_assert(offsetof(RigidBody, spinRate) == 0x1C4, "spin");
_Static_assert(offsetof(RigidBody, radius) == 0x1C8, "radius");
_Static_assert(offsetof(RigidBody, drive) == 0x1CC, "drive");
_Static_assert(offsetof(RigidBody, brake) == 0x1D0, "brake");
_Static_assert(offsetof(RigidBody, rollAngle) == 0x1D4, "roll");
_Static_assert(offsetof(RigidBody, suspension) == 0x1D8, "suspension");
_Static_assert(offsetof(RigidBody, boxSize) == 0x1DC, "box");
_Static_assert(offsetof(RigidBody, boxCenterZ) == 0x1E8, "boxz");
_Static_assert(offsetof(RigidBody, impactPoint) == 0x1EC, "impact");
_Static_assert(offsetof(RigidBody, uprightFrames) == 0x1F8, "upright");
_Static_assert(offsetof(RigidBody, impactSpeed) == 0x1FC, "speed");
_Static_assert(offsetof(RigidBody, tireType) == 0x1FD, "tyre");
_Static_assert(offsetof(RigidBody, vehicleImpactVolume) == 0x1FE, "vvol");
_Static_assert(offsetof(RigidBody, impactVolume) == 0x1FF, "vol");
_Static_assert(offsetof(RigidBody, framesSinceImpact) == 0x200, "since");
_Static_assert(offsetof(RigidBody, framesSinceVehicleImpact) == 0x201, "vsince");
_Static_assert(offsetof(RigidBody, landed) == 0x203, "landed");
_Static_assert(offsetof(RigidBody, skidding) == 0x204, "skid");
_Static_assert(offsetof(Vehicle, body) == 0x148, "vbody");
_Static_assert(offsetof(Vehicle, wheelBodies) == 0x350, "wheels");
_Static_assert(offsetof(Vehicle, wheelBodies[1].contactFrames) == 0x70C, "speedgate");
_Static_assert(offsetof(Vehicle, forces) == 0xB70, "forces");
_Static_assert(offsetof(Vehicle, forces[VEHICLE_FORCE_DRAG]) == 0xCD0, "drag");
_Static_assert(offsetof(Vehicle, forces[VEHICLE_FORCE_WHEEL_3]) == 0xD50, "wheel3");
_Static_assert(offsetof(Vehicle, steerAngle) == 0xDF0, "steerangle");
_Static_assert(offsetof(Vehicle, gear) == 0xE40, "gear");
_Static_assert(offsetof(Vehicle, frontLoad) == 0xE44, "frontload");
_Static_assert(offsetof(Vehicle, frontSliding) == 0xE48, "frontsliding");
_Static_assert(offsetof(Vehicle, rearLoad) == 0xE4C, "rearload");
_Static_assert(offsetof(Vehicle, rearSliding) == 0xE50, "rearsliding");
_Static_assert(offsetof(Vehicle, steerReturn) == 0xE51, "steerreturn");
_Static_assert(offsetof(Vehicle, skipDrive) == 0xE54, "skipdrive");
_Static_assert(offsetof(Vehicle, profile) == 0xE5C, "profile");
_Static_assert(offsetof(Vehicle, tireType) == 0xE60, "tiretype");
_Static_assert(offsetof(Vehicle, suspensionStiffness) == 0xE64, "stiffness");
_Static_assert(offsetof(Vehicle, steeringMode) == 0xE68, "steering");
_Static_assert(offsetof(Vehicle, chosenTransmission) == 0xE6C, "chosen");
_Static_assert(offsetof(Vehicle, racer) == 0xED0, "racer");
_Static_assert(offsetof(Vehicle, ghostMode) == 0x2063, "ghost");
_Static_assert(offsetof(Vehicle, opacity) == 0x2064, "opacity");
_Static_assert(offsetof(Vehicle, wrongWayFrames) == 0x206C, "wrongway");
_Static_assert(offsetof(Vehicle, wheelFrames[2].position) == 0xF0, "wheelpos");
_Static_assert(offsetof(Vehicle, placedVelocity) == 0xFD8, "placedvel");
_Static_assert(offsetof(Vehicle, exhaustTimer) == 0x1010, "exhaust");
_Static_assert(offsetof(Vehicle, exhaustPosition) == 0x1014, "exhaustpos");
_Static_assert(offsetof(Vehicle, smokeTimers) == 0x1020, "smoke");
_Static_assert(offsetof(Vehicle, smokePositions) == 0x1030, "smokepos");
_Static_assert(offsetof(Vehicle, sprayFramesLeft) == 0x1060, "sprayframes");
_Static_assert(offsetof(Vehicle, spraySurfaces) == 0x1070, "spraysurf");
_Static_assert(offsetof(Vehicle, sprayPhases) == 0x1080, "sprayphase");
_Static_assert(offsetof(Vehicle, sprayAdvanced) == 0x1090, "sprayadv");
_Static_assert(offsetof(Vehicle, sprayPositions) == 0x10A0, "spraypos");
_Static_assert(offsetof(Vehicle, sprayVertices) == 0x10D0, "sprayvtx");
_Static_assert(offsetof(Vehicle, sprayVertices[1][0]) == 0x1310, "sprayvtx1");
_Static_assert(offsetof(Vehicle, sprayVelocities) == 0x19D0, "sprayvel");
_Static_assert(offsetof(Vehicle, sprayVelocities[1][0]) == 0x1AA8, "sprayvel1");
_Static_assert(offsetof(Vehicle, spraySegmentSurfaces) == 0x1D30, "spraysegsurf");
_Static_assert(offsetof(Vehicle, spraySegmentSurfaces[1][0]) == 0x1D42, "spraysegsurf1");
_Static_assert(offsetof(Vehicle, sprayOrigin) == 0x1D78, "sprayorigin");
_Static_assert(offsetof(Vehicle, sprayMtx) == 0x1D88, "spraymtx");
_Static_assert(offsetof(Vehicle, groundInstanceCount) == 0x2000, "raydone");
_Static_assert(offsetof(Vehicle, groundDistance) == 0x2048, "grounddist");
_Static_assert(offsetof(Vehicle, body.skidding) == 0x34C, "skidding");
_Static_assert(sizeof(Particle) == 0x20, "particle");
_Static_assert(offsetof(Particle, next) == 0x1C, "next");
_Static_assert(offsetof(Particle, alpha) == 0x1F, "alpha");
_Static_assert(offsetof(Vehicle, fogFactor) == 0x1DE4, "fog");
_Static_assert(offsetof(Vehicle, shadowCorners) == 0x2050, "shadow");
_Static_assert(offsetof(Vehicle, shadowCorners[1][1]) == 0x2056, "shadow2");
_Static_assert(offsetof(Vehicle, nearbyInstances) == 0x2004, "nearby");
_Static_assert(offsetof(Vehicle, nearbyInstanceCount) == 0x2044, "nearbycount");
_Static_assert(offsetof(Vehicle, cameras[VEHICLE_CAMERA_MIRROR]) == 0x1F44, "mirror");
_Static_assert(offsetof(Vehicle, cameras[VEHICLE_CAMERA_TRACKSIDE]) == 0x1EBC, "trackside");
_Static_assert(offsetof(VehicleModel, wheels[0].y) == 0xE4, "width");
_Static_assert(offsetof(VehicleModel, wheels[2]) == 0xF8, "rearwheel");
_Static_assert(offsetof(PlayerProfile, season) == 0x4, "season");
_Static_assert(offsetof(PlayerProfile, unlocks) == 0xCC, "unlocks");
_Static_assert(offsetof(PlayerInputState, playbackBuffer) == 0x44, "playback");
_Static_assert(sizeof(TrackInstance) == 0x54, "instance");
_Static_assert(offsetof(TrackInstance, displayList) == 0x44, "dl");
_Static_assert(offsetof(TrackInstance, groups) == 0x48, "groups");
_Static_assert(offsetof(TrackInstance, flags) == 0x4C, "iflags");
_Static_assert(offsetof(TrackInstance, polygonCount) == 0x52, "polys");
_Static_assert(offsetof(TrackHeader, instanceNames) == 0x5C, "names");
_Static_assert(offsetof(TrackHeader, instances) == 0x60, "instances");
_Static_assert(offsetof(TrackHeader, gridInstanceStarts) == 0x6C, "gridstarts2");
_Static_assert(offsetof(TrackHeader, rootSection) == 0x70, "root");
_Static_assert(sizeof(Lights1) == 0x18, "lights");
_Static_assert(sizeof(VisibleCell) == 4, "cell");
_Static_assert(sizeof(Vehicle) == 0x2090, "vehicle");
_Static_assert(sizeof(RacerSlot) == 0x78, "slot");
_Static_assert(offsetof(RacerSlot, vehicle) == 0x60, "slotvehicle");
_Static_assert(offsetof(RacerSlot, status) == 0x68, "status");
_Static_assert(sizeof(TrackTriangle) == 0x20, "triangle");
_Static_assert(offsetof(TrackTriangle, planeOffset) == 0xC, "plane");
_Static_assert(offsetof(TrackTriangle, surface) == 0x1E, "trisurface");
_Static_assert(offsetof(TrackHeader, vertices) == 0x14, "vertices");
_Static_assert(offsetof(TrackHeader, gridTriangles) == 0x20, "grid");
_Static_assert(offsetof(TrackHeader, gridCellStarts) == 0x24, "gridstarts");
_Static_assert(offsetof(TrackHeader, minX) == 0x28, "minx");
_Static_assert(offsetof(TrackHeader, triangleFlags) == 0x94, "flags");
_Static_assert(sizeof(TrackSurface) == 8, "face");
_Static_assert(sizeof(CarPhysicsSpec) == 0x4C, "spec");
_Static_assert(offsetof(CarPhysicsSpec, boxSize) == 0x38, "specbox");
_Static_assert(sizeof(Season) == 0x1C, "season");
_Static_assert(offsetof(Season, pointsNeeded) == 0xC, "seasonpoints");
_Static_assert(offsetof(Season, cars) == 0x10, "seasoncars");
_Static_assert(offsetof(Season, races) == 0x14, "seasonraces");
_Static_assert(sizeof(MenuItem) == 0x14, "menuitem");
_Static_assert(offsetof(MenuItem, model) == 0x8, "menuitemmodel");
_Static_assert(offsetof(MenuItem, romStart) == 0xC, "menuitemrom");
_Static_assert(sizeof(ModelPart) == 0x14, "modelpart");
_Static_assert(offsetof(ModelPart, flags) == 0x4, "modelpartflags");
_Static_assert(offsetof(ModelPart, position) == 0x8, "modelpartpos");
_Static_assert(offsetof(Model, parts) == 0x8, "modelparts");
_Static_assert(offsetof(PlayerProfile, round) == 0x5, "round");
_Static_assert(offsetof(PlayerProfile, seasonPoints) == 0x1E, "seasonpoints");
_Static_assert(offsetof(PlayerProfile, lapRecords) == 0x8C, "laprecords");
_Static_assert(offsetof(PlayerProfile, unlockedTracks) == 0xCE, "unlocked");
_Static_assert(offsetof(PlayerProfile, raceRecords) == 0xE8, "racerecords");
_Static_assert(sizeof(Vehicle) == 0x2090, "vehicle");
_Static_assert(offsetof(Vehicle, profile) == 0xE5C, "profile");
_Static_assert(offsetof(Vehicle, driver) == 0xED8, "driver");
_Static_assert(offsetof(Vehicle, cameras[VEHICLE_CAMERA_TRACKSIDE]) == 0x1EBC, "trackside");
_Static_assert(offsetof(Vehicle, groundInstanceCount) == 0x2000, "raydone");
_Static_assert(offsetof(Vehicle, nearbyInstanceCount) == 0x2044, "nearby");
_Static_assert(offsetof(Vehicle, input) == 0x2074, "input");
_Static_assert(offsetof(PlayerInputState, stickX) == 0x18, "stickx");
_Static_assert(sizeof(MixerChannel) == 0x18, "channel");
_Static_assert(offsetof(MixerChannel, step) == 0x8, "channelstep");
_Static_assert(offsetof(MixerChannel, mixVolume) == 0x10, "channelmix");
_Static_assert(sizeof(VehicleModel) == 0xDF88, "vmodel");
_Static_assert(offsetof(VehicleModel, textures) == 0x14, "vmodeltex");
_Static_assert(offsetof(VehicleModel, displayLists) == 0x18, "vmodeldl");
_Static_assert(offsetof(VehicleModel, displayLists[0][VEHICLE_DL_NEAR_DETAIL]) == 0x24, "dlnear");
_Static_assert(offsetof(VehicleModel, displayLists[0][VEHICLE_DL_HEADLIGHT_BEAMS]) == 0x2C,
               "dlbeam");
_Static_assert(offsetof(VehicleModel, displayLists[0][VEHICLE_DL_HIGHLIGHTED]) == 0x3C, "dlhi");
_Static_assert(offsetof(VehicleModel, cockpitForward) == 0xB0, "cockpit");
_Static_assert(offsetof(VehicleModel, wheelDisplayLists[0][VEHICLE_WHEEL_DL_NORMAL]) == 0xBC,
               "wheeldl");
_Static_assert(offsetof(VehicleModel, wheelDisplayLists[0][VEHICLE_WHEEL_DL_SNOW]) == 0xC4,
               "snowdl");
_Static_assert(offsetof(VehicleModel, textureCount) == 0x10 &&
                   offsetof(VehicleModel, unreconstructed94) == 0x94 &&
                   offsetof(VehicleModel, decalMasks) == 0x11C,
               "modelptrs");
_Static_assert(offsetof(VehicleModel, wheels) == 0xE0, "wheels");
_Static_assert(offsetof(VehicleModel, wheels[1]) == 0xEC, "frontwheel");
_Static_assert(offsetof(VehicleModel, materialTextures[VEHICLE_MATERIAL_HEADLIGHTS]) == 0x11A,
               "headtex");
_Static_assert(offsetof(VehicleModel, materialTextures[VEHICLE_MATERIAL_REAR_LIGHTS]) == 0x11B,
               "reartex");
_Static_assert(sizeof(ModelTexture) == 0x24, "modeltex");
_Static_assert(offsetof(Vehicle, throttle) == 0xE38, "throttle");
_Static_assert(offsetof(Vehicle, inTunnel) == 0x1DC8, "tunnel");
_Static_assert(offsetof(Vehicle, cameraHeading) == 0x1DCC, "heading");
_Static_assert(offsetof(Vehicle, cameras[VEHICLE_CAMERA_MIRROR]) == 0x1F44, "mirror");
_Static_assert(offsetof(Vehicle, cameras[VEHICLE_CAMERA_BUMPER]) == 0x1E78, "bumper");
_Static_assert(offsetof(Vehicle, groundInstances) == 0x1FC0, "ground");
_Static_assert(offsetof(Vehicle, ghostMode) == 0x2063, "ghost");
_Static_assert(offsetof(Vehicle, otherLook) == 0x2068, "otherlook");
_Static_assert(offsetof(Vehicle, model) == 0x2078, "model");
_Static_assert(offsetof(Vehicle, dents) == 0x207C, "dents");
_Static_assert(offsetof(Vehicle, dentJitter) == 0x208C, "jitter");
_Static_assert(offsetof(CarModel, shadowLength) == 0x58, "shadowlen");
_Static_assert(sizeof(CarModel) == 0x60, "carmodel");
_Static_assert(offsetof(PlayerInputState, playbackBuffer) == 0x44, "playback");
_Static_assert(sizeof(Lights1) == 0x18, "lights");
_Static_assert(offsetof(TrackInstance, flags) == 0x4C && sizeof(TrackInstance) == 0x54, "instance");
_Static_assert(offsetof(PlayerProfile, seasonPlaces) == 0x6, "places");
_Static_assert(offsetof(PlayerProfile, seasonTimes) == 0x2C, "times");
_Static_assert(offsetof(PlayerProfile, handling) == 0xD4, "handling");
_Static_assert(offsetof(PlayerProfile, transmission) == 0xD8, "transmission");
_Static_assert(offsetof(PlayerProfile, tires) == 0xDC, "tires");
_Static_assert(offsetof(PlayerProfile, suspension) == 0xE0, "suspension");
_Static_assert(offsetof(PlayerProfile, controllerLayout) == 0xE4, "layout");
_Static_assert(offsetof(Vehicle, customDecals) == 0xE58, "decals");
_Static_assert(offsetof(Vehicle, lap) == 0xF78, "lap");
_Static_assert(offsetof(Vehicle, lapTimes) == 0xF84, "laptimes");
_Static_assert(offsetof(Vehicle, bestLapTime) == 0xF98, "bestlaptime");
_Static_assert(offsetof(Vehicle, bestLap) == 0xF9C, "bestlap");
_Static_assert(offsetof(Vehicle, totalTime) == 0xFA0, "totaltime");
_Static_assert(offsetof(Vehicle, racePosition) == 0xFAC, "position");
_Static_assert(offsetof(Vehicle, selectedCar) == 0x2058, "selected");
_Static_assert(offsetof(Vehicle, carModel) == 0x205C, "carmodel");
_Static_assert(offsetof(Vehicle, color) == 0x2060, "color");
_Static_assert(offsetof(CarModel, driveType) == 0x3C, "drive");
_Static_assert(offsetof(CarModel, engineLayout) == 0x40, "engine");
_Static_assert(offsetof(CarModel, acceleration) == 0x44, "accel");
_Static_assert(offsetof(CarModel, maxSpeed) == 0x48, "maxspeed");
_Static_assert(offsetof(CarModel, gearCount) == 0x54, "gears");
_Static_assert(offsetof(VehicleModel, materialTextures[VEHICLE_MATERIAL_PAINT]) == 0x112,
               "painttex");
_Static_assert(sizeof(Ambient) == 8 && sizeof(Light) == 16, "lights2");

_Static_assert(sizeof(PlayerProfile) == 0x128, "profilesize");
_Static_assert(offsetof(PlayerProfile, unlocks) == 0xCC &&
                   offsetof(PlayerProfile, unlockedTracks) == 0xCE,
               "unlocks");
_Static_assert(offsetof(PlayerProfile, handling) == 0xD4 &&
                   offsetof(PlayerProfile, controllerLayout) == 0xE4,
               "equip");
_Static_assert(sizeof(OptionSettings) == 0xC, "optset");
_Static_assert(sizeof(MenuItem) == 0x14, "menuitem");
_Static_assert(sizeof(PlayerInputState) == 0x15C && offsetof(PlayerInputState, stickX) == 0x18,
               "inputstate");
_Static_assert(sizeof(OSPfs) == 0x68 && offsetof(OSPfs, id) == 0xC, "pfs");
_Static_assert(offsetof(Sprite, width) == 0x10 && offsetof(Sprite, height) == 0x14, "spritewh");
_Static_assert(offsetof(Sprite, originX) == 0x1C && offsetof(Sprite, originY) == 0x20,
               "spriteorigin");
_Static_assert(offsetof(Sprite, drawWidth) == 0x24 && offsetof(Sprite, drawHeight) == 0x28 &&
                   offsetof(Sprite, style) == 0x2C && sizeof(Sprite) == 0x30,
               "sprite draw");
_Static_assert(offsetof(Sprite, bitsPerTexel) == 0xC && offsetof(Sprite, stripHeight) == 0x18,
               "spritedepth");
_Static_assert(sizeof(ScreenRect) == 0x10 && sizeof(PaletteSwatch) == 0x14 &&
                   offsetof(PaletteSwatch, color) == 0x10,
               "swatch");
_Static_assert(sizeof(DecalGlyph) == 0x1C && offsetof(DecalGlyph, character) == 8 &&
                   offsetof(DecalGlyph, cellX) == 0xC && offsetof(DecalGlyph, kernRight) == 0x18,
               "glyph");
_Static_assert(offsetof(Vehicle, profile) == 0xE5C && sizeof(Vehicle) == 0x2090, "vehprofile");
_Static_assert(offsetof(TrackHeader, specials) == 0x164 &&
                   offsetof(TrackHeader, specialCount) == 0x224,
               "specials");
_Static_assert(sizeof(TrackSpecial) == 12 && offsetof(TrackSpecial, type) == 8, "special");
_Static_assert(sizeof(TrackPoint) == 0x28 && offsetof(TrackPoint, right) == 0x18, "trackpoint");
_Static_assert(offsetof(TrackSection, points) == 0x40, "sectionpoints");
_Static_assert(offsetof(Vehicle, messageText) == 0xFC0 && offsetof(Vehicle, tableIndex) == 0xE34,
               "msgtext");
_Static_assert(offsetof(PlayerProfile, unlockedWeathers) == 0xD0, "weathers");
_Static_assert(offsetof(Season, unlocks) == 0x12 && sizeof(Season) == 0x1C, "season");
_Static_assert(sizeof(TrackMenuItem) == 0x17C && offsetof(TrackMenuItem, arcadeTimes) == 0x2C,
               "trackitem");
_Static_assert(sizeof(ArcadeTimes) == 0x1C && offsetof(ArcadeTimes, startTime) == 0x18,
               "arcadetimes");
_Static_assert(sizeof(RaceCaption) == 0x20 && sizeof(CreditLine) == 0x10 &&
                   sizeof(RacePhaseStep) == 8,
               "racedata");
_Static_assert(sizeof(MixerChannel) == 0x18 && offsetof(MixerChannel, mixVolume) == 0x10, "mixer");
_Static_assert(offsetof(RacerSlot, vehicle) == 0x60 && offsetof(RacerSlot, status) == 0x68, "slot");
_Static_assert(sizeof(ViewportRect) == 0x14 && offsetof(ViewportRect, vehicleIndex) == 0x10,
               "viewport");
_Static_assert(offsetof(PlayerInputState, recordBuffers) == 0x2C &&
                   offsetof(PlayerInputState, playbackLimit) == 0x4C,
               "rec");
_Static_assert(offsetof(PlayerInputState, controllerMissing) == 0x28, "missing");
_Static_assert(offsetof(CameraFrame, side) == 0x10 && offsetof(CameraFrame, up) == 0x20, "axes");
_Static_assert(offsetof(Vehicle, cameras) == 0x1DF0 && sizeof(CameraFrame) == 0x44, "cams");
_Static_assert(offsetof(CameraFrame, position) == 0x30 && offsetof(CameraFrame, fov) == 0x40,
               "camframe");
_Static_assert(offsetof(Vehicle, forcedForwardFrames) == 0xE70 &&
                   offsetof(Vehicle, forwardFrames) == 0xE7C && offsetof(Vehicle, racer) == 0xED0,
               "aiframes");
_Static_assert(offsetof(Vehicle, aiTarget) == 0xEDC && offsetof(Vehicle, trackPoint) == 0xEE8 &&
                   offsetof(Vehicle, trackForward) == 0xEF4,
               "track1");
_Static_assert(offsetof(Vehicle, trackSide) == 0xF00 && offsetof(Vehicle, trackUp) == 0xF0C &&
                   offsetof(Vehicle, trackSideDistance) == 0xF18,
               "track2");
_Static_assert(offsetof(Vehicle, trackOffset) == 0xF1C &&
                   offsetof(Vehicle, trackSideOffset) == 0xF28 &&
                   offsetof(Vehicle, cameraChanged) == 0xF48,
               "track3");
_Static_assert(offsetof(Vehicle, joltPosition) == 0xFEC &&
                   offsetof(Vehicle, joltVelocity) == 0xFF8 &&
                   offsetof(Vehicle, joltOffset) == 0x1004 &&
                   offsetof(Vehicle, exhaustTimer) == 0x1010,
               "jolt");
_Static_assert(offsetof(Vehicle, steerAngle) == 0xDF0 &&
                   offsetof(Vehicle, chasePosition) == 0x1FA0 &&
                   offsetof(Vehicle, distance) == 0xFA8,
               "misc");
_Static_assert(offsetof(Vehicle, wheelBodies[0].contactFrames) == 0x504 &&
                   offsetof(Vehicle, body.state.velocity) == 0x1CC &&
                   offsetof(Vehicle, body.state.angularVelocity) == 0x1E8,
               "bodies");
_Static_assert(offsetof(Vehicle, body.uprightFrames) == 0x340 &&
                   offsetof(RigidBody, ground) == 0x19C && offsetof(RigidBody, surface) == 0x1A0 &&
                   offsetof(RigidBody, contactFrames) == 0x1B4,
               "wheel");
_Static_assert(offsetof(Vehicle, steering) == 0x1DD4 && offsetof(Vehicle, accelerator) == 0x1DDC &&
                   offsetof(Vehicle, rollInput) == 0x1DE0,
               "controls");
_Static_assert(offsetof(RacerSlot, straggler) == 0x74 && sizeof(RacerSlot) == 0x78, "straggler");
_Static_assert(offsetof(TrackHeader, startPosition) == 0x40 &&
                   offsetof(TrackHeader, startHeading) == 0x4C &&
                   offsetof(TrackHeader, skyDisplayList) == 0x50,
               "start");
_Static_assert(offsetof(TrackHeader, cameraPoints) == 0x84 &&
                   offsetof(TrackHeader, cameraPointCount) == 0x88 &&
                   offsetof(TrackHeader, nearbyInstances) == 0x8C,
               "campoints");
_Static_assert(offsetof(PlayerProfile, seasonPoints) == 0x1E &&
                   offsetof(PlayerProfile, lapRecords) == 0x8C &&
                   offsetof(PlayerProfile, raceRecords) == 0xE8,
               "profile");
_Static_assert(offsetof(RacerSlot, distance) == 0x50 && offsetof(RacerSlot, color) == 0x5C &&
                   offsetof(RacerSlot, index) == 0x64 && offsetof(RacerSlot, lap) == 0x40,
               "slotfull");
_Static_assert(offsetof(Vehicle, skidFrames) == 0x144 &&
                   offsetof(Vehicle, nearestRacers) == 0xE80 &&
                   offsetof(Vehicle, slotCooldown) == 0xED4,
               "vfields1");
_Static_assert(offsetof(Vehicle, previousListenerPosition) == 0xF2C &&
                   offsetof(Vehicle, effectSound) == 0xF38 &&
                   offsetof(Vehicle, dopplerFactor) == 0xF44,
               "vfields2");
_Static_assert(offsetof(Vehicle, maxGate) == 0xF70 && offsetof(Vehicle, gate) == 0xF74 &&
                   offsetof(Vehicle, distanceLaps) == 0xF7C && offsetof(Vehicle, odometer) == 0xFE8,
               "vfields3");
_Static_assert(sizeof(TrackGate) == 0x14 && offsetof(TrackHeader, gates) == 0x98 &&
                   offsetof(TrackHeader, gateCount) == 0x160 &&
                   offsetof(TrackHeader, specials) == 0x164,
               "gates");
_Static_assert(offsetof(ArcadeTimes, gateBonuses) == 0 && offsetof(ArcadeTimes, startTime) == 0x18,
               "bonus");
_Static_assert(sizeof(TrackMenuItem) == 0x17C && offsetof(TrackMenuItem, trackRom) == 0x14 &&
                   offsetof(TrackMenuItem, sectionBRomEnd) == 0x28 &&
                   offsetof(TrackMenuItem, arcadeTimes) == 0x2C,
               "trackitem");
_Static_assert(offsetof(TrackHeader, size) == 0 && offsetof(TrackHeader, headerSize) == 4 &&
                   offsetof(TrackInstance, invScale) == 0x40,
               "trackhead");
_Static_assert(offsetof(ModelTexture, source) == 0x8 && offsetof(ModelTexture, width) == 0xC &&
                   offsetof(ModelTexture, height) == 0xE &&
                   offsetof(ModelTexture, tileT0) == 0x10 &&
                   offsetof(ModelTexture, tileS0) == 0x12 &&
                   offsetof(ModelTexture, tileT1) == 0x14 && offsetof(ModelTexture, tileS1) == 0x16,
               "texturetile");
_Static_assert(offsetof(TextureAnimation, frameCount) == 0x2 &&
                   offsetof(TextureAnimation, frames) == 0x8 &&
                   sizeof(TextureAnimationFrame) == 0xC &&
                   offsetof(TextureAnimationFrame, paletteRom) == 0x8,
               "textureanim");
_Static_assert(offsetof(TrackHeader, textureCount) == 0x18 &&
                   offsetof(TrackHeader, textures) == 0x1C &&
                   offsetof(TrackHeader, fogColor) == 0x80,
               "tracktextures");
_Static_assert(sizeof(Vp) == 0x10 && sizeof(OSTask) == 0x40 &&
                   offsetof(OSTask_t, output_buff_size) == 0x2C &&
                   offsetof(OSTask_t, data_size) == 0x34,
               "task");
