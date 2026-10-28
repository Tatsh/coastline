#include "win32/particlesnapshot.h"

#include <string.h>

/** @ghidraAddress BRD3D: 0x1039b71c */
const Particle *g_pParticleListData;
/** @ghidraAddress BRD3D: 0x1039b714 */
u16 g_wRenderExhaustParticleListHead;
/** @ghidraAddress BRD3D: 0x1039b718 */
u16 g_wRenderPavedTireSmokeListHead;
/** @ghidraAddress BRD3D: 0x1039b710 */
u16 g_wRenderTireSmokeListHead;

void LoadParticleListHeads(const ParticleSnapshot *snapshot) {
    g_pParticleListData = snapshot->particles;
    g_wRenderExhaustParticleListHead = snapshot->exhaustListHead;
    g_wRenderTireSmokeListHead = snapshot->tireSmokeListHead;
    g_wRenderPavedTireSmokeListHead = snapshot->pavedTireSmokeListHead;
}

void SnapshotParticleState(ParticleSnapshot *snapshot) {
    snapshot->freeListHead = g_wParticleFreeListHead;
    snapshot->exhaustListHead = g_wExhaustParticleListHead;
    snapshot->tireSmokeListHead = g_wTireSmokeListHead;
    snapshot->pavedTireSmokeListHead = g_wPavedTireSmokeListHead;
    memcpy(snapshot->particles, g_aParticlePool, sizeof(snapshot->particles));
}
