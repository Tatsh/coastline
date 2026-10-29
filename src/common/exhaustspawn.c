#include "common/effects.h"
#include "common/frametime.h"

#define RANDOM_SHARE (1.0f / 65535.0f)

#if TGR_BOSS_RALLY
void Vehicle::SpawnExhaustParticle(void) {
    Vehicle *vehicle = this;
    volatile long double negativeRevs, timer, rawSize;
#else
void SpawnExhaustParticle(Vehicle *vehicle) {
#endif
    Particle *particle;
    Vec3f exhaust;
    f32 revs, random, size;
    u16 index;

    random = (f32)(AdvanceRng() & 0x1FFF);
#if TGR_BOSS_RALLY
    negativeRevs = (long double)vehicle->engineRpm * -0.001f;
    timer = (((long double)random * (1.0f / 65536.0f) - negativeRevs) - -1.0f) * g_fFrameDeltaTime +
            vehicle->exhaustTimer;
    vehicle->exhaustTimer = timer;
    /* The spawn test uses the unrounded sum despite the timer store. */
    if (!(timer > 0.25f)) {
#else
    revs = vehicle->engineRpm * 0.001f;
    vehicle->exhaustTimer += g_fFrameDeltaTime * (1.0f + revs + random * (1.0f / 65536.0f));
    if (!(vehicle->exhaustTimer > 0.25f)) {
#endif /* TGR_BOSS_RALLY */
        return;
    }
    index = g_wParticleFreeListHead;
    if (index == 0) {
        return;
    }
#if TGR_BOSS_RALLY
    revs = (long double)vehicle->engineRpm * 0.001f;
#endif
    vehicle->exhaustTimer = 0.0f;
    particle = &g_aParticlePool[index];
    g_wParticleFreeListHead = particle->next;
    particle->next = g_wExhaustParticleListHead;
    g_wExhaustParticleListHead = index;
#if TGR_BOSS_RALLY
    Vec3fScale(&particle->velocity, &vehicle->frame.forward, (f32)(-1.5L - revs));
#else
    Vec3fScale(&particle->velocity, &vehicle->frame.forward, -1.5f - revs);
#endif

    /* The exhaust is behind and above the third wheel, and to its side. */
    Vec3fSubtract(&exhaust, &vehicle->wheelFrames[2].position, &vehicle->frame.forward);
    Vec3fMulAdd(&exhaust, &exhaust, &vehicle->frame.up, 0.2f);
    Vec3fMulAdd(&exhaust, &exhaust, &vehicle->frame.side, 0.2f);
    random = (f32)(AdvanceRng() & 0xFFFF) * RANDOM_SHARE;
    Vec3fSubtract(&particle->position, &vehicle->exhaustPosition, &exhaust);
    Vec3fMulAdd(&particle->position, &exhaust, &particle->position, random * random);
    vehicle->exhaustPosition = exhaust;

#if TGR_BOSS_RALLY
    rawSize = (long double)revs * 0.1f - -1.0f;
    size = rawSize;
    particle->size = rawSize * 0.15f;
    particle->fade = (s64)((1.0L / (Vec3fLength(&vehicle->placedVelocity) + size)) * 255.0f);
#else
    size = 1.0f + revs * 0.1f;
    particle->size = size * 0.15f;
    particle->fade = (u32)(1.0f / (Vec3fLength(&vehicle->placedVelocity) + size) * 255.0f);
#endif /* TGR_BOSS_RALLY */
    particle->alpha = 255;
}
