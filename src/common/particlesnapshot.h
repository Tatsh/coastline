#pragma once

#include "common/effects.h"

/** Captured particle pool and list indices for one race simulation frame. */
typedef struct ParticleSnapshot {
    u16 freeListHead; /*!< The head index of the captured free list, or zero for an empty list. */
    u16 exhaustListHead;   /*!< The head index of captured exhaust, or zero for an empty list. */
    u16 tireSmokeListHead; /*!< The head index of off-road tyre smoke, or zero for an empty list. */
    u16 pavedTireSmokeListHead; /*!< The paved-road smoke head index, or zero for an empty list. */
    Particle particles[PARTICLES]; /*!< The copied particle pool addressed by the list indices. */
} ParticleSnapshot;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The borrowed particle pool used by rendering, valid until the next snapshot selection.
 *
 * @ghidraAddress BRD3D: 0x1039b71c
 * @ghidraAddress BRGlide: 0x10396f04
 */
extern const Particle *g_pParticleListData;

/**
 * The head index of the exhaust particles selected for rendering, or zero for an empty list.
 *
 * @ghidraAddress BRD3D: 0x1039b714
 * @ghidraAddress BRGlide: 0x10396efc
 */
extern u16 g_wRenderExhaustParticleListHead;

/**
 * The head index of the off-road tyre smoke selected for rendering, or zero for an empty list.
 *
 * @ghidraAddress BRD3D: 0x1039b710
 * @ghidraAddress BRGlide: 0x10396ef8
 */
extern u16 g_wRenderTireSmokeListHead;

/**
 * The head index of the paved-road tyre smoke selected for rendering, or zero for an empty list.
 *
 * @ghidraAddress BRD3D: 0x1039b718
 * @ghidraAddress BRGlide: 0x10396f00
 */
extern u16 g_wRenderPavedTireSmokeListHead;

/**
 * Select a snapshot's particle pool and visible list heads without copying the pool.
 *
 * @param snapshot Captured particles in storage that remains valid while selected for rendering.
 * @ghidraAddress BRD3D: 0x10013a10
 * @ghidraAddress BRGlide: 0x10010f80
 */
void LoadParticleListHeads(const ParticleSnapshot *snapshot);

/**
 * Copy the entire particle pool and all list heads from the race into a snapshot.
 *
 * @param snapshot Writable snapshot storage that receives the full particle pool.
 * @ghidraAddress BRD3D: 0x1003a610
 * @ghidraAddress BRGlide: 0x10033c90
 */
void SnapshotParticleState(ParticleSnapshot *snapshot);
#ifdef __cplusplus
}
#endif
