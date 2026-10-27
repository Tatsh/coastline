#ifndef COASTLINE_RACE_SPECIAL_INIT_H
#define COASTLINE_RACE_SPECIAL_INIT_H

#include <PR/os.h>

#include "race.h"
#include "track.h"

/* Find the special objects of the track: the airplane, its path and trigger, and the waterfalls. */
static inline void FindTrackSpecials(void) {
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

#endif
