#include "common/analyzer.h"
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
    f32 thrown[3];
#if TGR_PLATFORM == TGR_PLATFORM_N64
    /* IDO reserves this unused array in the original stack frame. */
    s32 unused[3] TGR_MATCHING_UNUSED;
#endif
    s32 wheel;
    f32 rate;
    RigidBody *bodies[VEHICLE_WHEELS];
    RigidBody *wheelBody;
    s32 particleIndex;
    Particle *particle;
    f32 fade;
    f32 speed;

    if (vehicle->body.skidding != 0 && (speed = vehicle->speedKph) > 40.0f) {
#if TGR_BOSS_RALLY
        rate = (f32)((long double)g_fFrameDeltaTime * 0.5f - (long double)speed * -0.00066006600f);
#else
        rate = g_fFrameDeltaTime * 0.5f + speed * 0.00066006600f;
#endif
        for (wheel = 0; wheel < VEHICLE_WHEELS; wheel++) {
#if TGR_BOSS_RALLY
            timer =
                ((long double)vehicle->body.skidding * 0.03f) * rate + vehicle->smokeTimers[wheel];
            vehicle->smokeTimers[wheel] = (f32)timer;
            /* Compare the unrounded sum after storing the timer. */
            if (!(timer > 0.75f)) {
#else
            vehicle->smokeTimers[wheel] += rate * ((f32)(u32)vehicle->body.skidding * 0.03f);
            if (!(vehicle->smokeTimers[wheel] > 0.75f)) {
#endif
                continue;
            }
            vehicle->smokeTimers[wheel] = 0.0f;
            bodies[0] = &vehicle->wheelBodies[3];
            bodies[1] = &vehicle->wheelBodies[1];
            bodies[2] = &vehicle->wheelBodies[0];
            bodies[3] = &vehicle->wheelBodies[2];
            wheelBody = bodies[wheel];
            COASTLINE_MATCHING_STORE if (wheelBody->contactFrames == 0 ||
                                         (wheelBody->surface != 1 && wheelBody->surface != 2 &&
                                          wheelBody->surface !=
                                              (particleIndex = TRACK_SURFACE_PAVED)) ||
                                         (particleIndex = g_wParticleFreeListHead) == 0) {
                continue;
            }
            particle = &g_aParticlePool[particleIndex];
            g_wParticleFreeListHead = particle->next;
            if (wheelBody->surface == TRACK_SURFACE_PAVED) {
                particle->next = g_wPavedTireSmokeListHead;
                g_wPavedTireSmokeListHead = particleIndex;
            } else {
                particle->next = g_wTireSmokeListHead;
                g_wTireSmokeListHead = particleIndex;
            }

            /* The front wheels throw their smoke out to the side, the rear ones up. */
            Vec3fScale(&particle->velocity, &vehicle->placedVelocity, 0.2f);
            Vec3fMulAddInPlace(&particle->velocity, &vehicle->frame.up, wheel < 2 ? 0.25f : 2.0f);
            if (wheel < 2) {
                Vec3fMulAddInPlace(
                    &particle->velocity, &vehicle->frame.side, wheel != 0 ? -0.5f : 0.5f);
            }
            Vec3fSubtractInPlace(&particle->velocity, &vehicle->frame.forward);
#if TGR_BOSS_RALLY
            fade = (f32)(1.0L - 50.0f / ((long double)vehicle->speedKph - -50.0f));
#else
            fade = 1.0f - 50.0f / (vehicle->speedKph + 50.0f);
#endif
            Vec3fScaleInPlace(&particle->velocity, fade);

            Vec3fSubtract(&particle->position,
                          &vehicle->wheelFrames[wheel].position,
                          &vehicle->frame.forward);
#if TGR_BOSS_RALLY
            groundOffset = (long double)vehicle->groundDistance * -0.5f;
            particle->position.z = (f32)((long double)particle->position.z - groundOffset);
#else
            particle->position.z += vehicle->groundDistance * 0.5f;
#endif
            thrown[0] = particle->position.x;
            thrown[1] = particle->position.y;
            thrown[2] = particle->position.z;
            if (SMOKE_SCATTER(
                    Vec3fDistanceSquared(&particle->position, &vehicle->smokePositions[wheel]))) {
#if TGR_BOSS_RALLY
                SmokeRandom random = (SmokeRandom)(AdvanceRng() & 0xFFFF) * RANDOM_SHARE;
#else
                SmokeRandom random = (s32)(AdvanceRng() & 0xFFFF) * RANDOM_SHARE;
#endif
                Vec3fLerp(&particle->position,
                          &vehicle->smokePositions[wheel],
                          &particle->position,
                          (f32)(random * random));
            }
            vehicle->smokePositions[wheel].x = thrown[0];
            vehicle->smokePositions[wheel].y = thrown[1];
            vehicle->smokePositions[wheel].z = thrown[2];
            particle->size = 0.4f;
            particle->fade = 25;
#if TGR_BOSS_RALLY
            particle->alpha = (u8)(s64)(16.0L - (long double)fade * -167.3f);
#else
            particle->alpha = (s32)(239.0f * fade * 0.7f + 16.0f);
#endif
        }
    }
}
