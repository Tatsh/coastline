#include "common/effects.h"
#include "common/vehiclecount.h"
#include "common/weather.h"

#if TGR_PC
#define EFFECT_VEHICLE(index) g_aRacerSlots[index].GetVehicle()
#define EMIT_EXHAUST(vehicle) (vehicle)->SpawnExhaustParticle()
#define EMIT_SMOKE(vehicle) (vehicle)->SpawnTireSmoke()
#define EMIT_SPRAY(vehicle) (vehicle)->EmitTireSpray()
#else
#define EFFECT_VEHICLE(index) g_aRacerSlots[index].vehicle
#define EMIT_EXHAUST(vehicle) SpawnExhaustParticle(vehicle)
#define EMIT_SMOKE(vehicle) SpawnTireSmoke(vehicle)
#define EMIT_SPRAY(vehicle) EmitTireSpray(vehicle)
#endif /* TGR_PC */

void UpdateAllVehicleEffects(void) {
    /**
     * @ghidraAddress USA: 0x8028c840
     * @ghidraAddress EUR: 0x8028efa0
     * @ghidraAddress JPN: 0x8028e9f0
     * @ghidraAddress BRD3D: 0x10a9bbb8
     * @ghidraAddress BRGlide: 0x10ac2c48
     */
    static s32 g_dwVehicleEffectsInitialized = FALSE;
    s32 i;

    if (!g_dwVehicleEffectsInitialized) {
        InitParticleSystem();
        g_dwVehicleEffectsInitialized = TRUE;
    }
    if (g_dwSnowMode) {
        UpdateExhaustParticles();
        for (i = 0; i < g_dwNumVehicles; i++) {
            if (EFFECT_VEHICLE(i) != NULL) {
                EMIT_EXHAUST(EFFECT_VEHICLE(i));
                EMIT_SPRAY(EFFECT_VEHICLE(i));
            }
        }
    } else if (!g_dwNightMode && !g_dwRainMode) {
        UpdateTireSmoke();
        for (i = 0; i < g_dwNumVehicles; i++) {
            if (EFFECT_VEHICLE(i) != NULL) {
                EMIT_SMOKE(EFFECT_VEHICLE(i));
                EMIT_SPRAY(EFFECT_VEHICLE(i));
            }
        }
    } else {
        for (i = 0; i < g_dwNumVehicles; i++) {
            if (EFFECT_VEHICLE(i) != NULL) {
                EMIT_SPRAY(EFFECT_VEHICLE(i));
            }
        }
    }
}
