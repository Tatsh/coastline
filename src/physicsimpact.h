#ifndef COASTLINE_PHYSICSIMPACT_H
#define COASTLINE_PHYSICSIMPACT_H

#include "physics.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The speed a hit is counted up to for its sound. */
#define MAX_IMPACT_SPEED 27.0f

/* The loudness of a hit at a speed, which the sound of a crash plays at. */
static inline u8 ImpactVolume(f32 speed) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    return (u32)(128.0L - (long double)speed * -4.70370388031005859375f);
#else
    return (u32)(speed * 127.0f / MAX_IMPACT_SPEED + 128.0f);
#endif
}

/* Record a hard hit at a speed into a surface for its sound, and where it was. */
static inline void RecordImpact(RigidBody *body, f32 speed) {
    body->impactSpeed = (u32)speed;
#if TGR_PLATFORM != TGR_PLATFORM_WIN
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

/* Retain the loudest hit for the sound. */
static inline void RecordImpactVolume(RigidBody *body, f32 speed) {
    u8 volume = ImpactVolume(speed);

    /* The original writes the volume back even when it does not change. */
    body->impactVolume = body->impactVolume < volume ? volume : body->impactVolume;
}

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_PHYSICSIMPACT_H */
