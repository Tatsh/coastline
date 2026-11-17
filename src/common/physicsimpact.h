#ifndef COASTLINE_PHYSICSIMPACT_H
#define COASTLINE_PHYSICSIMPACT_H

/**
 * @file
 * The recording of the speed, the contact point, and the crash-sound volume of a collision.
 */

#include "common/physics.h"

#ifdef __cplusplus
extern "C" {
#endif

/** The speed a hit is counted up to for its sound. */
#define MAX_IMPACT_SPEED 27.0f

/**
 * Scale the speed of a hit to the volume of its crash sound.
 *
 * @param speed The speed of the hit along the contact normal, from 0 to MAX_IMPACT_SPEED.
 * @return The crash-sound volume, from 128 at rest to 255 at MAX_IMPACT_SPEED.
 */
static inline u8 ImpactVolume(f32 speed) {
#if TGR_BOSS_RALLY
    /* __ftol truncates through 64 bits, which gives 0 for a NaN speed. */
    return (u32)(s64)(128.0L - (long double)speed * -4.70370388031005859375f);
#else
    return (u32)(speed * 127.0f / MAX_IMPACT_SPEED + 128.0f);
#endif
}

/**
 * Record a hard impact and its contact point in @p body. The Nintendo 64 releases record a speed
 * below 10 as 0 and do not copy the contact point for that speed.
 *
 * @param body The body that hit the track.
 * @param speed The speed of the hit along the contact normal, from 0 to MAX_IMPACT_SPEED. The
 * integer part is stored in the impactSpeed member of @p body.
 */
static inline void RecordImpact(RigidBody *body, f32 speed) {
#if TGR_BOSS_RALLY
    /* __ftol truncates through 64 bits, which gives 0 for a NaN speed. */
    body->impactSpeed = (u32)(s64)speed;
#else
    body->impactSpeed = (u32)speed;
#endif
#if !TGR_BOSS_RALLY
    if (body->impactSpeed < 10) {
        body->impactSpeed = 0;
    } else
#endif
    {
        body->impactPoint.x = g_vCollisionContact.x;
        body->impactPoint.y = g_vCollisionContact.y;
        body->impactPoint.z = g_vCollisionContact.z;
    }
}

/**
 * Retain the loudest impact sound for @p body.
 *
 * @param body The body whose impactVolume member is raised to the volume of the hit when louder.
 * @param speed The speed of the hit along the contact normal, from 0 to MAX_IMPACT_SPEED.
 */
static inline void RecordImpactVolume(RigidBody *body, f32 speed) {
    u8 volume = ImpactVolume(speed);

    /* The original writes the volume back even when it does not change. */
    body->impactVolume = body->impactVolume < volume ? volume : body->impactVolume;
}

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_PHYSICSIMPACT_H */
