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
#define RACER_KINDS 2

/* The vehicles a sharing out can hold free at once. */
#define FREE_VEHICLES 12

/* The keys of the racers that keep their places in the sharing out of vehicles: other players,
 * and finished racers in the championship. */
#define KEY_OTHER_PLAYER 1e10f
#define KEY_FINISHED 1e9f

/* A computer's vehicle leaves a racer farther than this, by its key, and waits this many frames
 * before it takes another. */
#define RELEASE_KEY 80000.0
#define SLOT_COOLDOWN_FRAMES 60

/* A vehicle taken by a racer starts just above the route, at this speed, and at this many frames
 * upright. */
#define ACQUIRE_HEIGHT 0.1f
#define ACQUIRE_SPEED 50.0f
#define ACQUIRE_UPRIGHT_FRAMES 40
#define AI_START_GRACE -180

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

/* The progress of a racer for sorting. */
typedef struct {
    f32 progress; /*!< The sort key, the distance covered or a key derived from it. */
    s32 slot;     /*!< The index of the racer in g_aRacerSlots. */
} RacerProgress;

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

void FindTrackRoutePoint(TrackSection *section, f32 distance) {
    f32 span;
    s32 next;
    s32 i;

    g_dwTrackRouteGates = 0;
    g_dwTrackRouteLaps = 0;
    for (;;) {
        SKIP_ALTERNATE_SECTIONS(section);
        if (section == NULL) {
            return;
        }
        /* The last point of a section reads the point after it. */
        for (i = 0; i < section->pointCount; i++) {
            span = section->points[i].distance - section->points[i + 1].distance;
            if (span < distance) {
                distance -= span;
                COUNT_ROUTE_GATE(
                    next, &section->points[i].position, &section->points[i + 1].position);
                continue;
            }
            Vec3fLerp(&g_vTrackRoutePoint,
                      &section->points[i + 1].position,
                      &section->points[i].position,
                      distance / span);
            COUNT_ROUTE_GATE(next, &section->points[i].position, &g_vTrackRoutePoint);
            g_pTrackRouteSection = section;
            g_dwTrackRoutePointIndex = i;
            return;
        }
        section = section->child;
    }
}

void AdvanceAlongTrackRoute(TrackSection *section, s32 pointIndex, f32 fraction, f32 distance) {
    f32 length;

    for (;;) {
        SKIP_ALTERNATE_SECTIONS(section);
        if (section == NULL) {
            return;
        }
        for (; pointIndex < section->pointCount; pointIndex++) {
            length =
                (section->points[pointIndex].distance - section->points[pointIndex + 1].distance) *
                fraction;
            if (length < distance) {
                fraction = 1.0f;
                distance -= length;
                continue;
            }
            Vec3fLerp(&g_vTrackRoutePoint,
                      &section->points[pointIndex].position,
                      &section->points[pointIndex + 1].position,
                      fraction);
            Vec3fLerp(&g_vTrackRoutePoint,
                      &section->points[pointIndex + 1].position,
                      &g_vTrackRoutePoint,
                      distance / length);
            g_pTrackRouteSection = section;
            g_dwTrackRoutePointIndex = pointIndex;
            return;
        }
        section = section->child;
        pointIndex = 0;
    }
}

/* Set weather to the clamped index of the weather of the arcade times. */
#define ARCADE_WEATHER(weather)                                                                    \
    weather = g_dwCurrentWeather - 1;                                                              \
    if ((weather) >= ARCADE_WEATHERS || (weather) < 0) {                                           \
        weather = 0;                                                                               \
    }

/* The arcade times of the selected track. The descriptor of a track starts with its menu item. */
#define SELECTED_TRACK_ITEM() ((TrackMenuItem *)g_apTrackMenuItems[g_dwSelectedTrackId])

void InitRacerSlot(RacerSlot *slot) {
    s32 count;
    s32 index;
    s16 weather;

    /* The original compares the mode as a signed value. */
    switch ((s32)g_dwGameMode) {
    default:
        count = g_dwNumActiveVehicles;
        break;
    case GAME_MODE_ARCADE:
        count = ARCADE_VEHICLES;
        break;
    case GAME_MODE_CHAMPIONSHIP:
        count = g_dwNumLocalPlayers;
        break;
    }
    index = slot - g_aRacerSlots;
    slot->index = index;
    slot->color[0] = g_abPlayerColorsRgb[index][0];
    slot->color[1] = g_abPlayerColorsRgb[index][1];
    slot->color[2] = g_abPlayerColorsRgb[index][2];
    slot->status = 0;
    slot->straggler = index < (s32)g_dwNumLocalPlayers + LEADING_RACERS ? 0 : 1;
    if (slot->index < count) {
        slot->vehicle = &g_aVehicles[slot->index];
        slot->vehicle->racer = slot;
        return;
    }

    slot->vehicle = NULL;
    if (slot->straggler == 1) {
        slot->distance =
            (f32)(STRAGGLER_SPACING * (slot->index - (s32)g_dwNumLocalPlayers) - STRAGGLER_START);
    } else if (g_dwSelectedTrackId == TRACK_COASTLINE ||
               g_dwSelectedTrackId == TRACK_MIRROR_COASTLINE) {
        slot->distance = (f32)(slot->index * NARROW_RACER_SPACING);
    } else {
        slot->distance = (f32)(slot->index * RACER_SPACING);
    }
    FindTrackRoutePoint(g_LoadedTrack.rootSection, slot->distance);
    slot->position.x = g_vTrackRoutePoint.x;
    slot->position.y = g_vTrackRoutePoint.y;
    slot->position.z = g_vTrackRoutePoint.z;
    slot->section = g_pTrackRouteSection;
    slot->pointIndex = g_dwTrackRoutePointIndex;
    slot->maxGate = slot->gate = g_dwTrackRouteGates;
    slot->lap = slot->distanceLaps = g_dwTrackRouteLaps;
    osSyncPrintf("Initial lap.gate for %d(%d): %d.%d (dist=%f)\n",
                 slot->index,
                 slot->straggler,
                 slot->lap,
                 slot->gate,
                 (double)slot->distance);
    slot->previousPosition.x = slot->position.x;
    slot->previousPosition.y = slot->position.y;
    slot->previousPosition.z = slot->position.z;
    slot->lapTime = 0.0f;
    slot->totalTime = 0.0f;
    slot->bestLapTime = 0.0f;
    ARCADE_WEATHER(weather);
    slot->timeLeft =
        SELECTED_TRACK_ITEM()->arcadeTimes[g_aVehicles[0].tableIndex][weather].startTime;
    slot->racePosition = g_dwNumVehicles - slot->index - 1;
}

/* Order racers by their progress, the least first.
 * @ghidraAddress USA: 0x80229510 */
static s32 CompareRacerProgressAsc(const void *a, const void *b) {
    const RacerProgress *first = a;
    const RacerProgress *second = b;

    if (second->progress < first->progress) {
        return 1;
    }
    if (first->progress < second->progress) {
        return -1;
    }
    return 0;
}

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

/* Bring a distance along the route within half a lap. */
static f32 WrapHalfLap(f32 distance, f32 lapLength) {
    if (lapLength * 0.5f < distance) {
        distance -= lapLength;
    } else if (distance < lapLength * -0.5f) {
        distance += lapLength;
    }
    return distance;
}

/* The key a racer sorts by when vehicles are shared out: near the player first. */
static f32 RacerSharingKey(RacerSlot *slot, Vehicle *player, f32 lapLength) {
    Vehicle *vehicle = slot->vehicle;
    f32 gap;

    if (vehicle != NULL) {
        if (vehicle != player && vehicle->index < (s32)g_dwNumLocalPlayers) {
            return KEY_OTHER_PLAYER;
        }
        if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP && vehicle->index >= (s32)g_dwNumLocalPlayers &&
            (vehicle->racer->status & RACER_FINISHED) && vehicle->ghostMode == VEHICLE_GHOST &&
            vehicle->opacity == 0.0f) {
            return KEY_FINISHED;
        }
        gap = (player->distance - vehicle->distance) +
              (f32)(vehicle->distanceLaps - player->distanceLaps) * lapLength;
        gap = WrapHalfLap(gap, lapLength);
        return Vec3fDistance(&player->frame.position, &vehicle->frame.position) + gap * gap;
    }
    if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP && slot->index >= (s32)g_dwNumLocalPlayers &&
        (slot->status & RACER_FINISHED)) {
        return KEY_FINISHED;
    }
    gap = (f32)(slot->distanceLaps - player->distanceLaps) * lapLength +
          (player->distance - slot->distance);
    gap = WrapHalfLap(gap, lapLength);
    return Vec3fDistance(&player->frame.position, &slot->position) + gap * gap;
}

/* Take the vehicle from a racer, which keeps the progress of the vehicle. */
static void ReleaseRacerVehicle(RacerSlot *slot, Vehicle *vehicle, f32 key) {
    osSyncPrintf("enemy %d loses slot %d (dist=%f)\n", slot->index, vehicle->index, (double)key);
    slot->position = vehicle->frame.position;
    slot->velocity = vehicle->placedVelocity;
    slot->speedKph = vehicle->speedKph;
    slot->section = vehicle->section;
    slot->pointIndex = vehicle->pointIndex;
    slot->lapTime = vehicle->lapTime;
    slot->bestLapTime = vehicle->bestLapTime;
    slot->totalTime = vehicle->totalTime;
    slot->timeLeft = vehicle->timeLeft;
    slot->lap = vehicle->lap;
    slot->distanceLaps = vehicle->distanceLaps;
    slot->maxGate = vehicle->maxGate;
    slot->gate = vehicle->gate;
    osSyncPrintf("saving lap (%d/%d) and gate (%d/%d)\n",
                 slot->lap,
                 slot->distanceLaps,
                 slot->maxGate,
                 slot->gate);
    slot->distance = vehicle->distance;
    slot->racePosition = vehicle->racePosition;
}

/* Give a racer a vehicle, placed on the route where the racer is, facing along it. */
static void AcquireRacerVehicle(RacerSlot *slot, Vehicle *vehicle) {
    Vec3f left;
    Vec3f nextLeft;

    slot->vehicle = vehicle;
    osSyncPrintf("enemy %d acquires slot %d\n", slot->index, vehicle->index);
    vehicle->speedKph = slot->speedKph;
    vehicle->section = slot->section;
    vehicle->pointIndex = slot->pointIndex;
    vehicle->lapTime = slot->lapTime;
    vehicle->bestLapTime = slot->bestLapTime;
    vehicle->totalTime = slot->totalTime;
    vehicle->timeLeft = slot->timeLeft;
    vehicle->lap = slot->lap;
    vehicle->distanceLaps = slot->distanceLaps;
    vehicle->maxGate = slot->maxGate;
    vehicle->gate = slot->gate;
    osSyncPrintf("restoring lap (%d/%d) and gate (%d/%d)\n",
                 vehicle->lap,
                 vehicle->distanceLaps,
                 vehicle->maxGate,
                 vehicle->gate);
    vehicle->distance = slot->distance;
    vehicle->racer = slot;
    vehicle->ghostMode = 0;
    vehicle->opacity = 1.0f;
    vehicle->cameraChanged = 1;
    vehicle->racePosition = slot->racePosition;
    InitVehiclePhysics(vehicle);
    ResetVehicleEffects(vehicle);
    vehicle->previousPosition = slot->previousPosition;
    SetVehiclePosition(
        vehicle, slot->position.x, slot->position.y, slot->position.z + ACQUIRE_HEIGHT);
    /* Stored a second time. */
    vehicle->ghostMode = 0;
    vehicle->opacity = 1.0f;
    vehicle->color[0] = slot->color[0];
    vehicle->color[1] = slot->color[1];
    vehicle->color[2] = slot->color[2];
    SetMaterialColor(vehicle->model, slot->color[0] >> 3, slot->color[1] >> 3, slot->color[2] >> 3);

    /* The point is found again from the vehicle for each call. */
    Vec3fSubtract(&left,
                  &vehicle->section->points[vehicle->pointIndex].left,
                  &vehicle->section->points[vehicle->pointIndex].position);
    Vec3fSubtract(&nextLeft,
                  &vehicle->section->points[vehicle->pointIndex + 1].left,
                  &vehicle->section->points[vehicle->pointIndex + 1].position);
    Vec3fDirection(&vehicle->frame.forward,
                   &vehicle->section->points[vehicle->pointIndex].position,
                   &vehicle->section->points[vehicle->pointIndex + 1].position);
    Vec3fMidpoint(&vehicle->frame.side, &left, &nextLeft);
    Vec3fCross(&vehicle->frame.up, &vehicle->frame.forward, &vehicle->frame.side);
    Vec3fNormalize(&vehicle->frame.up);
    Vec3fCross(&vehicle->frame.side, &vehicle->frame.up, &vehicle->frame.forward);
    Vec3fNormalize(&vehicle->frame.side);
    vehicle->trackDirection = vehicle->frame.forward;
    /* The frame is set from itself, to set the orientation of the body. */
    SetVehicleFrameMatrix(vehicle, vehicle->matrix);
    if (vehicle->racer->status & RACER_HELD) {
        SetVehicleVelocity(vehicle, 0.0f, 0.0f, 0.0f);
    } else {
        SetVehicleVelocity(vehicle,
                           vehicle->frame.forward.x * ACQUIRE_SPEED,
                           vehicle->frame.forward.y * ACQUIRE_SPEED,
                           vehicle->frame.forward.z * ACQUIRE_SPEED);
    }
    /* Each wheel pointer is read again for each store, the third wheel after the fourth. */
    vehicle->body.wheels[0]->ground = NULL;
    vehicle->body.wheels[0]->contactFrames = 0;
    vehicle->body.wheels[0]->surface = TRACK_SURFACE_START;
    vehicle->body.wheels[1]->ground = NULL;
    vehicle->body.wheels[1]->contactFrames = 0;
    vehicle->body.wheels[1]->surface = TRACK_SURFACE_START;
    vehicle->body.wheels[3]->ground = NULL;
    vehicle->body.wheels[3]->contactFrames = 0;
    vehicle->body.wheels[3]->surface = TRACK_SURFACE_START;
    vehicle->body.wheels[2]->ground = NULL;
    vehicle->body.wheels[2]->contactFrames = 0;
    vehicle->body.wheels[2]->surface = TRACK_SURFACE_START;
    vehicle->forcedForwardFrames = 0;
    vehicle->forcedReverseFrames = 0;
    vehicle->reverseFrames = 0;
    vehicle->forwardFrames = AI_START_GRACE;
    vehicle->body.uprightFrames = ACQUIRE_UPRIGHT_FRAMES;
    vehicle->steerAngle = 0.0f;
}

void ManageAiRacerSlots(Vehicle *player) {
    RacerProgress entries[RACER_SLOTS];
    Vehicle *freeVehicles[FREE_VEHICLES];
    RacerSlot *slot;
    Vehicle *vehicle;
    f32 lapLength = g_LoadedTrack.rootSection->points[0].distance;
    f32 progress;
    s32 kind;
    s32 noVehicle;
    s32 freeCount;
    s32 seen;
    s32 i;

    for (i = 0; i < g_dwNumVehicles; i++) {
        entries[i].progress = RacerSharingKey(&g_aRacerSlots[i], player, lapLength);
        entries[i].slot = i;
    }
    if (g_dwNumVehicles >= 2) {
        /* The player's entry goes first, and the others sort behind it. */
        progress = entries[player->index].progress;
        entries[player->index].progress = entries[0].progress;
        entries[0].progress = progress;
        entries[0].slot = player->index;
        entries[player->index].slot = 0;
        QuickSort(&entries[1], g_dwNumVehicles - 1, sizeof(RacerProgress), CompareRacerProgressAsc);
    }

    for (kind = 0; kind != RACER_KINDS; kind++) {
        noVehicle = TRUE;
        freeCount = 0;
        seen = 0;
        for (i = 0; i < g_dwNumVehicles; i++) {
            slot = &g_aRacerSlots[entries[i].slot];
            if (slot->straggler != kind || slot->index < (s32)g_dwNumLocalPlayers) {
                continue;
            }
            vehicle = slot->vehicle;
            if (seen <= 0) {
                /* The nearest racer keeps its vehicle until it finished and faded away. */
                seen++;
                if (vehicle == NULL) {
                    continue;
                }
                noVehicle = FALSE;
                if (!(slot->status & RACER_FINISHED) || vehicle->ghostMode != VEHICLE_GHOST ||
                    vehicle->opacity != 0.0f) {
                    continue;
                }
            } else {
                if (vehicle == NULL) {
                    continue;
                }
                noVehicle = FALSE;
                if (vehicle->slotCooldown) {
                    continue;
                }
                if (!(RELEASE_KEY < (double)entries[i].progress)) {
                    continue;
                }
            }
            ReleaseRacerVehicle(slot, vehicle, entries[i].progress);
            freeVehicles[freeCount++] = slot->vehicle;
            slot->vehicle = NULL;
            vehicle->racer = NULL;
            vehicle->slotCooldown = SLOT_COOLDOWN_FRAMES;
        }
        if (noVehicle) {
            osSyncPrintf("init slot %d to being open\n", g_dwNumLocalPlayers + kind);
            freeVehicles[freeCount++] = &g_aVehicles[g_dwNumLocalPlayers + kind];
        }
        if (freeCount == 0 || g_dwNumVehicles <= 0) {
            continue;
        }
        for (i = 0;;) {
            slot = &g_aRacerSlots[entries[i].slot];
            if (slot->straggler == kind && slot->index >= (s32)g_dwNumLocalPlayers &&
                !(slot->status & RACER_FINISHED) && slot->vehicle == NULL) {
                AcquireRacerVehicle(slot, freeVehicles[--freeCount]);
            }
            player->nearestRacers[i] = &g_aRacerSlots[entries[i].slot];
            i++;
            if (freeCount == 0 || i >= g_dwNumVehicles) {
                break;
            }
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
