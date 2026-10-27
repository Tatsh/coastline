#include "platform/win/particlesnapshot.h"

#include <cstring>

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
    std::memcpy(snapshot->particles, g_aParticlePool, sizeof(snapshot->particles));
}
