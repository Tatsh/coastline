#ifndef COASTLINE_RACERSLOT_H
#define COASTLINE_RACERSLOT_H

#include "common/types.h"
#include "common/vecmath.h"
#include "common/vehiclefwd.h"

struct TrackSection;

/** The racers of a race. */
#define RACER_SLOTS 20

/** The racer status bits that stop its clock. */
#define RACER_CLOCK_STOPPED 0x3
/** The racer status bit that disengages its vehicle's gear. */
#define RACER_HELD 0x1
/** The racer status bit that marks a finished race. */
#define RACER_FINISHED 0x2

/**
 * A racer of a race. A racer with a vehicle copies the progress of the vehicle; one without moves
 * along the route by itself, and keeps the progress a vehicle takes up when it gets one.
 */
#if TGR_BOSS_RALLY && defined(__cplusplus)
class RacerSlot {
private:
#else
typedef struct RacerSlot {
#endif
    Vec3f position;                    /*!< The position of the racer. */
    Vec3f previousPosition;            /*!< The position of the last frame. */
    Vec3f velocity;                    /*!< How far the racer moves a second. */
    f32 speedKph;                      /*!< The speed of the racer, in kilometres an hour. */
    struct TrackSection *section;      /*!< The section of the route the racer is on. */
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
    Vehicle *vehicle;                  /*!< The vehicle of the racer, or NULL. */
    s32 index;                         /*!< The index of the racer in g_aRacerSlots. */
    u32 status; /*!< The RACER_ bits. The clock of the racer runs while the bits of
                     #RACER_CLOCK_STOPPED are clear. */
    u8 unreconstructed6C[0x74 - 0x6C]; /*!< Not read by the routines reconstructed so far. */
    s32 straggler; /*!< Set for a racer past the thirteenth after the players. The championship
                        slows the vehicle of a straggler more, and the racers of each kind share
                        separate vehicles. */
#if TGR_BOSS_RALLY
    u8 **colorPanels;
    s32 colorPanelCount;

#ifdef __cplusplus

    /**
     * Count crossed gates and laps and update race records.
     *
     * @ghidraAddress BRD3D: 0x10066e90
     * @ghidraAddress BRGlide: 0x1005ff00
     */
    void UpdateRaceProgress(void);

public:
    /**
     * Drive the assigned vehicle or advance this racer along the route.
     *
     * @ghidraAddress BRD3D: 0x10068ef0
     * @ghidraAddress BRGlide: 0x10061f60
     */
    void UpdateRacerSlotMotion(void);

    /**
     * @ghidraAddress BRD3D: 0x10069330
     * @ghidraAddress BRGlide: 0x100623a0
     */
    void UpdateRemoteVehicleFrame(void);

    /**
     * Update the assigned vehicle after its movement and contacts for a frame.
     *
     * @ghidraAddress BRD3D: 0x10069370
     * @ghidraAddress BRGlide: 0x100623e0
     */
    void UpdateRacerSlotRaceState(void);
    Vec3f &GetPosition() {
        return position;
    }
    Vec3f &GetPreviousPosition() {
        return previousPosition;
    }
    Vec3f &GetVelocity() {
        return velocity;
    }
    u8 *GetColor() {
        return color;
    }
    f32 GetSpeedKph() const {
        return speedKph;
    }
    f32 SetSpeedKph(f32 value) {
        return speedKph = value;
    }
    TrackSection *GetSection() const {
        return section;
    }
    TrackSection *SetSection(TrackSection *value) {
        return section = value;
    }
    s32 GetPointIndex() const {
        return pointIndex;
    }
    s32 SetPointIndex(s32 value) {
        return pointIndex = value;
    }
    f32 GetLapTime() const {
        return lapTime;
    }
    f32 SetLapTime(f32 value) {
        return lapTime = value;
    }
    f32 GetBestLapTime() const {
        return bestLapTime;
    }
    f32 SetBestLapTime(f32 value) {
        return bestLapTime = value;
    }
    f32 GetTotalTime() const {
        return totalTime;
    }
    f32 SetTotalTime(f32 value) {
        return totalTime = value;
    }
    f32 GetTimeLeft() const {
        return timeLeft;
    }
    f32 SetTimeLeft(f32 value) {
        return timeLeft = value;
    }
    s32 GetLap() const {
        return lap;
    }
    s32 SetLap(s32 value) {
        return lap = value;
    }
    s32 GetDistanceLaps() const {
        return distanceLaps;
    }
    s32 SetDistanceLaps(s32 value) {
        return distanceLaps = value;
    }
    s32 GetMaxGate() const {
        return maxGate;
    }
    s32 SetMaxGate(s32 value) {
        return maxGate = value;
    }
    s32 GetGate() const {
        return gate;
    }
    s32 SetGate(s32 value) {
        return gate = value;
    }
    f32 GetDistance() const {
        return distance;
    }
    f32 SetDistance(f32 value) {
        return distance = value;
    }
    s32 GetRacePosition() const {
        return racePosition;
    }
    s32 SetRacePosition(s32 value) {
        return racePosition = value;
    }
    Vehicle *GetVehicle() const {
        return vehicle;
    }
    Vehicle *SetVehicle(Vehicle *value) {
        return vehicle = value;
    }
    s32 GetIndex() const {
        return index;
    }
    s32 SetIndex(s32 value) {
        return index = value;
    }
    u32 GetStatus() const {
        return status;
    }
    u32 SetStatus(u32 value) {
        return status = value;
    }
    s32 GetStraggler() const {
        return straggler;
    }
    s32 SetStraggler(s32 value) {
        return straggler = value;
    }
    u8 **GetColorPanels() const {
        return colorPanels;
    }
    u8 **SetColorPanels(u8 **value) {
        return colorPanels = value;
    }
    s32 GetColorPanelCount() const {
        return colorPanelCount;
    }
    s32 SetColorPanelCount(s32 value) {
        return colorPanelCount = value;
    }

    /**
     * @ghidraAddress BRD3D: 0x100664c0
     * @ghidraAddress BRGlide: 0x1005f530
     */
    void FreeEnemyCarColorPanels(void);

    /**
     * @ghidraAddress BRD3D: 0x100662a0
     * @ghidraAddress BRGlide: 0x1005f310
     */
    void InitRacerSlot(void);

#endif /* __cplusplus */
#endif /* TGR_BOSS_RALLY */
#if TGR_BOSS_RALLY && defined(__cplusplus)
};
#else
} RacerSlot;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The racers of the race.
 *
 * @ghidraAddress USA: 0x803239a0
 * @ghidraAddress EUR: 0x80325ea0
 * @ghidraAddress JPN: 0x80324ad0
 * @ghidraAddress BRD3D: 0x10acd498
 * @ghidraAddress BRGlide: 0x10af07f8
 */
extern RacerSlot g_aRacerSlots[RACER_SLOTS];

/**
 * Set up a racer slot for the start of the race: attach its vehicle, or place it on the route of
 * the track when it has none.
 *
 * @param slot The slot of g_aRacerSlots.
 * @ghidraAddress USA: 0x802291f8
 * @ghidraAddress EUR: 0x8022a570
 * @ghidraAddress JPN: 0x80229d4c
 */
void InitRacerSlot(RacerSlot *slot);

/**
 * Give each racer its place by distance, or read assigned vehicle positions from the network.
 *
 * @ghidraAddress USA: 0x80229550
 * @ghidraAddress EUR: 0x8022a8c8
 * @ghidraAddress JPN: 0x8022a080
 * @ghidraAddress BRD3D: 0x10066510
 * @ghidraAddress BRGlide: 0x1005f580
 */
void SortRacersByPosition(void);

#if TGR_PLATFORM == TGR_PLATFORM_N64
/**
 * Update the assigned vehicle timers, messages, progress, and velocity for a frame.
 *
 * @param slot The slot of g_aRacerSlots.
 * @ghidraAddress USA: 0x8022cf88
 * @ghidraAddress EUR: 0x8022e498
 * @ghidraAddress JPN: 0x8022dc50
 */
void UpdateRacerSlotRaceState(RacerSlot *slot);
#endif /* TGR_PLATFORM == TGR_PLATFORM_N64 */

#if TGR_BOSS_RALLY
/**
 * @ghidraAddress BRD3D: 0x10065d50
 * @ghidraAddress BRGlide: 0x1005edc0
 */
void MakeEnemyCarColorPanels(RacerSlot *slot);

/**
 * @ghidraAddress BRD3D: 0x100661b0
 * @ghidraAddress BRGlide: 0x1005f220
 */
void ApplyEnemyCarColorPanels(Vehicle *vehicle, RacerSlot *slot);
#endif /* TGR_BOSS_RALLY */

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_RACERSLOT_H */
