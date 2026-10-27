#pragma once

#include "effects.h"

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
 */
extern const Particle *g_pParticleListData;

/**
 * @ghidraAddress BRD3D: 0x1039b714
 */
extern u16 g_wRenderExhaustParticleListHead;

/**
 * @ghidraAddress BRD3D: 0x1039b710
 */
extern u16 g_wRenderTireSmokeListHead;

/**
 * @ghidraAddress BRD3D: 0x1039b718
 */
extern u16 g_wRenderPavedTireSmokeListHead;

/**
 * @ghidraAddress BRD3D: 0x10013a10
 */
void LoadParticleListHeads(const ParticleSnapshot *snapshot);

/**
 * @ghidraAddress BRD3D: 0x1003a610
 */
void SnapshotParticleState(ParticleSnapshot *snapshot);
#ifdef __cplusplus
}
#endif
