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
    Vec3f exhaust;
    f32 random;
    s32 index;
    Particle *particle;
    f32 revs, size;
    s32 randomBits;

    randomBits = AdvanceRng();
#if TGR_BOSS_RALLY
    random = (f32)(randomBits & 0x1FFF);
    negativeRevs = (long double)vehicle->engineRpm * -0.001f;
    timer = (((long double)random * (1.0f / 65536.0f) - negativeRevs) - -1.0f) * g_fFrameDeltaTime +
            vehicle->exhaustTimer;
    vehicle->exhaustTimer = (f32)timer;
    /* The spawn test uses the unrounded sum despite the timer store. */
    if (timer > 0.25f && (index = g_wParticleFreeListHead) != 0) {
#else
    vehicle->exhaustTimer = vehicle->exhaustTimer +
                            g_fFrameDeltaTime * (1.0f + vehicle->engineRpm * 0.001f +
                                                 (f32)(randomBits & 0x1FFF) * (1.0f / 65536.0f));
    if (vehicle->exhaustTimer > 0.25f && (index = g_wParticleFreeListHead) != 0) {
#endif /* TGR_BOSS_RALLY */
#if TGR_BOSS_RALLY
        revs = (f32)((long double)vehicle->engineRpm * 0.001f);
#else
        revs = vehicle->engineRpm * 0.001f;
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
        randomBits = AdvanceRng();
        random = (f32)(randomBits & 0xFFFF) * RANDOM_SHARE;
        Vec3fSubtract(&particle->position, &vehicle->exhaustPosition, &exhaust);
        Vec3fMulAdd(&particle->position, &exhaust, &particle->position, random * random);
        vehicle->exhaustPosition.x = exhaust.x;
        vehicle->exhaustPosition.y = exhaust.y;
        vehicle->exhaustPosition.z = exhaust.z;

#if TGR_BOSS_RALLY
        rawSize = (long double)revs * 0.1f - -1.0f;
        size = (f32)rawSize;
        particle->size = (f32)(rawSize * 0.15f);
        particle->fade =
            (u8)(s64)((1.0L / (Vec3fLength(&vehicle->placedVelocity) + size)) * 255.0f);
#else
        revs *= 0.1f;
        size = 1.0f + revs;
        particle->size = size * 0.15f;
        particle->fade = (u32)(1.0f / (Vec3fLength(&vehicle->placedVelocity) + size) * 255.0f);
#endif /* TGR_BOSS_RALLY */
        particle->alpha = 255;
    }
}
