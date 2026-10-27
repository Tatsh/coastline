#ifndef COASTLINE_RACERSLOT_H
#define COASTLINE_RACERSLOT_H

#include "types.h"
#include "vecmath.h"

struct TrackSection;
struct Vehicle;

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
typedef struct RacerSlot {
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
    struct Vehicle *vehicle;           /*!< The vehicle of the racer, or NULL. */
    s32 index;                         /*!< The index of the racer in g_aRacerSlots. */
    u32 status; /*!< The RACER_ bits. The clock of the racer runs while the bits of
                     #RACER_CLOCK_STOPPED are clear. */
    u8 unreconstructed6C[0x74 - 0x6C]; /*!< Not read by the routines reconstructed so far. */
    s32 straggler; /*!< Set for a racer past the thirteenth after the players. The championship
                        slows the vehicle of a straggler more, and the racers of each kind share
                        separate vehicles. */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    u8 **colorPanels;
    s32 colorPanelCount;

#ifdef __cplusplus
    /**
     * @ghidraAddress BRD3D: 0x100664c0
     */
    void FreeEnemyCarColorPanels(void);
    /**
     * @ghidraAddress BRD3D: 0x100662a0
     */
    void InitRacerSlot(void);

    /**
     * Drive the assigned vehicle or advance this racer along the route.
     *
     * @ghidraAddress BRD3D: 0x10068ef0
     */
    void UpdateRacerSlotMotion(void);

    /**
     * Count crossed gates and laps and update race records.
     *
     * @ghidraAddress BRD3D: 0x10066e90
     * @stub
     */
    void UpdateRaceProgress(void);
#endif
#endif
} RacerSlot;

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

#if TGR_PLATFORM == TGR_PLATFORM_WIN
/**
 * @ghidraAddress BRD3D: 0x10065d50
 */
void MakeEnemyCarColorPanels(RacerSlot *slot);

/**
 * @ghidraAddress BRD3D: 0x100661b0
 */
void ApplyEnemyCarColorPanels(struct Vehicle *vehicle, RacerSlot *slot);
#endif

#ifdef __cplusplus
}
#endif

#endif
