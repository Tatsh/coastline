#include "common/effects.h"
#include "common/vehicleaccess.h"
#include "common/vehiclecount.h"

#if TGR_PC
/** @ghidraAddress BRD3D: 0x10a99bb0 */
u16 g_wExhaustParticleListHead;
/** @ghidraAddress BRD3D: 0x10a99ba8 */
u16 g_wParticleFreeListHead;
/** @ghidraAddress BRD3D: 0x10a99bb4 */
u16 g_wPavedTireSmokeListHead;
/** @ghidraAddress BRD3D: 0x10a99bac */
u16 g_wTireSmokeListHead;
#endif /* TGR_PC */

#if TGR_PC
#define PARTICLE_ACTIVE_VEHICLES activeVehicles
#else
#define PARTICLE_ACTIVE_VEHICLES g_dwNumActiveVehicles
#endif

void InitParticleSystem(void) {
    s32 i;
#if TGR_PC
    s32 activeVehicles;
#endif

    for (i = 1; i < PARTICLES; i++) {
        g_aParticlePool[i].next = i + 1;
    }
#if TGR_PC
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
