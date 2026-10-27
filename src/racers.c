#include "racers.h"

#include <PR/os.h>
#include <PR/os_libc.h>

#include "audio.h"
#include "camera.h"
#include "carselect.h"
#include "game.h"
#include "input.h"
#include "mathutil.h"
#include "race.h"
#include "racemenu.h"
#include "racersharing.h"
#include "render.h"
#include "system.h"
#include "trackselect.h"
#include "util.h"
#include "vehiclerender.h"
#include "weather.h"

/* The racers after the players that are not stragglers, and where the racers without vehicles
 * start: stragglers 600 apart from 7760 behind, the others 550 apart, or 520 on the coastline. */
#define LEADING_RACERS 13
#define STRAGGLER_SPACING 600
#define STRAGGLER_START 7760
#define RACER_SPACING 550
#define NARROW_RACER_SPACING 520

/* The arcade gives two racers vehicles; the kinds of racer share vehicles separately. */
#define ARCADE_VEHICLES 2

/* The messages of a lap: how long each stays, in seconds. */
#define LAP_MESSAGE_TIME 1.0f
#define FINISH_MESSAGE_TIME 5.0f
#define RECORD_MESSAGE_TIME 1.5f
#define RECORD_SUB_MESSAGE_TIME 3.5f
#define CHECKPOINT_MESSAGE_TIME 0.4f
#define CHECKPOINT_SUB_MESSAGE_TIME 0.9f
#define FINISH_OPACITY 0.9f

/* A race of this many laps keeps a course record. */
#define RECORD_LAPS 3

/* A lap time is kept in hundredths of a second. */
#define LAP_TIME_SCALE 100.0f

/* How far behind a player is, from the distance between the first two vehicles at their pace;
 * 1000 seconds with no pace, and way behind past 120. */
#define PACE_SCALE 2.24f
#define NO_PACE 1000.0f
#define WAY_BEHIND 120.0f

/* The length of the color prefix of a message. */
#define COLOR_PREFIX_LENGTH 3

/* A ghost fades in this fast, up to this in time attack; a racer without a vehicle moves this far
 * a frame when no vehicle of its kind sets the pace. */
#define GHOST_FADE_RATE 1.6f
#define TIME_ATTACK_GHOST_OPACITY 0.375f
#define IDLE_RACER_STEP 2.22f

/* The steering a finished player's vehicle holds. */
#define FINISHED_STEERING_INPUT -127

/* Whether a segment crosses a gate, seen from above. The original hands the positions themselves
 * to the flat test, which reads only x and y. */
#define CROSSES_GATE(gate, from, to)                                                               \
    TestLineSegmentIntersection(&(gate)->ends[1], &(gate)->ends[0], (Vec2f *)(from), (Vec2f *)(to))

/* Count the gate after the ones counted if the segment crosses it, and a lap at the first. */
#define COUNT_ROUTE_GATE(next, from, to)                                                           \
    if (g_LoadedTrack.gateCount) {                                                                 \
        next = (g_dwTrackRouteGates + 1) % g_LoadedTrack.gateCount;                                \
        if (CROSSES_GATE(&g_LoadedTrack.gates[next], from, to)) {                                  \
            g_dwTrackRouteGates++;                                                                 \
            if (&g_LoadedTrack.gates[next] == g_LoadedTrack.gates) {                               \
                g_dwTrackRouteLaps++;                                                              \
            }                                                                                      \
        }                                                                                          \
    }

/* Skip the alternate branches from a section on. */
#define SKIP_ALTERNATE_SECTIONS(section)                                                           \
    while ((section) != NULL && ((section)->flags & TRACK_SECTION_ALTERNATE)) {                    \
        section = (section)->next;                                                                 \
    }

/* Set weather to the clamped index of the weather of the arcade times. */
#define ARCADE_WEATHER(weather)                                                                    \
    weather = g_dwCurrentWeather - 1;                                                              \
    if ((weather) >= ARCADE_WEATHERS || (weather) < 0) {                                           \
        weather = 0;                                                                               \
    }

/* The arcade times of the selected track. The descriptor of a track starts with its menu item. */
#define SELECTED_TRACK_ITEM() ((TrackMenuItem *)g_apTrackMenuItems[g_dwSelectedTrackId])

void SortRacersByPosition(void) {
    RacerProgress entries[RACER_SLOTS];
    RacerSlot *slot;
    s32 count = 0;
    s32 i;

    for (i = 0; i < g_dwNumVehicles; i++) {
        slot = &g_aRacerSlots[i];
        if (slot->status & RACER_FINISHED) {
            continue;
        }
        entries[count].slot = i;
        entries[count].progress = slot->vehicle != NULL ? slot->vehicle->distance : slot->distance;
        count++;
    }
    if (count) {
        QuickSort(entries, count, sizeof(RacerProgress), CompareRacerProgressAsc);
    }
    /* The finished racers keep the leading places. */
    for (i = 0; i < count; i++) {
        slot = &g_aRacerSlots[entries[i].slot];
        if (slot->vehicle != NULL) {
            slot->vehicle->racePosition = g_dwNumVehicles - i - 1;
        } else {
            slot->racePosition = g_dwNumVehicles - i - 1;
        }
    }
}

/* The gate a gate count is at, within a lap. */
static s32 GateInLap(s32 gate, s32 count) {
    if (gate < 0) {
        return (count - (-1 - gate) % count - 1) % count;
    }
    return gate % count;
}

/* Practice counts no laps: a racer that finished the first goes back to lap 0. */
static void LapBack(RacerSlot *slot, const char *format) {
    osSyncPrintf(format, slot->index);
    slot->lap--;
    slot->distanceLaps--;
    slot->maxGate -= g_LoadedTrack.gateCount;
    slot->gate -= g_LoadedTrack.gateCount;
    if (g_LoadedTrack.rootSection != NULL) {
        slot->distance -= g_LoadedTrack.rootSection->points[0].distance;
    }
}

/* Finish the race of a racer, with its place and a course record of a player. */
static void FinishRacer(RacerSlot *slot, Vehicle *vehicle) {
    f32 *record;

    slot->status |= RACER_FINISHED;
    if (vehicle != NULL) {
        vehicle->opacity = FINISH_OPACITY;
        vehicle->totalTime -= slot->lapTime;
        slot->lapTime = 0.0f;
        vehicle->racePosition = g_dwRaceFinishOrderCounter;
        vehicle->message = g_apszRaceProgressStrings[g_dwRaceFinishOrderCounter];
        vehicle->messageTime = FINISH_MESSAGE_TIME;
        if (slot->index < (s32)g_dwNumLocalPlayers && g_dwTotalLaps == RECORD_LAPS) {
            record = &g_pActiveProfile->raceRecords[g_dwSelectedTrackId];
            if (*record == 0.0f || vehicle->totalTime < *record) {
                *record = vehicle->totalTime;
                vehicle->subMessage = "%ryNEW COURSE RECORD!";
                /* The message, not the record, stays longer. */
                vehicle->messageTime = RECORD_MESSAGE_TIME;
                vehicle->subMessageTime = RECORD_SUB_MESSAGE_TIME;
            }
        }
    }
    g_dwRaceFinishOrderCounter++;
}

/* Add the time of an arcade checkpoint to the players, and show it with how far behind they are. */
static void PassArcadeCheckpoint(RacerSlot *slot, s32 gate) {
    Vehicle *other;
    f32 gap;
    f32 pace;
    s32 i;
    s16 weather;

    if (!gInstantReplayActive) {
        PlayBeepSound();
    }
    gap = g_aVehicles[0].distance - g_aVehicles[1].distance;
    if (gap < 0.0f) {
        gap = -gap;
    }
    pace = slot->vehicle->odometer / (slot->vehicle->totalTime * PACE_SCALE);
    if (pace == 0.0f) {
        pace = NO_PACE;
    } else {
        pace = gap / pace;
    }
    for (i = 0; i < (s32)g_dwNumLocalPlayers; i++) {
        other = g_aRacerSlots[i].vehicle;
        other->message = "%ryCheck Point!";
        other->messageTime = CHECKPOINT_MESSAGE_TIME;
        ARCADE_WEATHER(weather);
        other->timeLeft += SELECTED_TRACK_ITEM()
                               ->arcadeTimes[g_aVehicles[0].tableIndex][weather]
                               .gateBonuses[gate];
        if (g_dwNumLocalPlayers == 1) {
            sprintf(other->messageText, "%%ry");
            FormatLapTime(other->messageText + COLOR_PREFIX_LENGTH, other->timeLeft);
            sprintf(other->messageText + strlen(other->messageText), " Left");
            other->subMessage = other->messageText;
            other->subMessageTime = CHECKPOINT_SUB_MESSAGE_TIME;
        } else if (other->totalTime != 0.0f) {
            sprintf(other->messageText, "%%ry");
            FormatLapTime(other->messageText + COLOR_PREFIX_LENGTH, pace);
            if (other->racePosition != 0) {
                /* Overwritten below. */
                if (WAY_BEHIND < pace) {
                    other->subMessage = "%ryYou're way behind!";
                }
                sprintf(other->messageText + strlen(other->messageText), " Behind");
            } else {
                sprintf(other->messageText + strlen(other->messageText), " Ahead");
            }
            other->subMessage = other->messageText;
            other->subMessageTime = CHECKPOINT_SUB_MESSAGE_TIME;
        }
        other->maxGate = slot->maxGate;
    }
}

/* A racer crossed the next gate for the first time. */
static void ReachNewGate(RacerSlot *slot, Vehicle *vehicle, s32 next, f32 lapTime) {
    const char *message;
    f32 *record;

    slot->maxGate = slot->gate;
    osSyncPrintf("moved ahead one gate, getting %f seconds\n",
                 (double)g_LoadedTrack.gates[next].seconds);
    if (next == 0) {
        if (slot->lap < g_dwTotalLaps) {
            /* Retain the best lap, and choose the message of a record of a player. */
            message = NULL;
            if (slot->bestLapTime == 0.0f || lapTime < slot->bestLapTime) {
                slot->bestLapTime = lapTime;
                if (slot->index < (s32)g_dwNumLocalPlayers) {
                    /* A player's racer always has its vehicle. */
                    slot->vehicle->bestLap = slot->lap;
                    record = &g_pActiveProfile->lapRecords[g_dwSelectedTrackId];
                    if (*record == 0.0f || slot->bestLapTime < *record) {
                        /* The record is read back from the racer here, not from the lap. */
                        *record = slot->bestLapTime;
                        message = "%ryNEW LAP RECORD!";
                    } else if (slot->lap != 0) {
                        message = "%ryBEST LAP!";
                    }
                }
            }
            if (vehicle != NULL) {
                osSyncPrintf("veh->lapTimeFinal[%d]=%f\n", slot->lap, (double)lapTime);
                vehicle->lapTimes[slot->lap] = lapTime;
            }
            slot->lap++;
            slot->distanceLaps = slot->lap;
            slot->lapTime -= lapTime;
            if (g_dwGameMode == GAME_MODE_PRACTICE) {
                if (slot->lap == 1) {
                    LapBack(slot, "lapping %d back to 0 (a)\n");
                }
            } else if (vehicle != NULL) {
                /* Show the lap the racer starts and its record. */
                osSyncPrintf("******* LAP #%d ********\n", slot->lap);
                sprintf(vehicle->messageText, "%%ryLAP %d", slot->lap + 1);
                vehicle->message = vehicle->messageText;
                vehicle->messageTime = LAP_MESSAGE_TIME;
                if (message != NULL) {
                    vehicle->subMessage = message;
                    vehicle->subMessageTime = LAP_MESSAGE_TIME;
                }
            }
        }
        if (g_dwTotalLaps == slot->lap) {
            FinishRacer(slot, vehicle);
            return;
        }
    }
    if (g_dwGameMode == GAME_MODE_ARCADE && slot->index < (s32)g_dwNumLocalPlayers &&
        0.0f < slot->vehicle->timeLeft) {
        PassArcadeCheckpoint(slot, next);
    }
}

/* A racer crossed a gate it had reached before; the line of the lap still counts the lap. */
static void ReachGateAgain(RacerSlot *slot, Vehicle *vehicle, TrackGate *gate, f32 lapTime) {
    const char *message;
    f32 *record;

    if (gate != &g_LoadedTrack.gates[0]) {
        return;
    }
    slot->distanceLaps++;
    if (slot->lap < slot->distanceLaps) {
        /* Retain the best lap, and choose the message of a record of a player. */
        message = NULL;
        if (slot->bestLapTime == 0.0f || lapTime < slot->bestLapTime) {
            slot->bestLapTime = lapTime;
            if (slot->index < (s32)g_dwNumLocalPlayers) {
                /* A player's racer always has its vehicle. */
                slot->vehicle->bestLap = slot->lap;
                record = &g_pActiveProfile->lapRecords[g_dwSelectedTrackId];
                if (*record == 0.0f || slot->bestLapTime < *record) {
                    *record = lapTime;
                    message = "%ryNEW LAP RECORD!";
                } else if (slot->lap != 0) {
                    message = "%ryBEST LAP!";
                }
            }
        }
        if (vehicle != NULL) {
            osSyncPrintf("veh->lapTimeFinal[%d]=%f\n", slot->lap, (double)lapTime);
            vehicle->lapTimes[slot->lap] = lapTime;
        }
        slot->lap = slot->distanceLaps;
        slot->lapTime -= lapTime;
        if (vehicle != NULL) {
            /* Show the lap the racer starts and its record. */
            osSyncPrintf("******* LAP #%d ********\n", slot->lap);
            sprintf(vehicle->messageText, "%%ryLAP %d", slot->lap + 1);
            vehicle->message = vehicle->messageText;
            vehicle->messageTime = LAP_MESSAGE_TIME;
            if (message != NULL) {
                vehicle->subMessage = message;
                vehicle->subMessageTime = LAP_MESSAGE_TIME;
            }
        }
    }
    osSyncPrintf("granting technically-earned lap %d/%d to %d\n",
                 slot->distanceLaps,
                 slot->lap,
                 slot->index);
    if (g_dwGameMode == GAME_MODE_PRACTICE && slot->lap == 1) {
        LapBack(slot, "lapping %d back to 0 (b)\n");
    }
}

void UpdateRaceProgress(RacerSlot *slot) {
    Vehicle *vehicle = slot->vehicle;
    TrackGate *gate;
    s32 current;
    s32 next;
    f32 lapTime;

    /* The count of the gates is read from the track at each use. */
    if (g_LoadedTrack.gateCount == 0) {
        return;
    }
    if (vehicle != NULL) {
        slot->position = vehicle->frame.position;
        slot->previousPosition = vehicle->previousPosition;
        slot->maxGate = vehicle->maxGate;
        slot->gate = vehicle->gate;
        slot->lap = vehicle->lap;
        slot->distanceLaps = vehicle->distanceLaps;
        slot->lapTime = vehicle->lapTime;
        slot->bestLapTime = vehicle->bestLapTime;
        slot->distance = vehicle->distance;
    }
    current = GateInLap(slot->gate, g_LoadedTrack.gateCount);
    next = (current + 1) % g_LoadedTrack.gateCount;
    lapTime = (f32)FloatToInt(slot->lapTime * LAP_TIME_SCALE) / LAP_TIME_SCALE;

    if (CROSSES_GATE(&g_LoadedTrack.gates[current], &slot->previousPosition, &slot->position)) {
        osSyncPrintf("%d went backwards, from gate %d to gate %d\n",
                     slot->index,
                     slot->gate,
                     slot->gate - 1);
        if (current == 0) {
            slot->distanceLaps--;
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
        if (current == slot->maxGate % g_LoadedTrack.gateCount && slot->gate < 0) {
            slot->gate += g_LoadedTrack.gateCount;
            slot->distanceLaps++;
            if (g_LoadedTrack.rootSection != NULL) {
                slot->distance += g_LoadedTrack.rootSection->points[0].distance;
            }
            osSyncPrintf("lapping %d forward to -1\n", slot->index);
        }
        slot->gate--;
    } else {
        gate = &g_LoadedTrack.gates[next];
        if (CROSSES_GATE(gate, &slot->previousPosition, &slot->position)) {
            osSyncPrintf("%d went forward, from gate %d to gate %d\n",
                         slot->index,
                         slot->gate,
                         slot->gate + 1);
            slot->gate++;
            if (slot->maxGate < slot->gate) {
                ReachNewGate(slot, vehicle, next, lapTime);
            } else {
                ReachGateAgain(slot, vehicle, gate, lapTime);
            }
        }
    }

    if (vehicle != NULL) {
        vehicle->maxGate = slot->maxGate;
        vehicle->gate = slot->gate;
        vehicle->lap = slot->lap;
        vehicle->distanceLaps = slot->distanceLaps;
        vehicle->lapTime = slot->lapTime;
        vehicle->bestLapTime = slot->bestLapTime;
        vehicle->distance = slot->distance;
    }
}

/* Step the vehicle of a racer, retaining its last position and letting its driver drive it. The
 * vehicle is read from the racer at each use. */
#define DRIVE_RACER_VEHICLE(slot)                                                                  \
    {                                                                                              \
        Vehicle *vehicle = (slot)->vehicle;                                                        \
        Vec3f *from = &vehicle->frame.position;                                                    \
        Vec3f *to = &vehicle->previousPosition;                                                    \
        to->x = from->x;                                                                           \
        to->y = from->y;                                                                           \
        to->z = from->z;                                                                           \
    }                                                                                              \
    if ((slot)->vehicle->driver != NULL) {                                                         \
        (slot)->vehicle->driver((slot)->vehicle);                                                  \
    }

void UpdateRacerSlotMotion(RacerSlot *slot) {
    Vehicle *pacer;
    TrackPoint *point;
    f32 lapLength;
    f32 fraction;
    s32 i;

    /* The vehicle is read from the racer at each use. */
    if (slot->vehicle != NULL && slot->vehicle->driver == DriveAiVehicle &&
        g_dwGameMode != GAME_MODE_AFTER_SEASON) {
        slot->vehicle->input->buttons &= INPUT_VIEWS;
        slot->vehicle->input->steering = 0.0f;
    }
    if (g_dwRacePauseState) {
        for (i = 0; i != 5; i++) {
            g_aSoundEffectChannels[i].step = 0;
            g_aSoundEffectChannels[i].volume = 0;
        }
        return;
    }
    if (slot->status & RACER_HELD) {
        if (slot->vehicle == NULL) {
            return;
        }
        slot->vehicle->input->buttons |= INPUT_BRAKE;
        slot->vehicle->gear = 0;
        DRIVE_RACER_VEHICLE(slot);
        slot->vehicle->gear = 0;
        return;
    }
    if (slot->status & RACER_FINISHED) {
        if (slot->vehicle == NULL) {
            return;
        }
        if (g_dwGameMode != GAME_MODE_CHAMPIONSHIP || slot->index < (s32)g_dwNumLocalPlayers) {
            slot->vehicle->input->buttons = INPUT_BRAKE | INPUT_EBRAKE;
            slot->vehicle->input->steeringInput = FINISHED_STEERING_INPUT;
            slot->vehicle->input->steering = -1.0f;
        }
        if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP && slot->index >= (s32)g_dwNumLocalPlayers) {
            slot->vehicle->ghostMode = VEHICLE_GHOST;
            slot->vehicle->opacity -= g_fFrameDeltaTime;
            if (slot->vehicle->opacity < 0.0f) {
                slot->vehicle->opacity = 0.0f;
            }
        }
        DRIVE_RACER_VEHICLE(slot);
        return;
    }
    if (slot->vehicle != NULL) {
        if (slot->vehicle->ghostMode == VEHICLE_GHOST) {
            slot->vehicle->opacity += g_fFrameDeltaTime * GHOST_FADE_RATE;
            if (g_dwGameMode == GAME_MODE_TIME_ATTACK && slot->index != 0) {
                /* The ghost of a time attack stays faint. */
                if (TIME_ATTACK_GHOST_OPACITY < slot->vehicle->opacity) {
                    slot->vehicle->opacity = TIME_ATTACK_GHOST_OPACITY;
                }
            } else if (1.0f <= slot->vehicle->opacity) {
                slot->vehicle->opacity = 1.0f;
                slot->vehicle->ghostMode = 0;
            }
        }
        DRIVE_RACER_VEHICLE(slot);
        slot->vehicle->odometer += slot->vehicle->speedKph * g_fFrameDeltaTime;
        return;
    }

    /* A racer without a vehicle keeps the pace of a vehicle of its kind. */
    slot->previousPosition = slot->position;
    lapLength = g_LoadedTrack.rootSection->points[0].distance;
    point = &slot->section->points[slot->pointIndex];
    fraction = ((lapLength * (f32)(slot->distanceLaps + 1) - slot->distance) - point[1].distance) /
               (point[0].distance - point[1].distance);
    pacer = &g_aVehicles[g_dwNumLocalPlayers + slot->straggler];
    if (pacer->racer != NULL) {
        AdvanceAlongTrackRoute(slot->section,
                               slot->pointIndex,
                               fraction,
                               Vec3fLength(&pacer->placedVelocity) * g_fFrameDeltaTime);
        slot->distance +=
            Vec3fLength(&g_aVehicles[g_dwNumLocalPlayers + slot->straggler].placedVelocity) *
            g_fFrameDeltaTime;
    } else {
        AdvanceAlongTrackRoute(slot->section, slot->pointIndex, fraction, IDLE_RACER_STEP);
        slot->distance += IDLE_RACER_STEP;
    }
    slot->section = g_pTrackRouteSection;
    slot->pointIndex = g_dwTrackRoutePointIndex;
    slot->position = g_vTrackRoutePoint;
    Vec3fSubtract(&slot->velocity, &slot->position, &slot->previousPosition);
    Vec3fScaleInPlace(&slot->velocity, 1.0f / g_fFrameDeltaTime);
    UpdateRaceProgress(slot);
}

void UpdateRacerSlotRaceState(RacerSlot *slot) {
    if (g_dwRacePauseState || slot->vehicle == NULL) {
        return;
    }
    if (slot->vehicle->body.impactSpeed) {
        DeformVehicleMeshByImpact(
            slot->vehicle, &slot->vehicle->body.impactPoint, (s16)slot->vehicle->body.impactSpeed);
        slot->vehicle->body.impactSpeed = 0;
    }
    UpdateRaceTimers(slot->vehicle);
    UpdateHudMessageTimers(slot->vehicle);
    CheckWrongWayWarning(slot->vehicle);
    UpdateRaceProgress(slot);
    UpdateVehicleGridPosition(slot->vehicle);
    Vec3fSubtract(&slot->vehicle->placedVelocity,
                  &slot->vehicle->frame.position,
                  &slot->vehicle->previousPosition);
    Vec3fScaleInPlace(&slot->vehicle->placedVelocity, 1.0f / g_fFrameDeltaTime);
    slot->vehicle->cameraHeading =
        Atan2f(slot->vehicle->activeCamera->forward.x, slot->vehicle->activeCamera->forward.y);
    if (slot->vehicle->slotCooldown) {
        slot->vehicle->slotCooldown--;
    }
}
