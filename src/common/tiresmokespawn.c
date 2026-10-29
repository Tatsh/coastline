#include "common/effects.h"
#include "common/frametime.h"

#define SCATTER_DISTANCE_SQUARED 256.0f
#define RANDOM_SHARE (1.0f / 65535.0f)

#if TGR_BOSS_RALLY
typedef long double SmokeRandom;
/* Unordered distances also scatter the puff. */
#define SMOKE_SCATTER(distance) (!((distance) >= SCATTER_DISTANCE_SQUARED))
#else
typedef f32 SmokeRandom;
#define SMOKE_SCATTER(distance) ((distance) < SCATTER_DISTANCE_SQUARED)
#endif

#if TGR_BOSS_RALLY
void Vehicle::SpawnTireSmoke(void) {
    Vehicle *vehicle = this;
    volatile long double timer, groundOffset;
#else
void SpawnTireSmoke(Vehicle *vehicle) {
#endif
    RigidBody *bodies[VEHICLE_WHEELS];
    RigidBody *body;
    Particle *particle;
    Vec3f thrown;
    f32 rate, fade;
    SmokeRandom random;
    s32 wheel;
    u16 index;

    if (vehicle->body.skidding == 0 || !(vehicle->speedKph > 40.0f)) {
        return;
    }
#if TGR_BOSS_RALLY
    rate = (long double)g_fFrameDeltaTime * 0.5f - (long double)vehicle->speedKph * -0.00066006600f;
#else
    rate = g_fFrameDeltaTime * 0.5f + vehicle->speedKph * 0.00066006600f;
#endif
    for (wheel = 0; wheel < VEHICLE_WHEELS; wheel++) {
#if TGR_BOSS_RALLY
        timer = ((long double)vehicle->body.skidding * 0.03f) * rate + vehicle->smokeTimers[wheel];
        vehicle->smokeTimers[wheel] = timer;
        /* Compare the unrounded sum after storing the timer. */
        if (!(timer > 0.75f)) {
#else
        vehicle->smokeTimers[wheel] += rate * ((f32)vehicle->body.skidding * 0.03f);
        if (!(vehicle->smokeTimers[wheel] > 0.75f)) {
#endif
            continue;
        }
        vehicle->smokeTimers[wheel] = 0.0f;
        bodies[0] = &vehicle->wheelBodies[3];
        bodies[1] = &vehicle->wheelBodies[1];
        bodies[2] = &vehicle->wheelBodies[0];
        bodies[3] = &vehicle->wheelBodies[2];
        body = bodies[wheel];
        if (body->contactFrames == 0) {
            continue;
        }
        if (body->surface != 1 && body->surface != 2 && body->surface != TRACK_SURFACE_PAVED) {
            continue;
        }
        index = g_wParticleFreeListHead;
        if (index == 0) {
            continue;
        }
        particle = &g_aParticlePool[index];
        g_wParticleFreeListHead = particle->next;
        if (body->surface == TRACK_SURFACE_PAVED) {
            particle->next = g_wPavedTireSmokeListHead;
            g_wPavedTireSmokeListHead = index;
        } else {
            particle->next = g_wTireSmokeListHead;
            g_wTireSmokeListHead = index;
        }

        /* The front wheels throw their smoke out to the side, the rear ones up. */
        Vec3fScale(&particle->velocity, &vehicle->placedVelocity, 0.2f);
        Vec3fMulAddInPlace(&particle->velocity, &vehicle->frame.up, wheel < 2 ? 0.25f : 2.0f);
        if (wheel < 2) {
            Vec3fMulAddInPlace(
                &particle->velocity, &vehicle->frame.side, wheel == 0 ? 0.5f : -0.5f);
        }
        Vec3fSubtractInPlace(&particle->velocity, &vehicle->frame.forward);
#if TGR_BOSS_RALLY
        fade = 1.0L - 50.0f / ((long double)vehicle->speedKph - -50.0f);
#else
        fade = 1.0f - 50.0f / (vehicle->speedKph + 50.0f);
#endif
        Vec3fScaleInPlace(&particle->velocity, fade);

        Vec3fSubtract(
            &particle->position, &vehicle->wheelFrames[wheel].position, &vehicle->frame.forward);
#if TGR_BOSS_RALLY
        groundOffset = (long double)vehicle->groundDistance * -0.5f;
        particle->position.z = (long double)particle->position.z - groundOffset;
#else
        particle->position.z += vehicle->groundDistance * 0.5f;
#endif
        thrown = particle->position;
        if (SMOKE_SCATTER(
                Vec3fDistanceSquared(&particle->position, &vehicle->smokePositions[wheel]))) {
            random = (SmokeRandom)(AdvanceRng() & 0xFFFF) * RANDOM_SHARE;
            Vec3fLerp(&particle->position,
                      &vehicle->smokePositions[wheel],
                      &particle->position,
                      (f32)(random * random));
        }
        vehicle->smokePositions[wheel] = thrown;
        particle->size = 0.4f;
        particle->fade = 25;
#if TGR_BOSS_RALLY
        particle->alpha = (s64)(16.0L - (long double)fade * -167.3f);
#else
        particle->alpha = (s32)(239.0f * fade * 0.7f + 16.0f);
#endif
    }
}
