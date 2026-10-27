#ifndef COASTLINE_RACE_SPECIALS_H
#define COASTLINE_RACE_SPECIALS_H

#include <PR/gu.h>
#include <PR/os.h>

#include "frametime.h"
#include "game.h"
#include "race.h"
#include "trackselect.h"
#include "vehicleaccess.h"
#include "weather.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#define RACE_SPECIAL_MATRIX g_afCameraRollMatrix
#define RACE_INSTANCE_VECTOR(row) (&g_LoadedTrack.instances[g_dwAirplaneInstance].rows[row].vector)
#else
#define RACE_SPECIAL_MATRIX g_afMvpMatrix
#define RACE_INSTANCE_VECTOR(row)                                                                  \
    ((Vec3f *)g_LoadedTrack.instances[g_dwAirplaneInstance].matrix[row])
#endif

/* Fly the airplane along its path and orient it along the legs of the path. */
static inline void FlyAirplane(void) {
    Vec3f a, b, c;
    f32 t;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    /* Preserve the negative factor before reverse subtraction, including NaN signs. */
    volatile f32 negativeSpeed;
#endif

    if (g_dwSelectedTrackId == TRACK_MOUNTAIN || g_dwSelectedTrackId == TRACK_MIRROR_MOUNTAIN) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        negativeSpeed = -18.0f;
#else
        g_fAirplaneDistance += g_fFrameDeltaTime * 18.0f;
#endif
    } else {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        negativeSpeed = -50.0f;
#else
        g_fAirplaneDistance += g_fFrameDeltaTime * 50.0f;
#endif
    }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
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
#if TGR_PLATFORM == TGR_PLATFORM_WIN
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
static inline void UpdateTrackSpecials(void) {
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

#endif
