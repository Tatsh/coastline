#include "common/effects.h"
#include "common/frametime.h"
#include "common/weather.h"

#define PARTICLE_MIN_SHARE (1.0f / 32.0f)
#define PARTICLE_SHARE (1.0f / 65280.0f)

#if TGR_BOSS_RALLY
typedef long double ParticleReal;
#define PARTICLE_RISE(value) ((value) - -0.8f)
#define PARTICLE_FALLING(value) (!((value) >= -30.0f))
#else
typedef f32 ParticleReal;
#define PARTICLE_RISE(value) ((value) + 0.8f)
#define PARTICLE_FALLING(value) ((value) < -30.0f)
#endif

void UpdateExhaustParticles(void) {
#if !TGR_BOSS_RALLY
    u16 *link;
    s32 index;
    s32 next;
    f32 share;
    f32 grow;

    grow = 0.3f * g_fFrameDeltaTime;
    link = &g_wExhaustParticleListHead;
    index = *link;
    while (index != 0) {
        next = g_aParticlePool[index].next;
        g_aParticlePool[index].size += grow;
        share =
            (f32)(s32)(g_aParticlePool[index].fade * g_aParticlePool[index].alpha) * PARTICLE_SHARE;
        g_aParticlePool[index].position.x =
            (g_vWindVelocity.x + g_aParticlePool[index].velocity.x * share * g_fFrameDeltaTime) +
            g_aParticlePool[index].position.x;
        g_aParticlePool[index].position.y =
            (g_vWindVelocity.y + g_aParticlePool[index].velocity.y * share * g_fFrameDeltaTime) +
            g_aParticlePool[index].position.y;
        g_aParticlePool[index].position.z =
            (g_vWindVelocity.z +
             (g_aParticlePool[index].velocity.z * share + 0.8f) * g_fFrameDeltaTime) +
            g_aParticlePool[index].position.z;
        g_aParticlePool[index].fade =
            (s32)(5.7375f / (g_aParticlePool[index].size * g_aParticlePool[index].size));
        if (share < PARTICLE_MIN_SHARE) {
            *link = g_aParticlePool[index].next;
            g_aParticlePool[index].next = g_wParticleFreeListHead;
            g_wParticleFreeListHead = index;
        } else {
            link = &g_aParticlePool[index].next;
        }
        index = next;
    }
#else
    Particle *particle;
    u16 *link;
    s32 index;
    u16 next;
    /* Preserve x87 rounding before float stores, including subnormal results. */
    volatile ParticleReal growth, size;
    f32 grow, step;
    ParticleReal share;

    growth = (ParticleReal)0.3f * g_fFrameDeltaTime;
    grow = growth;
    link = &g_wExhaustParticleListHead;
    for (index = *link; index != 0; index = next) {
        particle = &g_aParticlePool[index];
        step = g_fFrameDeltaTime;
        next = particle->next;
        share = (ParticleReal)(particle->alpha * particle->fade) * PARTICLE_SHARE;
        size = (ParticleReal)grow + particle->size;
        particle->size = size;
        particle->position.x =
            particle->velocity.x * share * step + g_vWindVelocity.x + particle->position.x;
        particle->position.y =
            particle->velocity.y * share * step + g_vWindVelocity.y + particle->position.y;
        particle->position.z = PARTICLE_RISE(particle->velocity.z * share) * step +
                               g_vWindVelocity.z + particle->position.z;
        particle->fade =
            (s64)((ParticleReal)5.7375007f / ((ParticleReal)particle->size * particle->size));
        if (share < PARTICLE_MIN_SHARE) {
            *link = particle->next;
            particle->next = g_wParticleFreeListHead;
            g_wParticleFreeListHead = index;
        } else {
            link = &particle->next;
        }
    }
#endif
}

void UpdateTireSmoke(void) {
    Particle *particle;
    u16 *link;
    s32 index;
    u16 next;
    s32 list;
    ParticleReal grow, share;
    f32 step, rise;
#if TGR_BOSS_RALLY
    /* A stored fall prevents negating a NaN while folding the subtraction. */
    volatile ParticleReal fall;
#endif

    grow = (ParticleReal)0.7f * g_fFrameDeltaTime;
    for (list = 0; list < 2; list++) {
        if (list != 0) {
            link = &g_wTireSmokeListHead;
            index = g_wTireSmokeListHead;
        } else {
            link = &g_wPavedTireSmokeListHead;
            index = g_wPavedTireSmokeListHead;
        }
        for (; index != 0; index = next) {
            particle = &g_aParticlePool[index];
            step = g_fFrameDeltaTime;
            rise = particle->velocity.z;
            next = particle->next;
            share = (ParticleReal)(particle->alpha * particle->fade) * PARTICLE_SHARE;
            particle->size += grow;
#if TGR_BOSS_RALLY
            particle->position.x =
                particle->velocity.x * share * step + g_vWindVelocity.x + particle->position.x;
            particle->position.y =
                particle->velocity.y * share * step + g_vWindVelocity.y + particle->position.y;
            particle->position.z =
                PARTICLE_RISE(share * rise) * step + g_vWindVelocity.z + particle->position.z;
            fall = (ParticleReal)step * 19.62f;
            particle->velocity.z = rise - fall;
            particle->fade = (s64)((ParticleReal)102.0f / particle->size);
#else
            particle->position.x += g_vWindVelocity.x + particle->velocity.x * share * step;
            particle->position.y += g_vWindVelocity.y + particle->velocity.y * share * step;
            particle->position.z += g_vWindVelocity.z + (rise * share + 0.8f) * step;
            particle->velocity.z = rise - step * 19.62f;
            particle->fade = (s32)(102.0f / particle->size);
#endif /* TGR_BOSS_RALLY */
            if (share < PARTICLE_MIN_SHARE || PARTICLE_FALLING(particle->velocity.z)) {
                *link = particle->next;
                particle->next = g_wParticleFreeListHead;
                g_wParticleFreeListHead = index;
            } else {
                link = &particle->next;
            }
        }
    }
}
