#include "common/driving.h"

#define JOLT_MAX_ONCE 2.5f
#define JOLT_MAX 5.0f

void AddVehicleJolt(s32 index, f32 amount) {
    if (JOLT_MAX_ONCE < amount) {
        amount = JOLT_MAX_ONCE;
    }
    /* The index of a computer's vehicle may pass the end of the array. */
    g_afVehicleJolts[index] += amount;
    if (JOLT_MAX < g_afVehicleJolts[index]) {
        g_afVehicleJolts[index] = JOLT_MAX;
    }
}
