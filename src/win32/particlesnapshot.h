#pragma once

#include "common/effects.h"

typedef struct ParticleSnapshot {
    u16 freeListHead;
    u16 exhaustListHead;
    u16 tireSmokeListHead;
    u16 pavedTireSmokeListHead;
    Particle particles[PARTICLES];
} ParticleSnapshot;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ghidraAddress BRD3D: 0x1039b71c
 * @ghidraAddress BRGlide: 0x10396f04
 */
extern const Particle *g_pParticleListData;

/**
 * @ghidraAddress BRD3D: 0x1039b714
 * @ghidraAddress BRGlide: 0x10396efc
 */
extern u16 g_wRenderExhaustParticleListHead;

/**
 * @ghidraAddress BRD3D: 0x1039b710
 * @ghidraAddress BRGlide: 0x10396ef8
 */
extern u16 g_wRenderTireSmokeListHead;

/**
 * @ghidraAddress BRD3D: 0x1039b718
 * @ghidraAddress BRGlide: 0x10396f00
 */
extern u16 g_wRenderPavedTireSmokeListHead;

/**
 * @ghidraAddress BRD3D: 0x10013a10
 * @ghidraAddress BRGlide: 0x10010f80
 */
void LoadParticleListHeads(const ParticleSnapshot *snapshot);

/**
 * @ghidraAddress BRD3D: 0x1003a610
 * @ghidraAddress BRGlide: 0x10033c90
 */
void SnapshotParticleState(ParticleSnapshot *snapshot);
#ifdef __cplusplus
}
#endif
