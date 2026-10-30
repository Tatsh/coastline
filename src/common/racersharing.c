#include "common/racersharing.h"

#include "common/racerslotaccess.h"
#include "common/vehicleaccess.h"

#if TGR_PLATFORM == TGR_PLATFORM_N64
#include <PR/os_libc.h>

#include "common/util.h"
#else
#include <stdlib.h>

#include "common/screen.h"
#define osSyncPrintf(...) NullStub()
#endif /* TGR_PLATFORM == TGR_PLATFORM_N64 */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
#define QuickSort qsort
#elif TGR_PLATFORM == TGR_PLATFORM_SDL
#include "common/util.h"
#endif

#include "common/game.h"
#include "common/input.h"
#include "common/vehiclecount.h"
#include "common/vehicleeffects.h"
#include "common/vehiclereset.h"

#define RACER_KINDS 2

#if TGR_BOSS_RALLY
typedef long double SharingReal;
#define SHARING_INTERMEDIATE(value) ((long double)(value))
#define SHARING_ZERO(value) (!((value) < 0.0f) && !((value) > 0.0f))
#else
typedef f32 SharingReal;
#define SHARING_INTERMEDIATE(value) (value)
#define SHARING_ZERO(value) ((value) == 0.0f)
#endif

/* The maximum number of free vehicles during reassignment. */
#define FREE_VEHICLES 12

/* Other players and finished championship racers retain their positions during reassignment. */
#define KEY_OTHER_PLAYER 1e10f
#define KEY_FINISHED 1e9f

/* A computer's vehicle departs from a racer farther than this, by its key, and waits this many
 * frames before it takes another. */
#define RELEASE_KEY 80000.0
#define SLOT_COOLDOWN_FRAMES 60

/* A vehicle taken by a racer starts just above the route, at this speed, and at this many frames
 * upright. */
#define ACQUIRE_HEIGHT 0.1f
#define ACQUIRE_SPEED 50.0f
#define ACQUIRE_UPRIGHT_FRAMES 40
#define AI_START_GRACE -180

s32 CompareRacerProgressAsc(const void *a, const void *b) {
    const RacerProgress *first = (const RacerProgress *)a;
    const RacerProgress *second = (const RacerProgress *)b;

    if (second->progress < first->progress) {
        return 1;
    }
#if TGR_BOSS_RALLY
    if (!(first->progress >= second->progress)) {
#else
    if (first->progress < second->progress) {
#endif
        return -1;
    }
    return 0;
}

/* Bring a distance along the route within half a lap. */
static BOSS_RALLY_INLINE f32 WrapHalfLap(SharingReal rawDistance, f32 lapLength) {
    f32 distance = rawDistance;

    if (SHARING_INTERMEDIATE(lapLength) * 0.5f < rawDistance) {
        distance = SHARING_INTERMEDIATE(distance) - lapLength;
    } else if (distance < SHARING_INTERMEDIATE(lapLength) * -0.5f) {
        distance = SHARING_INTERMEDIATE(distance) + lapLength;
    }
    return distance;
}

/* The key a racer sorts by when vehicles are shared out: near the player first. */
static BOSS_RALLY_INLINE f32 RacerSharingKey(RacerSlot *slot, Vehicle *player, f32 lapLength) {
    Vehicle *vehicle = RACER_SLOT_GET(slot, Vehicle);
    f32 gap;
    SharingReal rawGap;

    if (vehicle != NULL) {
        if (vehicle != player && VEHICLE_GET(vehicle, Index) < (s32)g_dwNumLocalPlayers) {
            return KEY_OTHER_PLAYER;
        }
        if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP &&
            VEHICLE_GET(vehicle, Index) >= (s32)g_dwNumLocalPlayers &&
            (RACER_SLOT_GET(VEHICLE_GET(vehicle, Racer), Status) & RACER_FINISHED) &&
            VEHICLE_GET(vehicle, GhostMode) == VEHICLE_GHOST &&
            SHARING_ZERO(VEHICLE_GET(vehicle, Opacity))) {
            return KEY_FINISHED;
        }
        rawGap =
            (SHARING_INTERMEDIATE(VEHICLE_GET(player, Distance)) - VEHICLE_GET(vehicle, Distance)) +
            SHARING_INTERMEDIATE(VEHICLE_GET(vehicle, DistanceLaps) -
                                 VEHICLE_GET(player, DistanceLaps)) *
                lapLength;
        gap = WrapHalfLap(rawGap, lapLength);
        return Vec3fDistance(&VEHICLE_GET(player, Frame).position,
                             &VEHICLE_GET(vehicle, Frame).position) +
               SHARING_INTERMEDIATE(gap) * gap;
    }
    if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP &&
        RACER_SLOT_GET(slot, Index) >= (s32)g_dwNumLocalPlayers &&
        (RACER_SLOT_GET(slot, Status) & RACER_FINISHED)) {
        return KEY_FINISHED;
    }
    rawGap = SHARING_INTERMEDIATE(RACER_SLOT_GET(slot, DistanceLaps) -
                                  VEHICLE_GET(player, DistanceLaps)) *
                 lapLength +
             (SHARING_INTERMEDIATE(VEHICLE_GET(player, Distance)) - RACER_SLOT_GET(slot, Distance));
    gap = WrapHalfLap(rawGap, lapLength);
    return Vec3fDistance(&VEHICLE_GET(player, Frame).position, &RACER_SLOT_GET(slot, Position)) +
           SHARING_INTERMEDIATE(gap) * gap;
}

/* Transfer the vehicle's progress to its racer before detaching the vehicle. */
static BOSS_RALLY_INLINE void ReleaseRacerVehicle(RacerSlot *slot, Vehicle *vehicle, f32 key) {
#if TGR_PLATFORM == TGR_PLATFORM_N64
    osSyncPrintf("enemy %d loses slot %d (dist=%f)\n", slot->index, vehicle->index, (double)key);
#endif
    RACER_SLOT_GET(slot, Position) = VEHICLE_GET(vehicle, Frame).position;
    RACER_SLOT_GET(slot, Velocity) = VEHICLE_GET(vehicle, PlacedVelocity);
    RACER_SLOT_SET(slot, SpeedKph, VEHICLE_GET(vehicle, SpeedKph));
    RACER_SLOT_SET(slot, Section, VEHICLE_GET(vehicle, Section));
    RACER_SLOT_SET(slot, PointIndex, VEHICLE_GET(vehicle, PointIndex));
    RACER_SLOT_SET(slot, LapTime, VEHICLE_GET(vehicle, LapTime));
    RACER_SLOT_SET(slot, BestLapTime, VEHICLE_GET(vehicle, BestLapTime));
    RACER_SLOT_SET(slot, TotalTime, VEHICLE_GET(vehicle, TotalTime));
    RACER_SLOT_SET(slot, TimeLeft, VEHICLE_GET(vehicle, TimeLeft));
    RACER_SLOT_SET(slot, Lap, VEHICLE_GET(vehicle, Lap));
    RACER_SLOT_SET(slot, DistanceLaps, VEHICLE_GET(vehicle, DistanceLaps));
    RACER_SLOT_SET(slot, MaxGate, VEHICLE_GET(vehicle, MaxGate));
    RACER_SLOT_SET(slot, Gate, VEHICLE_GET(vehicle, Gate));
    osSyncPrintf("saving lap (%d/%d) and gate (%d/%d)\n",
                 slot->lap,
                 slot->distanceLaps,
                 slot->maxGate,
                 slot->gate);
    RACER_SLOT_SET(slot, Distance, VEHICLE_GET(vehicle, Distance));
    RACER_SLOT_SET(slot, RacePosition, VEHICLE_GET(vehicle, RacePosition));
}

/* Give a racer a vehicle, placed on the route where the racer is, facing along it. */
static BOSS_RALLY_INLINE void AcquireRacerVehicle(RacerSlot *slot, Vehicle *vehicle) {
    Vec3f left;
    Vec3f nextLeft;

    RACER_SLOT_SET(slot, Vehicle, vehicle);
#if TGR_PLATFORM == TGR_PLATFORM_N64
    osSyncPrintf("enemy %d acquires slot %d\n", slot->index, vehicle->index);
#endif
    VEHICLE_SET(vehicle, SpeedKph, RACER_SLOT_GET(slot, SpeedKph));
    VEHICLE_SET(vehicle, Section, RACER_SLOT_GET(slot, Section));
    VEHICLE_SET(vehicle, PointIndex, RACER_SLOT_GET(slot, PointIndex));
    VEHICLE_SET(vehicle, LapTime, RACER_SLOT_GET(slot, LapTime));
    VEHICLE_SET(vehicle, BestLapTime, RACER_SLOT_GET(slot, BestLapTime));
    VEHICLE_SET(vehicle, TotalTime, RACER_SLOT_GET(slot, TotalTime));
    VEHICLE_SET(vehicle, TimeLeft, RACER_SLOT_GET(slot, TimeLeft));
    VEHICLE_SET(vehicle, Lap, RACER_SLOT_GET(slot, Lap));
    VEHICLE_SET(vehicle, DistanceLaps, RACER_SLOT_GET(slot, DistanceLaps));
    VEHICLE_SET(vehicle, MaxGate, RACER_SLOT_GET(slot, MaxGate));
    VEHICLE_SET(vehicle, Gate, RACER_SLOT_GET(slot, Gate));
    osSyncPrintf("restoring lap (%d/%d) and gate (%d/%d)\n",
                 vehicle->lap,
                 vehicle->distanceLaps,
                 vehicle->maxGate,
                 vehicle->gate);
    VEHICLE_SET(vehicle, Distance, RACER_SLOT_GET(slot, Distance));
    VEHICLE_SET(vehicle, Racer, slot);
    VEHICLE_SET(vehicle, GhostMode, 0);
    VEHICLE_SET(vehicle, Opacity, 1.0f);
    VEHICLE_SET(vehicle, CameraChanged, 1);
    VEHICLE_SET(vehicle, RacePosition, RACER_SLOT_GET(slot, RacePosition));
    InitVehiclePhysics(vehicle);
    ResetVehicleEffects(vehicle);
    VEHICLE_GET(vehicle, PreviousPosition) = RACER_SLOT_GET(slot, PreviousPosition);
    SetVehiclePosition(vehicle,
                       RACER_SLOT_GET(slot, Position).x,
                       RACER_SLOT_GET(slot, Position).y,
                       RACER_SLOT_GET(slot, Position).z + ACQUIRE_HEIGHT);
    /* Stored a second time. */
    VEHICLE_SET(vehicle, GhostMode, 0);
    VEHICLE_SET(vehicle, Opacity, 1.0f);
    VEHICLE_GET(vehicle, Color)[0] = RACER_SLOT_GET(slot, Color)[0];
    VEHICLE_GET(vehicle, Color)[1] = RACER_SLOT_GET(slot, Color)[1];
    VEHICLE_GET(vehicle, Color)[2] = RACER_SLOT_GET(slot, Color)[2];
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    ApplyEnemyCarColorPanels(vehicle, slot);
#elif !TGR_BOSS_RALLY
    SetMaterialColor(vehicle->model, slot->color[0] >> 3, slot->color[1] >> 3, slot->color[2] >> 3);
#endif

    /* The point is found again from the vehicle for each call. */
    Vec3fSubtract(
        &left,
        &VEHICLE_GET(vehicle, Section)->points[VEHICLE_GET(vehicle, PointIndex)].left,
        &VEHICLE_GET(vehicle, Section)->points[VEHICLE_GET(vehicle, PointIndex)].position);
    Vec3fSubtract(
        &nextLeft,
        &VEHICLE_GET(vehicle, Section)->points[VEHICLE_GET(vehicle, PointIndex) + 1].left,
        &VEHICLE_GET(vehicle, Section)->points[VEHICLE_GET(vehicle, PointIndex) + 1].position);
    Vec3fDirection(
        &VEHICLE_GET(vehicle, Frame).forward,
        &VEHICLE_GET(vehicle, Section)->points[VEHICLE_GET(vehicle, PointIndex)].position,
        &VEHICLE_GET(vehicle, Section)->points[VEHICLE_GET(vehicle, PointIndex) + 1].position);
    Vec3fMidpoint(&VEHICLE_GET(vehicle, Frame).side, &left, &nextLeft);
    Vec3fCross(&VEHICLE_GET(vehicle, Frame).up,
               &VEHICLE_GET(vehicle, Frame).forward,
               &VEHICLE_GET(vehicle, Frame).side);
    Vec3fNormalize(&VEHICLE_GET(vehicle, Frame).up);
    Vec3fCross(&VEHICLE_GET(vehicle, Frame).side,
               &VEHICLE_GET(vehicle, Frame).up,
               &VEHICLE_GET(vehicle, Frame).forward);
    Vec3fNormalize(&VEHICLE_GET(vehicle, Frame).side);
    VEHICLE_GET(vehicle, TrackDirection) = VEHICLE_GET(vehicle, Frame).forward;
    /* The frame is set from itself, to set the orientation of the body. */
    SetVehicleFrameMatrix(vehicle, VEHICLE_GET(vehicle, Matrix));
    if (RACER_SLOT_GET(VEHICLE_GET(vehicle, Racer), Status) & RACER_HELD) {
        SetVehicleVelocity(vehicle, 0.0f, 0.0f, 0.0f);
    } else {
        SetVehicleVelocity(vehicle,
                           VEHICLE_GET(vehicle, Frame).forward.x * ACQUIRE_SPEED,
                           VEHICLE_GET(vehicle, Frame).forward.y * ACQUIRE_SPEED,
                           VEHICLE_GET(vehicle, Frame).forward.z * ACQUIRE_SPEED);
    }
    /* Each wheel pointer is read again for each store, the third wheel after the fourth. */
    VEHICLE_GET(vehicle, Body).wheels[0]->ground = NULL;
    VEHICLE_GET(vehicle, Body).wheels[0]->contactFrames = 0;
    VEHICLE_GET(vehicle, Body).wheels[0]->surface = TRACK_SURFACE_START;
    VEHICLE_GET(vehicle, Body).wheels[1]->ground = NULL;
    VEHICLE_GET(vehicle, Body).wheels[1]->contactFrames = 0;
    VEHICLE_GET(vehicle, Body).wheels[1]->surface = TRACK_SURFACE_START;
    VEHICLE_GET(vehicle, Body).wheels[3]->ground = NULL;
    VEHICLE_GET(vehicle, Body).wheels[3]->contactFrames = 0;
    VEHICLE_GET(vehicle, Body).wheels[3]->surface = TRACK_SURFACE_START;
    VEHICLE_GET(vehicle, Body).wheels[2]->ground = NULL;
    VEHICLE_GET(vehicle, Body).wheels[2]->contactFrames = 0;
    VEHICLE_GET(vehicle, Body).wheels[2]->surface = TRACK_SURFACE_START;
    VEHICLE_SET(vehicle, ForcedForwardFrames, 0);
    VEHICLE_SET(vehicle, ForcedReverseFrames, 0);
    VEHICLE_SET(vehicle, ReverseFrames, 0);
    VEHICLE_SET(vehicle, ForwardFrames, AI_START_GRACE);
    VEHICLE_GET(vehicle, Body).uprightFrames = ACQUIRE_UPRIGHT_FRAMES;
    VEHICLE_SET(vehicle, SteerAngle, 0.0f);
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
        progress = entries[VEHICLE_GET(player, Index)].progress;
        entries[VEHICLE_GET(player, Index)].progress = entries[0].progress;
        entries[0].progress = progress;
        entries[0].slot = VEHICLE_GET(player, Index);
        entries[VEHICLE_GET(player, Index)].slot = 0;
        QuickSort(&entries[1], g_dwNumVehicles - 1, sizeof(RacerProgress), CompareRacerProgressAsc);
    }

    for (kind = 0; kind != RACER_KINDS; kind++) {
        noVehicle = TRUE;
        freeCount = 0;
        seen = 0;
        for (i = 0; i < g_dwNumVehicles; i++) {
            slot = &g_aRacerSlots[entries[i].slot];
            if (RACER_SLOT_GET(slot, Straggler) != kind ||
                RACER_SLOT_GET(slot, Index) < (s32)g_dwNumLocalPlayers) {
                continue;
            }
            vehicle = RACER_SLOT_GET(slot, Vehicle);
            if (seen <= 0) {
                /* The nearest racer retains its vehicle until it finished and faded away. */
                seen++;
                if (vehicle == NULL) {
                    continue;
                }
                noVehicle = FALSE;
                if (!(RACER_SLOT_GET(slot, Status) & RACER_FINISHED) ||
                    VEHICLE_GET(vehicle, GhostMode) != VEHICLE_GHOST ||
                    !SHARING_ZERO(VEHICLE_GET(vehicle, Opacity))) {
                    continue;
                }
            } else {
                if (vehicle == NULL) {
                    continue;
                }
                noVehicle = FALSE;
                if (VEHICLE_GET(vehicle, SlotCooldown)) {
                    continue;
                }
                if (!(RELEASE_KEY < (double)entries[i].progress)) {
                    continue;
                }
            }
            ReleaseRacerVehicle(slot, vehicle, entries[i].progress);
            freeVehicles[freeCount++] = RACER_SLOT_GET(slot, Vehicle);
            RACER_SLOT_SET(slot, Vehicle, NULL);
            VEHICLE_SET(vehicle, Racer, NULL);
            VEHICLE_SET(vehicle, SlotCooldown, SLOT_COOLDOWN_FRAMES);
        }
        if (noVehicle) {
#if TGR_PLATFORM == TGR_PLATFORM_N64
            osSyncPrintf("init slot %d to being open\n", g_dwNumLocalPlayers + kind);
#endif
            freeVehicles[freeCount++] = &g_aVehicles[g_dwNumLocalPlayers + kind];
        }
        if (freeCount == 0 || g_dwNumVehicles <= 0) {
            continue;
        }
        for (i = 0;;) {
            slot = &g_aRacerSlots[entries[i].slot];
            if (RACER_SLOT_GET(slot, Straggler) == kind &&
                RACER_SLOT_GET(slot, Index) >= (s32)g_dwNumLocalPlayers &&
                !(RACER_SLOT_GET(slot, Status) & RACER_FINISHED) &&
                RACER_SLOT_GET(slot, Vehicle) == NULL) {
                AcquireRacerVehicle(slot, freeVehicles[--freeCount]);
            }
            VEHICLE_GET(player, NearestRacers)[i] = &g_aRacerSlots[entries[i].slot];
            i++;
            if (freeCount == 0 || i >= g_dwNumVehicles) {
                break;
            }
        }
    }
}
