#include "common/racespecials.h"

#include <PR/gu.h>
#include <PR/os.h>

#include "common/frametime.h"
#include "common/game.h"
#include "common/race.h"
#include "common/render.h"
#include "common/track.h"
#include "common/trackselect.h"
#include "common/vehicleaccess.h"
#include "common/weather.h"

#if TGR_PC
/** @ghidraAddress BRD3D: 0x10680940 */
s32 g_dwWaterfallCount;
#endif /* TGR_PC */

/* Find the special objects of the track: the airplane, its path and trigger, and the waterfalls. */
void FindTrackSpecials(void) {
    Vec3f axis;
    s32 i;

    g_bAirplaneFlying = FALSE;
    g_dwAirplaneTrigger = 0;
    g_dwAirplaneInstance = 0;
    g_pAirplanePathRight = NULL;
    g_pAirplanePathLeft = NULL;
    g_fAirplaneDistance = 0.0f;
    g_fAirplaneSegmentLength = 0.0f;
    g_dwAirplaneNode = 0;
    g_dwAirplaneNodeCount = 0;
    osSyncPrintf("specials: %d\n", g_LoadedTrack.specialCount);
    g_dwWaterfallCount = 0;
    for (i = 0; i < g_LoadedTrack.specialCount; i++) {
        switch (g_LoadedTrack.specials[i].type) {
        case TRACK_SPECIAL_AIRPLANE:
            g_dwAirplaneInstance = g_LoadedTrack.specials[i].target.instance;
            osSyncPrintf("airplane = %d\n", g_dwAirplaneInstance);
            break;
        case TRACK_SPECIAL_AIRPLANE_PATH_LEFT:
            g_dwAirplaneNodeCount = g_LoadedTrack.specials[i].param.pathLength;
            g_pAirplanePathLeft = g_LoadedTrack.specials[i].target.path;
            osSyncPrintf("airplanePathLeft = %08x\n", g_pAirplanePathLeft);
            break;
        case TRACK_SPECIAL_AIRPLANE_PATH_RIGHT:
            g_pAirplanePathRight = g_LoadedTrack.specials[i].target.path;
            osSyncPrintf("airplanePathRight = %08x\n", g_pAirplanePathRight);
            break;
        case TRACK_SPECIAL_AIRPLANE_TRIGGER:
            g_dwAirplaneTrigger = g_LoadedTrack.specials[i].target.instance;
            osSyncPrintf("airplanetrigger = %d\n", g_dwAirplaneTrigger);
            break;
        case TRACK_SPECIAL_WATERFALL:
            g_adwWaterfallInstances[g_dwWaterfallCount] = g_LoadedTrack.specials[i].target.instance;
            g_dwWaterfallCount++;
            osSyncPrintf("waterfall = %d\n", g_LoadedTrack.specials[i].target.instance);
            break;
        }
    }
    if (!g_dwAirplaneTrigger || !g_pAirplanePathLeft || !g_pAirplanePathRight) {
        g_dwAirplaneInstance = 0;
    }
    if (g_dwAirplaneInstance) {
        /* The scale of the model is the length its matrix gives a unit vector. */
        axis.x = 1.0f;
        axis.y = 0.0f;
        axis.z = 0.0f;
        Mtx4x4ProjectPoint(&axis, &axis, g_LoadedTrack.instances[g_dwAirplaneInstance].matrix);
        g_fAirplaneScale = Vec3fLength(&axis);
    }
    g_dwAirplaneTriggerLap = g_dwTotalLaps - 1;
}

#if TGR_PC
#define RACE_SPECIAL_MATRIX g_afCameraRollMatrix
#define RACE_INSTANCE_VECTOR(row) (&g_LoadedTrack.instances[g_dwAirplaneInstance].rows[row].vector)
#else
#define RACE_SPECIAL_MATRIX g_afMvpMatrix
#define RACE_INSTANCE_VECTOR(row)                                                                  \
    ((Vec3f *)g_LoadedTrack.instances[g_dwAirplaneInstance].matrix[row])
#endif

/* Fly the airplane along its path and orient it along the legs of the path. */
static void FlyAirplane(void) {
    Vec3f a, b, c;
    f32 t;
#if TGR_PC
    /* Preserve the negative factor before reverse subtraction, including NaN signs. */
    volatile f32 negativeSpeed;
#endif

    if (g_dwSelectedTrackId == TRACK_MOUNTAIN || g_dwSelectedTrackId == TRACK_MIRROR_MOUNTAIN) {
#if TGR_PC
        negativeSpeed = -18.0f;
#else
        g_fAirplaneDistance += g_fFrameDeltaTime * 18.0f;
#endif
    } else {
#if TGR_PC
        negativeSpeed = -50.0f;
#else
        g_fAirplaneDistance += g_fFrameDeltaTime * 50.0f;
#endif
    }
#if TGR_PC
    g_fAirplaneDistance =
        (long double)g_fAirplaneDistance - (long double)g_fFrameDeltaTime * negativeSpeed;
#endif
    while (g_fAirplaneSegmentLength < g_fAirplaneDistance) {
        g_fAirplaneDistance -= g_fAirplaneSegmentLength;
        g_dwAirplaneNode++;
        if (g_dwAirplaneNode >= g_dwAirplaneNodeCount) {
            g_dwAirplaneInstance = 0;
            break;
        }
        Vec3fMidpoint(&a,
                      &g_pAirplanePathLeft[g_dwAirplaneNode - 1],
                      &g_pAirplanePathRight[g_dwAirplaneNode - 1]);
        Vec3fMidpoint(
            &b, &g_pAirplanePathLeft[g_dwAirplaneNode], &g_pAirplanePathRight[g_dwAirplaneNode]);
        g_fAirplaneSegmentLength = Vec3fDistance(&a, &b);
        Vec3fSubtract(&g_vAirplaneDir, &b, &a);
        Vec3fNormalize(&g_vAirplaneDir);
        if (g_dwAirplaneNode < 2) {
            g_vAirplaneDirPrev = g_vAirplaneDir;
        } else {
            Vec3fMidpoint(&c,
                          &g_pAirplanePathLeft[g_dwAirplaneNode - 2],
                          &g_pAirplanePathRight[g_dwAirplaneNode - 2]);
            Vec3fSubtract(&g_vAirplaneDirPrev, &a, &c);
            Vec3fNormalize(&g_vAirplaneDirPrev);
        }
        if (g_dwAirplaneNode + 1 == g_dwAirplaneNodeCount) {
            g_vAirplaneDirNext = g_vAirplaneDir;
        } else {
            Vec3fMidpoint(&c,
                          &g_pAirplanePathLeft[g_dwAirplaneNode + 1],
                          &g_pAirplanePathRight[g_dwAirplaneNode + 1]);
            Vec3fSubtract(&g_vAirplaneDirNext, &c, &b);
            Vec3fNormalize(&g_vAirplaneDirNext);
        }
    }
    if (!g_dwAirplaneInstance) {
        return;
    }
#if TGR_PC
    t = (long double)g_fAirplaneDistance / g_fAirplaneSegmentLength;
#else
    t = g_fAirplaneDistance / g_fAirplaneSegmentLength;
#endif
    Vec3fLerp(
        &a, &g_pAirplanePathLeft[g_dwAirplaneNode], &g_pAirplanePathLeft[g_dwAirplaneNode - 1], t);
    Vec3fLerp(&b,
              &g_pAirplanePathRight[g_dwAirplaneNode],
              &g_pAirplanePathRight[g_dwAirplaneNode - 1],
              t);
    Vec3fMidpoint(RACE_INSTANCE_VECTOR(3), &a, &b);
    /* The nose turns from the leg before to the leg after across the middle of each leg. */
    if (0.5f < t) {
        Vec3fLerp(RACE_INSTANCE_VECTOR(0), &g_vAirplaneDirNext, &g_vAirplaneDir, t - 0.5f);
    } else {
        Vec3fLerp(RACE_INSTANCE_VECTOR(0), &g_vAirplaneDir, &g_vAirplaneDirPrev, t + 0.5f);
    }
    Vec3fNormalize(RACE_INSTANCE_VECTOR(0));
    Vec3fSubtract(RACE_INSTANCE_VECTOR(2), &a, &b);
    Vec3fCross(RACE_INSTANCE_VECTOR(1), RACE_INSTANCE_VECTOR(0), RACE_INSTANCE_VECTOR(2));
    Vec3fNormalize(RACE_INSTANCE_VECTOR(1));
    Vec3fCross(RACE_INSTANCE_VECTOR(2), RACE_INSTANCE_VECTOR(1), RACE_INSTANCE_VECTOR(0));
    Vec3fNormalize(RACE_INSTANCE_VECTOR(2));
    Vec3fScaleInPlace(RACE_INSTANCE_VECTOR(0), g_fAirplaneScale);
    Vec3fScaleInPlace(RACE_INSTANCE_VECTOR(2), -g_fAirplaneScale);
    Vec3fScaleInPlace(RACE_INSTANCE_VECTOR(1), g_fAirplaneScale);
}

/* Move the special objects of the track. */
void UpdateTrackSpecials(void) {
    s32 i, j;

    if (g_dwAirplaneInstance && !g_bAirplaneFlying &&
        (!(g_dwRainMode || g_dwSnowMode || g_dwNightMode) ||
         (g_dwSelectedTrackId != TRACK_COASTLINE &&
          g_dwSelectedTrackId != TRACK_MIRROR_COASTLINE))) {
        /* A player over the trigger on the last lap sends the airplane off. */
        for (i = 0; i < (s32)g_dwNumLocalPlayers; i++) {
            for (j = 0; j < VEHICLE_GET(&g_aVehicles[i], GroundInstanceCount); j++) {
                if (VEHICLE_GET(&g_aVehicles[i], Lap) == g_dwAirplaneTriggerLap &&
                    VEHICLE_GET(&g_aVehicles[i], GroundInstances)[j] == g_dwAirplaneTrigger) {
                    g_bAirplaneFlying = TRUE;
                    break;
                }
            }
        }
    }
    for (i = 0; i < g_LoadedTrack.specialCount; i++) {
        switch (g_LoadedTrack.specials[i].type) {
        case TRACK_SPECIAL_SPIN_Z:
            guRotateF(
                RACE_SPECIAL_MATRIX, g_LoadedTrack.specials[i].param.spinAngle, 0.0f, 0.0f, 1.0f);
            goto spin;
        case TRACK_SPECIAL_SPIN_X:
            guRotateF(
                RACE_SPECIAL_MATRIX, g_LoadedTrack.specials[i].param.spinAngle, 1.0f, 0.0f, 0.0f);
            goto spin;
        case TRACK_SPECIAL_SPIN_Y:
            guRotateF(
                RACE_SPECIAL_MATRIX, g_LoadedTrack.specials[i].param.spinAngle, 0.0f, 1.0f, 0.0f);
        spin:
            /* The three spins share the turn of the instance. */
            guMtxCatF(RACE_SPECIAL_MATRIX,
                      g_LoadedTrack.instances[g_LoadedTrack.specials[i].target.instance].matrix,
                      g_LoadedTrack.instances[g_LoadedTrack.specials[i].target.instance].matrix);
            g_LoadedTrack.instances[g_LoadedTrack.specials[i].target.instance].flags &=
                ~TRACK_INSTANCE_UNROTATED;
            break;
        case TRACK_SPECIAL_AIRPLANE:
            if (g_dwAirplaneInstance && g_bAirplaneFlying) {
                FlyAirplane();
            }
            break;
        }
    }
}

#undef RACE_SPECIAL_MATRIX
#undef RACE_INSTANCE_VECTOR
