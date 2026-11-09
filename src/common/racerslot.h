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
    u8 **colorPanels;    /*!< The colour panels of the opponent's vehicle. */
    s32 colorPanelCount; /*!< The number of colour panels. */

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
     * Update the vehicle assigned to this racer from remote race state.
     *
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
    /** @return The racer's current position. */
    Vec3f &GetPosition() {
        return position;
    }
    /** @return The racer's position in the previous frame. */
    Vec3f &GetPreviousPosition() {
        return previousPosition;
    }
    /** @return The racer's velocity. */
    Vec3f &GetVelocity() {
        return velocity;
    }
    /** @return The racer's vehicle colour components. */
    u8 *GetColor() {
        return color;
    }
    /** @return The racer's speed in kilometres an hour. */
    f32 GetSpeedKph() const {
        return speedKph;
    }
    /** Store the racer's speed in kilometres an hour and return it.
     * @param value The new value.
     * @return The stored value.
     */
    f32 SetSpeedKph(f32 value) {
        return speedKph = value;
    }
    /** @return The route section containing the racer. */
    TrackSection *GetSection() const {
        return section;
    }
    /** Store the route section containing the racer and return it.
     * @param value The new value.
     * @return The stored value.
     */
    TrackSection *SetSection(TrackSection *value) {
        return section = value;
    }
    /** @return The racer's point index in its route section. */
    s32 GetPointIndex() const {
        return pointIndex;
    }
    /** Store the racer's point index in its route section and return it.
     * @param value The new value.
     * @return The stored value.
     */
    s32 SetPointIndex(s32 value) {
        return pointIndex = value;
    }
    /** @return The current lap time in seconds. */
    f32 GetLapTime() const {
        return lapTime;
    }
    /** Store the current lap time in seconds and return it.
     * @param value The new value.
     * @return The stored value.
     */
    f32 SetLapTime(f32 value) {
        return lapTime = value;
    }
    /** @return The shortest completed lap time in seconds. */
    f32 GetBestLapTime() const {
        return bestLapTime;
    }
    /** Store the shortest completed lap time in seconds and return it.
     * @param value The new value.
     * @return The stored value.
     */
    f32 SetBestLapTime(f32 value) {
        return bestLapTime = value;
    }
    /** @return The total race time in seconds. */
    f32 GetTotalTime() const {
        return totalTime;
    }
    /** Store the total race time in seconds and return it.
     * @param value The new value.
     * @return The stored value.
     */
    f32 SetTotalTime(f32 value) {
        return totalTime = value;
    }
    /** @return The time remaining in seconds. */
    f32 GetTimeLeft() const {
        return timeLeft;
    }
    /** Store the time remaining in seconds and return it.
     * @param value The new value.
     * @return The stored value.
     */
    f32 SetTimeLeft(f32 value) {
        return timeLeft = value;
    }
    /** @return The number of completed laps. */
    s32 GetLap() const {
        return lap;
    }
    /** Store the number of completed laps and return it.
     * @param value The new value.
     * @return The stored value.
     */
    s32 SetLap(s32 value) {
        return lap = value;
    }
    /** @return The number of laps counted by distance travelled. */
    s32 GetDistanceLaps() const {
        return distanceLaps;
    }
    /** Store the number of laps counted by distance travelled and return it.
     * @param value The new value.
     * @return The stored value.
     */
    s32 SetDistanceLaps(s32 value) {
        return distanceLaps = value;
    }
    /** @return The farthest gate passed across laps. */
    s32 GetMaxGate() const {
        return maxGate;
    }
    /** Store the farthest gate passed across laps and return it.
     * @param value The new value.
     * @return The stored value.
     */
    s32 SetMaxGate(s32 value) {
        return maxGate = value;
    }
    /** @return The last gate crossed across laps. */
    s32 GetGate() const {
        return gate;
    }
    /** Store the last gate crossed across laps and return it.
     * @param value The new value.
     * @return The stored value.
     */
    s32 SetGate(s32 value) {
        return gate = value;
    }
    /** @return The distance travelled along the route. */
    f32 GetDistance() const {
        return distance;
    }
    /** Store the distance travelled along the route and return it.
     * @param value The new value.
     * @return The stored value.
     */
    f32 SetDistance(f32 value) {
        return distance = value;
    }
    /** @return The racer's place, zero for the leader. */
    s32 GetRacePosition() const {
        return racePosition;
    }
    /** Store the racer's place, zero for the leader and return it.
     * @param value The new value.
     * @return The stored value.
     */
    s32 SetRacePosition(s32 value) {
        return racePosition = value;
    }
    /** @return The vehicle assigned to the racer. */
    Vehicle *GetVehicle() const {
        return vehicle;
    }
    /** Store the vehicle assigned to the racer and return it.
     * @param value The new value.
     * @return The stored value.
     */
    Vehicle *SetVehicle(Vehicle *value) {
        return vehicle = value;
    }
    /** @return The racer's index in g_aRacerSlots. */
    s32 GetIndex() const {
        return index;
    }
    /** Store the racer's index in g_aRacerSlots and return it.
     * @param value The new value.
     * @return The stored value.
     */
    s32 SetIndex(s32 value) {
        return index = value;
    }
    /** @return The RACER_ status bits. */
    u32 GetStatus() const {
        return status;
    }
    /** Store the RACER_ status bits and return it.
     * @param value The new value.
     * @return The stored value.
     */
    u32 SetStatus(u32 value) {
        return status = value;
    }
    /** @return Whether the racer is a straggler. */
    s32 GetStraggler() const {
        return straggler;
    }
    /** Store whether the racer is a straggler and return it.
     * @param value The new value.
     * @return The stored value.
     */
    s32 SetStraggler(s32 value) {
        return straggler = value;
    }
    /** @return The opponent's vehicle colour panels. */
    u8 **GetColorPanels() const {
        return colorPanels;
    }
    /** Store the opponent's vehicle colour panels and return it.
     * @param value The new value.
     * @return The stored value.
     */
    u8 **SetColorPanels(u8 **value) {
        return colorPanels = value;
    }
    /** @return The number of colour panels. */
    s32 GetColorPanelCount() const {
        return colorPanelCount;
    }
    /** Store the number of colour panels and return it.
     * @param value The new value.
     * @return The stored value.
     */
    s32 SetColorPanelCount(s32 value) {
        return colorPanelCount = value;
    }

    /**
     * Free the colour panels allocated for this opponent.
     *
     * @ghidraAddress BRD3D: 0x100664c0
     * @ghidraAddress BRGlide: 0x1005f530
     */
    void FreeEnemyCarColorPanels(void);

    /**
     * Initialise this racer for the start of a race.
     *
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
 * @ghidraAddress Asia: 0x80323450
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
 * @ghidraAddress Asia: 0x80228D48
 * @ghidraAddress EUR: 0x8022a570
 * @ghidraAddress JPN: 0x80229d4c
 */
void InitRacerSlot(RacerSlot *slot);

/**
 * Give each racer its place by distance, or read assigned vehicle positions from the network.
 *
 * @ghidraAddress USA: 0x80229550
 * @ghidraAddress Asia: 0x802290A0
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
 * @ghidraAddress Asia: 0x8022CAD8
 * @ghidraAddress EUR: 0x8022e498
 * @ghidraAddress JPN: 0x8022dc50
 */
void UpdateRacerSlotRaceState(RacerSlot *slot);
#endif /* TGR_PLATFORM == TGR_PLATFORM_N64 */

#if TGR_BOSS_RALLY
/**
 * Build the colour panels of an opponent's vehicle for a racer.
 *
 * @param slot The opponent racer.
 * @ghidraAddress BRD3D: 0x10065d50
 * @ghidraAddress BRGlide: 0x1005edc0
 */
void MakeEnemyCarColorPanels(RacerSlot *slot);

/**
 * Apply an opponent's colour panels to its vehicle.
 *
 * @param vehicle The opponent vehicle.
 * @param slot The opponent racer.
 * @ghidraAddress BRD3D: 0x100661b0
 * @ghidraAddress BRGlide: 0x1005f220
 */
void ApplyEnemyCarColorPanels(Vehicle *vehicle, RacerSlot *slot);
#endif /* TGR_BOSS_RALLY */

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_RACERSLOT_H */
