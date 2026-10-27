#include "effects.h"
#include "vehicleaccess.h"
#include "vehiclecount.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#define PARTICLE_ACTIVE_VEHICLES activeVehicles
#else
#define PARTICLE_ACTIVE_VEHICLES g_dwNumActiveVehicles
#endif

void InitParticleSystem(void) {
    s32 i;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    s32 activeVehicles;
#endif

    for (i = 1; i < PARTICLES; i++) {
        g_aParticlePool[i].next = i + 1;
    }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    activeVehicles = g_dwNumActiveVehicles;
#endif
    /* The loop links the last puff past the pool, and this ends the list there. */
    g_aParticlePool[PARTICLES - 1].next = 0;
    g_wParticleFreeListHead = 1;
    for (i = 0; i < PARTICLE_ACTIVE_VEHICLES; i++) {
        VEHICLE_SET(&g_aVehicles[i], ExhaustTimer, 0.0f);
    }
    g_wExhaustParticleListHead = 0;
    g_wTireSmokeListHead = 0;
    g_wPavedTireSmokeListHead = 0;
}
