#include "common/raceprogress.h"

#include "common/racerslotaccess.h"
#include "common/vehicleaccess.h"

#if TGR_BOSS_RALLY
#include <stdio.h>
#include <string.h>
#endif

#include "common/game.h"
#include "common/instantreplay.h"
#include "common/laps.h"
#include "common/laptime.h"
#include "common/mathutil.h"
#include "common/race.h"
#include "common/soundeffect.h"
#include "common/trackmenuitem.h"
#include "common/trackselect.h"
#include "common/vehiclecount.h"
#include "common/weather.h"

#if TGR_PLATFORM == TGR_PLATFORM_N64
#include <PR/os_libc.h>
#define PROGRESS_PROFILE (g_aVehicles[0].profile)
#define PROGRESS_MESSAGE(text, id) (text)
#define PROGRESS_ZERO(value) ((value) == 0.0f)
#define PROGRESS_LESS(a, b) ((a) < (b))
#else
#include "common/screen.h"
#define osSyncPrintf(...) NullStub()
#define PROGRESS_PROFILE VEHICLE_GET(&g_aVehicles[0], Profile)
#define PROGRESS_MESSAGE(text, id) GetRaceProgressMessage(id)
#define PROGRESS_ZERO(value) (!((value) < 0.0f) && !((value) > 0.0f))
#define PROGRESS_LESS(a, b) (!((a) >= (b)))
#endif /* TGR_PLATFORM == TGR_PLATFORM_N64 */

/* The message durations of a lap, in seconds. */
#define LAP_MESSAGE_TIME 1.0f
#define FINISH_MESSAGE_TIME 5.0f
#define RECORD_MESSAGE_TIME 1.5f
#define RECORD_SUB_MESSAGE_TIME 3.5f
#define CHECKPOINT_MESSAGE_TIME 0.4f
#define CHECKPOINT_SUB_MESSAGE_TIME 0.9f
#define FINISH_OPACITY 0.9f

/* Save a course record only for races of this many laps. */
#define RECORD_LAPS 3

/* Lap times use hundredths of a second. */
#define LAP_TIME_SCALE 100.0f

/* How far behind a player is, from the distance between the first two vehicles at their pace;
 * 1000 seconds with no pace, and way behind past 120. */
#define PACE_SCALE 2.24f
#define NO_PACE 1000.0f
#define WAY_BEHIND 120.0f

/* The length of the color prefix of a message. */
#define COLOR_PREFIX_LENGTH 3

/* Whether a segment crosses a gate, seen from above. The original hands the positions themselves
 * to the flat test, which reads only x and y. */
#define CROSSES_GATE(gate, from, to)                                                               \
    TestLineSegmentIntersection(&(gate)->ends[1], &(gate)->ends[0], (Vec2f *)(from), (Vec2f *)(to))

/* Set weather to the clamped index of the weather of the arcade times. */
#define ARCADE_WEATHER(weather)                                                                    \
    weather = g_dwCurrentWeather - 1;                                                              \
    if ((weather) >= ARCADE_WEATHERS || (weather) < 0) {                                           \
        weather = 0;                                                                               \
    }

/* The arcade times of the selected track. The descriptor of a track starts with its menu item. */
#if TGR_BOSS_RALLY
#define SELECTED_TRACK_ITEM() (g_apTrackMenuItems[g_dwSelectedTrackId])
#else
#define SELECTED_TRACK_ITEM() ((TrackMenuItem *)g_apTrackMenuItems[g_dwSelectedTrackId])
#endif

/* The gate a gate count is at, within a lap. */
static BOSS_RALLY_INLINE s32 GateInLap(s32 gate, s32 count) {
    if (gate < 0) {
        return (count - (-1 - gate) % count - 1) % count;
    }
    return gate % count;
}

/* Practice returns a racer to lap 0 after the first lap. */
static BOSS_RALLY_INLINE void LapBack(RacerSlot *slot, const char *format) {
    osSyncPrintf(format, slot->index);
    RACER_SLOT_SET(slot, Lap, RACER_SLOT_GET(slot, Lap) - 1);
    RACER_SLOT_SET(slot, DistanceLaps, RACER_SLOT_GET(slot, DistanceLaps) - 1);
    RACER_SLOT_SET(slot, MaxGate, RACER_SLOT_GET(slot, MaxGate) - (g_LoadedTrack.gateCount));
    RACER_SLOT_SET(slot, Gate, RACER_SLOT_GET(slot, Gate) - (g_LoadedTrack.gateCount));
    if (g_LoadedTrack.rootSection != NULL) {
        RACER_SLOT_SET(slot,
                       Distance,
                       RACER_SLOT_GET(slot, Distance) -
                           (g_LoadedTrack.rootSection->points[0].distance));
    }
}

/* Finish the race of a racer, with its place and a course record of a player. */
static BOSS_RALLY_INLINE void FinishRacer(RacerSlot *slot, Vehicle *vehicle) {
    f32 *record;

    RACER_SLOT_SET(slot, Status, RACER_SLOT_GET(slot, Status) | (RACER_FINISHED));
    if (vehicle != NULL) {
        VEHICLE_SET(vehicle, Opacity, FINISH_OPACITY);
        VEHICLE_SET(
            vehicle, TotalTime, VEHICLE_GET(vehicle, TotalTime) - (RACER_SLOT_GET(slot, LapTime)));
        RACER_SLOT_SET(slot, LapTime, 0.0f);
        VEHICLE_SET(vehicle, RacePosition, g_dwRaceFinishOrderCounter);
#if TGR_BOSS_RALLY
        VEHICLE_SET(
            vehicle,
            Message,
            g_dwGameMode == GAME_MODE_NETWORK ?
                GetNetworkRaceFinishMessage() :
                GetRaceProgressMessage(g_anRaceProgressStringIds[g_dwRaceFinishOrderCounter]));
#else
        vehicle->message = g_apszRaceProgressStrings[g_dwRaceFinishOrderCounter];
#endif /* TGR_BOSS_RALLY */
        VEHICLE_SET(vehicle, MessageTime, FINISH_MESSAGE_TIME);
        if (RACER_SLOT_GET(slot, Index) < (s32)g_dwNumLocalPlayers &&
            g_dwTotalLaps == RECORD_LAPS) {
            record = &PROGRESS_PROFILE->raceRecords[g_dwSelectedTrackId];
            if (PROGRESS_ZERO(*record) || PROGRESS_LESS(VEHICLE_GET(vehicle, TotalTime), *record)) {
                *record = VEHICLE_GET(vehicle, TotalTime);
                VEHICLE_SET(vehicle, SubMessage, PROGRESS_MESSAGE("%ryNEW COURSE RECORD!", 0x121));
                /* The message, not the record, stays longer. */
                VEHICLE_SET(vehicle, MessageTime, RECORD_MESSAGE_TIME);
                VEHICLE_SET(vehicle, SubMessageTime, RECORD_SUB_MESSAGE_TIME);
            }
        }
    }
    g_dwRaceFinishOrderCounter++;
}

/* Add the time of an arcade checkpoint to the players, and show it with how far behind they are. */
static BOSS_RALLY_INLINE void PassArcadeCheckpoint(RacerSlot *slot, s32 gate, f32 gapFallback) {
    Vehicle *other;
#if TGR_BOSS_RALLY
    long double gap;
    long double speed;
    s32 bestPosition;
#else
    f32 gap;
#endif
    f32 pace;
    s32 i;
    s16 weather;

    if (!gInstantReplayActive) {
        PlayBeepSound();
    }
#if TGR_BOSS_RALLY
    if (g_dwGameMode == GAME_MODE_ARCADE) {
        gap = (long double)VEHICLE_GET(&(g_aVehicles[0]), Distance) -
              VEHICLE_GET(&(g_aVehicles[1]), Distance);
    } else {
        gap = gapFallback;
        bestPosition = 255;
        for (i = 1; i < g_dwNumActiveVehicles; i++) {
            if (VEHICLE_GET(&(g_aVehicles[i]), RacePosition) < bestPosition) {
                gap = (long double)VEHICLE_GET(&(g_aVehicles[0]), Distance) -
                      VEHICLE_GET(&(g_aVehicles[i]), Distance);
                bestPosition = VEHICLE_GET(&(g_aVehicles[i]), RacePosition);
            }
        }
    }
#else
    gap = g_aVehicles[0].distance - g_aVehicles[1].distance;
#endif /* TGR_BOSS_RALLY */
    if (PROGRESS_LESS(gap, 0.0f)) {
        gap = -gap;
    }
#if TGR_BOSS_RALLY
    speed = (long double)VEHICLE_GET(RACER_SLOT_GET(slot, Vehicle), SpeedKph) * 0.44642857f;
    if (PROGRESS_ZERO(speed)) {
        pace = NO_PACE;
    } else {
        if (PROGRESS_LESS(speed, 25.0f)) {
            speed = 25.0f;
        }
        pace = gap / speed;
    }
#else
    pace = slot->vehicle->odometer / (slot->vehicle->totalTime * PACE_SCALE);
    if (pace == 0.0f) {
        pace = NO_PACE;
    } else {
        pace = gap / pace;
    }
#endif /* TGR_BOSS_RALLY */
    for (i = 0; i < (s32)g_dwNumLocalPlayers; i++) {
        other = RACER_SLOT_GET(&(g_aRacerSlots[i]), Vehicle);
#if TGR_BOSS_RALLY
        if (VEHICLE_GET(other, Message) == VEHICLE_GET(other, MessageText)) {
            strcpy(VEHICLE_GET(other, CheckpointMessageText), VEHICLE_GET(other, MessageText));
            VEHICLE_SET(other, Message, VEHICLE_GET(other, CheckpointMessageText));
        } else
#endif
        {
            VEHICLE_SET(other, Message, PROGRESS_MESSAGE("%ryCheck Point!", 0x122));
            VEHICLE_SET(other, MessageTime, CHECKPOINT_MESSAGE_TIME);
        }
        ARCADE_WEATHER(weather);
        VEHICLE_ADD(other,
                    TimeLeft,
                    SELECTED_TRACK_ITEM()
                        ->arcadeTimes[VEHICLE_GET(&(g_aVehicles[0]), TableIndex)][weather]
                        .gateBonuses[gate]);
#if TGR_PLATFORM != TGR_PLATFORM_WIN
        if (g_dwNumLocalPlayers == 1) {
            sprintf(VEHICLE_GET(other, MessageText), "%%ry");
            FormatLapTime(VEHICLE_GET(other, MessageText) + COLOR_PREFIX_LENGTH,
                          VEHICLE_GET(other, TimeLeft));
            sprintf(VEHICLE_GET(other, MessageText) + strlen(VEHICLE_GET(other, MessageText)),
                    " Left");
            VEHICLE_SET(other, SubMessage, VEHICLE_GET(other, MessageText));
            VEHICLE_SET(other, SubMessageTime, CHECKPOINT_SUB_MESSAGE_TIME);
        } else
#endif
            if (!PROGRESS_ZERO(VEHICLE_GET(other, TotalTime))
#if TGR_BOSS_RALLY
                && g_dwNumActiveVehicles > 1
#endif
            ) {
            sprintf(VEHICLE_GET(other, MessageText), "%%ry");
            FormatLapTime(VEHICLE_GET(other, MessageText) + COLOR_PREFIX_LENGTH, pace);
            if (VEHICLE_GET(other, RacePosition) != 0) {
                /* Overwritten below. */
                if (WAY_BEHIND < pace) {
                    VEHICLE_SET(
                        other, SubMessage, PROGRESS_MESSAGE("%ryYou're way behind!", 0x123));
                }
#ifdef ENABLE_PATCHES
                sprintf(VEHICLE_GET(other, MessageText) + strlen(VEHICLE_GET(other, MessageText)),
                        "%s",
                        PROGRESS_MESSAGE(" Behind", 0x124));
#else
                /* The original treats the localised suffix as a format string. */
                sprintf(VEHICLE_GET(other, MessageText) + strlen(VEHICLE_GET(other, MessageText)),
                        PROGRESS_MESSAGE(" Behind", 0x124));
#endif /* ENABLE_PATCHES */
#if TGR_PLATFORM == TGR_PLATFORM_N64
            } else {
                sprintf(other->messageText + strlen(other->messageText), " Ahead");
#endif
            }
            VEHICLE_SET(other, SubMessage, VEHICLE_GET(other, MessageText));
            VEHICLE_SET(other, SubMessageTime, CHECKPOINT_SUB_MESSAGE_TIME);
        }
        VEHICLE_SET(other, MaxGate, RACER_SLOT_GET(slot, MaxGate));
    }
}

/* A racer crossed the next gate for the first time. */
static BOSS_RALLY_INLINE void
ReachNewGate(RacerSlot *slot, Vehicle *vehicle, s32 next, f32 lapTime, f32 gapFallback) {
    const char *message;
    f32 *record;

    RACER_SLOT_SET(slot, MaxGate, RACER_SLOT_GET(slot, Gate));
    osSyncPrintf("moved ahead one gate, getting %f seconds\n",
                 (double)g_LoadedTrack.gates[next].seconds);
    if (next == 0) {
        if (RACER_SLOT_GET(slot, Lap) < g_dwTotalLaps) {
            /* Retain the best lap, and choose the message of a record of a player. */
            message = NULL;
            if (PROGRESS_ZERO(RACER_SLOT_GET(slot, BestLapTime)) ||
                PROGRESS_LESS(lapTime, RACER_SLOT_GET(slot, BestLapTime))) {
                RACER_SLOT_SET(slot, BestLapTime, lapTime);
                if (RACER_SLOT_GET(slot, Index) < (s32)g_dwNumLocalPlayers) {
                    /* A player's racer always has its vehicle. */
                    VEHICLE_SET(RACER_SLOT_GET(slot, Vehicle), BestLap, RACER_SLOT_GET(slot, Lap));
                    record = &PROGRESS_PROFILE->lapRecords[g_dwSelectedTrackId];
                    if (PROGRESS_ZERO(*record) ||
                        PROGRESS_LESS(RACER_SLOT_GET(slot, BestLapTime), *record)) {
                        /* The record is read back from the racer here, not from the lap. */
                        *record = RACER_SLOT_GET(slot, BestLapTime);
                        message = PROGRESS_MESSAGE("%ryNEW LAP RECORD!", 0x109);
                    } else if (RACER_SLOT_GET(slot, Lap) != 0) {
                        message = PROGRESS_MESSAGE("%ryBEST LAP!", 0x10a);
                    }
                }
            }
            if (vehicle != NULL) {
                osSyncPrintf("veh->lapTimeFinal[%d]=%f\n", slot->lap, (double)lapTime);
                VEHICLE_GET(vehicle, LapTimes)[RACER_SLOT_GET(slot, Lap)] = lapTime;
            }
            RACER_SLOT_SET(slot, Lap, RACER_SLOT_GET(slot, Lap) + 1);
            RACER_SLOT_SET(slot, DistanceLaps, RACER_SLOT_GET(slot, Lap));
            RACER_SLOT_SET(slot, LapTime, RACER_SLOT_GET(slot, LapTime) - (lapTime));
            if (g_dwGameMode == GAME_MODE_PRACTICE) {
                if (RACER_SLOT_GET(slot, Lap) == 1) {
                    LapBack(slot, "lapping %d back to 0 (a)\n");
                }
            } else if (vehicle != NULL) {
                /* Show the lap the racer starts and its record. */
                osSyncPrintf("******* LAP #%d ********\n", slot->lap);
#if TGR_BOSS_RALLY
                if (RACER_SLOT_GET(slot, Lap) == g_dwTotalLaps - 1) {
                    strcpy(VEHICLE_GET(vehicle, MessageText), GetRaceProgressMessage(0x10b));
                } else
#endif
                {
                    sprintf(VEHICLE_GET(vehicle, MessageText),
                            PROGRESS_MESSAGE("%%ryLAP %d", 0x10c),
                            RACER_SLOT_GET(slot, Lap) + 1);
                }
                VEHICLE_SET(vehicle, Message, VEHICLE_GET(vehicle, MessageText));
                VEHICLE_SET(vehicle, MessageTime, LAP_MESSAGE_TIME);
                if (message != NULL) {
                    VEHICLE_SET(vehicle, SubMessage, message);
                    VEHICLE_SET(vehicle, SubMessageTime, LAP_MESSAGE_TIME);
                }
            }
        }
        if (g_dwTotalLaps == RACER_SLOT_GET(slot, Lap)) {
            FinishRacer(slot, vehicle);
            return;
        }
    }
#if TGR_BOSS_RALLY && TGR_PLATFORM == TGR_PLATFORM_SDL
    /* The SDL game has the Time Attack of the N64, which passes no checkpoint. */
    if ((g_dwGameMode == GAME_MODE_ARCADE || g_dwGameMode == GAME_MODE_NETWORK) &&
        RACER_SLOT_GET(slot, Index) < (s32)g_dwNumLocalPlayers) {
#elif TGR_BOSS_RALLY
    if ((g_dwGameMode == GAME_MODE_ARCADE || g_dwGameMode == GAME_MODE_NETWORK ||
         g_dwGameMode == GAME_MODE_TIME_ATTACK) &&
        RACER_SLOT_GET(slot, Index) < (s32)g_dwNumLocalPlayers) {
#else
    if (g_dwGameMode == GAME_MODE_ARCADE && slot->index < (s32)g_dwNumLocalPlayers &&
        0.0f < slot->vehicle->timeLeft) {
#endif
        PassArcadeCheckpoint(slot, next, gapFallback);
    }
}

/* A racer crossed a previously counted gate; the line of the lap still counts the lap. */
static BOSS_RALLY_INLINE void
ReachGateAgain(RacerSlot *slot, Vehicle *vehicle, TrackGate *gate, f32 lapTime) {
    const char *message;
    f32 *record;

    if (gate != &g_LoadedTrack.gates[0]) {
        return;
    }
    RACER_SLOT_SET(slot, DistanceLaps, RACER_SLOT_GET(slot, DistanceLaps) + 1);
    if (RACER_SLOT_GET(slot, Lap) < RACER_SLOT_GET(slot, DistanceLaps)) {
        /* Retain the best lap, and choose the message of a record of a player. */
        message = NULL;
        if (PROGRESS_ZERO(RACER_SLOT_GET(slot, BestLapTime)) ||
            PROGRESS_LESS(lapTime, RACER_SLOT_GET(slot, BestLapTime))) {
            RACER_SLOT_SET(slot, BestLapTime, lapTime);
            if (RACER_SLOT_GET(slot, Index) < (s32)g_dwNumLocalPlayers) {
                /* A player's racer always has its vehicle. */
                VEHICLE_SET(RACER_SLOT_GET(slot, Vehicle), BestLap, RACER_SLOT_GET(slot, Lap));
                record = &PROGRESS_PROFILE->lapRecords[g_dwSelectedTrackId];
                if (PROGRESS_ZERO(*record) ||
                    PROGRESS_LESS(RACER_SLOT_GET(slot, BestLapTime), *record)) {
                    *record = lapTime;
                    message = PROGRESS_MESSAGE("%ryNEW LAP RECORD!", 0x109);
                } else if (RACER_SLOT_GET(slot, Lap) != 0) {
                    message = PROGRESS_MESSAGE("%ryBEST LAP!", 0x10a);
                }
            }
        }
        if (vehicle != NULL) {
            osSyncPrintf("veh->lapTimeFinal[%d]=%f\n", slot->lap, (double)lapTime);
            VEHICLE_GET(vehicle, LapTimes)[RACER_SLOT_GET(slot, Lap)] = lapTime;
        }
        RACER_SLOT_SET(slot, Lap, RACER_SLOT_GET(slot, DistanceLaps));
        RACER_SLOT_SET(slot, LapTime, RACER_SLOT_GET(slot, LapTime) - (lapTime));
        if (vehicle != NULL) {
            /* Show the lap the racer starts and its record. */
            osSyncPrintf("******* LAP #%d ********\n", slot->lap);
            sprintf(VEHICLE_GET(vehicle, MessageText),
                    PROGRESS_MESSAGE("%%ryLAP %d", 0x10c),
                    RACER_SLOT_GET(slot, Lap) + 1);
            VEHICLE_SET(vehicle, Message, VEHICLE_GET(vehicle, MessageText));
            VEHICLE_SET(vehicle, MessageTime, LAP_MESSAGE_TIME);
            if (message != NULL) {
                VEHICLE_SET(vehicle, SubMessage, message);
                VEHICLE_SET(vehicle, SubMessageTime, LAP_MESSAGE_TIME);
            }
        }
    }
    osSyncPrintf("granting technically-earned lap %d/%d to %d\n",
                 slot->distanceLaps,
                 slot->lap,
                 slot->index);
    if (g_dwGameMode == GAME_MODE_PRACTICE && RACER_SLOT_GET(slot, Lap) == 1) {
        LapBack(slot, "lapping %d back to 0 (b)\n");
    }
}

void UpdateRaceProgress(RacerSlot *slot) {
    Vehicle *vehicle = RACER_SLOT_GET(slot, Vehicle);
    TrackGate *gate;
    s32 current;
    s32 next;
    f32 lapTime;
#if TGR_BOSS_RALLY
    union {
        s32 hundredths;
        f32 gapFallback;
    } timer;
#endif

    /* The count of the gates is read from the track at each use. */
    if (g_LoadedTrack.gateCount == 0) {
        return;
    }
    if (vehicle != NULL) {
        RACER_SLOT_GET(slot, Position) = VEHICLE_GET(vehicle, Frame).position;
        RACER_SLOT_GET(slot, PreviousPosition) = VEHICLE_GET(vehicle, PreviousPosition);
        RACER_SLOT_SET(slot, MaxGate, VEHICLE_GET(vehicle, MaxGate));
        RACER_SLOT_SET(slot, Gate, VEHICLE_GET(vehicle, Gate));
        RACER_SLOT_SET(slot, Lap, VEHICLE_GET(vehicle, Lap));
        RACER_SLOT_SET(slot, DistanceLaps, VEHICLE_GET(vehicle, DistanceLaps));
        RACER_SLOT_SET(slot, LapTime, VEHICLE_GET(vehicle, LapTime));
        RACER_SLOT_SET(slot, BestLapTime, VEHICLE_GET(vehicle, BestLapTime));
        RACER_SLOT_SET(slot, Distance, VEHICLE_GET(vehicle, Distance));
    }
    current = GateInLap(RACER_SLOT_GET(slot, Gate), g_LoadedTrack.gateCount);
    next = (current + 1) % g_LoadedTrack.gateCount;
#if TGR_BOSS_RALLY
    timer.hundredths = FloatToInt(RACER_SLOT_GET(slot, LapTime) * LAP_TIME_SCALE);
    lapTime = (long double)timer.hundredths * 0.01f;
#else
    lapTime = (f32)FloatToInt(slot->lapTime * LAP_TIME_SCALE) / LAP_TIME_SCALE;
#endif

    if (CROSSES_GATE(&g_LoadedTrack.gates[current],
                     &RACER_SLOT_GET(slot, PreviousPosition),
                     &RACER_SLOT_GET(slot, Position))) {
        osSyncPrintf("%d went backwards, from gate %d to gate %d\n",
                     slot->index,
                     slot->gate,
                     slot->gate - 1);
        if (current == 0) {
            RACER_SLOT_SET(slot, DistanceLaps, RACER_SLOT_GET(slot, DistanceLaps) - 1);
        }
        osSyncPrintf("Hmm %d == %d %% %d && %d < 0 ? %d\n",
                     current,
                     slot->maxGate,
                     g_LoadedTrack.gateCount,
                     slot->gate,
                     slot->maxGate % g_LoadedTrack.gateCount ==
                             (slot->gate + 2 * g_LoadedTrack.gateCount) % g_LoadedTrack.gateCount ?
                         slot->gate < 0 :
                         0);
        if (current == RACER_SLOT_GET(slot, MaxGate) % g_LoadedTrack.gateCount &&
            RACER_SLOT_GET(slot, Gate) < 0) {
            RACER_SLOT_SET(slot, Gate, RACER_SLOT_GET(slot, Gate) + (g_LoadedTrack.gateCount));
            RACER_SLOT_SET(slot, DistanceLaps, RACER_SLOT_GET(slot, DistanceLaps) + 1);
            if (g_LoadedTrack.rootSection != NULL) {
                RACER_SLOT_SET(slot,
                               Distance,
                               RACER_SLOT_GET(slot, Distance) +
                                   (g_LoadedTrack.rootSection->points[0].distance));
            }
            osSyncPrintf("lapping %d forward to -1\n", slot->index);
        }
        RACER_SLOT_SET(slot, Gate, RACER_SLOT_GET(slot, Gate) - 1);
    } else {
        gate = &g_LoadedTrack.gates[next];
        if (CROSSES_GATE(
                gate, &RACER_SLOT_GET(slot, PreviousPosition), &RACER_SLOT_GET(slot, Position))) {
            osSyncPrintf("%d went forward, from gate %d to gate %d\n",
                         slot->index,
                         slot->gate,
                         slot->gate + 1);
            RACER_SLOT_SET(slot, Gate, RACER_SLOT_GET(slot, Gate) + 1);
            if (RACER_SLOT_GET(slot, MaxGate) < RACER_SLOT_GET(slot, Gate)) {
                ReachNewGate(
                    slot,
                    vehicle,
                    next,
                    lapTime,
#if TGR_BOSS_RALLY
                    /* With one active vehicle, the original reuses the integer timer bits. */
                    timer.gapFallback
#else
                    0.0f
#endif
                );
            } else {
                ReachGateAgain(slot, vehicle, gate, lapTime);
            }
        }
    }

    if (vehicle != NULL) {
        VEHICLE_SET(vehicle, MaxGate, RACER_SLOT_GET(slot, MaxGate));
        VEHICLE_SET(vehicle, Gate, RACER_SLOT_GET(slot, Gate));
        VEHICLE_SET(vehicle, Lap, RACER_SLOT_GET(slot, Lap));
        VEHICLE_SET(vehicle, DistanceLaps, RACER_SLOT_GET(slot, DistanceLaps));
        VEHICLE_SET(vehicle, LapTime, RACER_SLOT_GET(slot, LapTime));
        VEHICLE_SET(vehicle, BestLapTime, RACER_SLOT_GET(slot, BestLapTime));
        VEHICLE_SET(vehicle, Distance, RACER_SLOT_GET(slot, Distance));
    }
}
