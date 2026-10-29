#ifndef COASTLINE_EFFECTS_H
#define COASTLINE_EFFECTS_H

/**
 * @file
 * What the wheels and the exhausts of the vehicles throw up: the spray of the tyres, the smoke of
 * the tyres in the dry, and the exhaust in the snow.
 */

#include <PR/ultratypes.h>

#include "common/tirespray.h"
#include "common/vecmath.h"
#include "common/vehicle.h"

#ifdef __cplusplus
extern "C" {
#endif

/** A puff of smoke or exhaust, in a list of the pool linked by index. */
typedef struct {
    Vec3f position; /*!< The position of the puff in the world. */
    Vec3f velocity; /*!< How far the puff moves a second at its full opacity; it slows as it
                         fades. */
    f32 size;       /*!< The size, growing as the puff fades. */
    u16 next; /*!< The index of the next puff of its list in g_aParticlePool, or 0 at its end. */
    u8 fade;  /*!< The share of the opacity remaining, from 0 to 255, falling as the puff grows. */
    u8 alpha; /*!< The opacity when the puff began, from 0 to 255; times fade over 256, it gives
                   the opacity drawn. */
} Particle;

/** The puffs of the pool. The first is never used, so that index 0 ends a list. */
#define PARTICLES 256

/**
 * @ghidraAddress USA: 0x80366a80
 * @ghidraAddress EUR: 0x80368f80
 * @ghidraAddress JPN: 0x80367bf0
 * @ghidraAddress BRD3D: 0x10a99bb8
 * @ghidraAddress BRGlide: 0x10ac0c48
 */
extern Particle g_aParticlePool[PARTICLES];

/**
 * The list of the free puffs of the pool.
 *
 * @ghidraAddress USA: 0x8028c830
 * @ghidraAddress EUR: 0x8028ef90
 * @ghidraAddress JPN: 0x8028e9e0
 * @ghidraAddress BRD3D: 0x10a99ba8
 * @ghidraAddress BRGlide: 0x10ac0c38
 */
extern u16 g_wParticleFreeListHead;

/**
 * The list of the exhaust.
 *
 * @ghidraAddress USA: 0x8028c834
 * @ghidraAddress EUR: 0x8028ef94
 * @ghidraAddress JPN: 0x8028e9e4
 * @ghidraAddress BRD3D: 0x10a99bb0
 * @ghidraAddress BRGlide: 0x10ac0c40
 */
extern u16 g_wExhaustParticleListHead;

/**
 * The list of the tyre smoke off the paved road.
 *
 * @ghidraAddress USA: 0x8028c838
 * @ghidraAddress EUR: 0x8028ef98
 * @ghidraAddress JPN: 0x8028e9e8
 * @ghidraAddress BRD3D: 0x10a99bac
 * @ghidraAddress BRGlide: 0x10ac0c3c
 */
extern u16 g_wTireSmokeListHead;

/**
 * The list of the tyre smoke on the paved road.
 *
 * @ghidraAddress USA: 0x8028c83c
 * @ghidraAddress EUR: 0x8028ef9c
 * @ghidraAddress JPN: 0x8028e9ec
 * @ghidraAddress BRD3D: 0x10a99bb4
 * @ghidraAddress BRGlide: 0x10ac0c44
 */
extern u16 g_wPavedTireSmokeListHead;

/**
 * The matrix from the world to the screen of the current view, with the axes of the screen as the
 * puffs take them.
 *
 * @ghidraAddress USA: 0x80368a80
 * @ghidraAddress EUR: 0x8036af80
 * @ghidraAddress JPN: 0x80369bf0
 * @ghidraAddress BRD3D: 0x1039b6d0
 * @ghidraAddress BRGlide: 0x10396eb8
 */
extern f32 g_afBillboardMatrix[4][4];

/**
 * The texture of the spray, 32 by 64 texels of 4-bit intensity.
 *
 * @ghidraAddress USA: 0x802a2978
 * @ghidraAddress EUR: 0x802a5138
 * @ghidraAddress JPN: 0x802a4bd8
 */
extern u8 g_aTireSprayTexture[];

/**
 * Take a depth to the units of the depth buffer through the viewport.
 *
 * @param z The depth.
 * @param w The w it is divided by.
 * @return The depth in the units of the depth buffer.
 * @ghidraAddress USA: 0x8023b130
 * @ghidraAddress EUR: 0x8023c640
 * @ghidraAddress JPN: 0x8023c140
 */
s32 ComputeScreenDepthZ(f32 z, f32 w);

#if TGR_PLATFORM == TGR_PLATFORM_N64
/**
 * Count down to the next puff of exhaust, faster as the engine revs, and blow it out behind the
 * vehicle when the pool has a free puff.
 *
 * @param vehicle The vehicle whose exhaust puffs.
 * @ghidraAddress USA: 0x8023b178
 * @ghidraAddress EUR: 0x8023c688
 * @ghidraAddress JPN: 0x8023c188
 */
void SpawnExhaustParticle(Vehicle *vehicle);
#endif /* TGR_PLATFORM == TGR_PLATFORM_N64 */

#if TGR_PLATFORM == TGR_PLATFORM_N64
/**
 * Throw the newest segment of the spray of each wheel from the surface under it, while the wheel
 * is on the ground and a few frames after.
 *
 * @param vehicle The vehicle whose wheels throw the spray.
 * @ghidraAddress USA: 0x8023b418
 * @ghidraAddress EUR: 0x8023c928
 * @ghidraAddress JPN: 0x8023c428
 */
void EmitTireSpray(Vehicle *vehicle);
#endif /* TGR_PLATFORM == TGR_PLATFORM_N64 */

#if TGR_BOSS_RALLY
/**
 * Draw the supplied racers' tyre spray, including snow only on the coastline tracks.
 *
 * @param slots The racer slots supplying the current vehicle bindings.
 * @ghidraAddress BRD3D: 0x10013a40
 * @ghidraAddress BRGlide: 0x10010fb0
 */
void DrawTireSpray(const RacerSlot *slots);
#else
/**
 * Draw the spray of the wheels of every racer, except in the snow away from the tracks that show
 * it there.
 *
 * @ghidraAddress USA: 0x8023bb50
 * @ghidraAddress EUR: 0x8023d060
 * @ghidraAddress JPN: 0x8023cb60
 */
void DrawTireSpray(void);
#endif /* TGR_BOSS_RALLY */

#if TGR_PLATFORM == TGR_PLATFORM_N64
/**
 * Count down to the next puff of smoke of each wheel, faster with the speed of the vehicle and the
 * spin of the wheel, and raise one from a wheel on a surface that smokes.
 *
 * @param vehicle The vehicle whose wheels smoke.
 * @ghidraAddress USA: 0x8023c800
 * @ghidraAddress EUR: 0x8023dd10
 * @ghidraAddress JPN: 0x8023d810
 */
void SpawnTireSmoke(Vehicle *vehicle);
#endif /* TGR_PLATFORM == TGR_PLATFORM_N64 */

/**
 * Drift, grow, and fade the exhaust, and free the puffs that have faded.
 *
 * @ghidraAddress USA: 0x8023cbfc
 * @ghidraAddress EUR: 0x8023e10c
 * @ghidraAddress JPN: 0x8023dc0c
 * @ghidraAddress BRD3D: 0x1003a200
 * @ghidraAddress BRGlide: 0x10033880
 */
void UpdateExhaustParticles(void);

/**
 * Drift, grow, and fade the tyre smoke, and free the puffs that have faded or fall too fast.
 *
 * @ghidraAddress USA: 0x8023cd60
 * @ghidraAddress EUR: 0x8023e270
 * @ghidraAddress JPN: 0x8023dd70
 * @ghidraAddress BRD3D: 0x1003a340
 * @ghidraAddress BRGlide: 0x100339c0
 */
void UpdateTireSmoke(void);

/**
 * Free every puff of the pool and stop the exhaust of the vehicles.
 *
 * @ghidraAddress USA: 0x8023cf30
 * @ghidraAddress EUR: 0x8023e440
 * @ghidraAddress JPN: 0x8023df40
 * @ghidraAddress BRD3D: 0x1003a4d0
 * @ghidraAddress BRGlide: 0x10033b50
 */
void InitParticleSystem(void);

/**
 * Update the effects of every racer for the weather: the exhaust in the snow, the tyre smoke in
 * the dry, and the spray in all.
 *
 * @ghidraAddress USA: 0x8023cfc4
 * @ghidraAddress EUR: 0x8023e4d4
 * @ghidraAddress JPN: 0x8023dfd4
 * @ghidraAddress BRD3D: 0x1003a530
 * @ghidraAddress BRGlide: 0x10033bb0
 */
void UpdateAllVehicleEffects(void);

#if TGR_BOSS_RALLY
/**
 * Draw snapshot particle puffs as rectangles in the current view.
 *
 * @param viewports The viewport records indexed by the current split-screen view.
 * @param head The first particle index, or zero for an empty list.
 * @param red The red component, of which the low eight bits count.
 * @param green The green component.
 * @param blue The blue component.
 * @ghidraAddress BRD3D: 0x10013d90
 * @ghidraAddress BRGlide: 0x10011300
 */
void RenderBillboardParticles(
    const ViewportRect *viewports, u32 head, s32 red, s32 green, s32 blue);
#else
/**
 * Draw a list of puffs of the pool as rectangles of the particle texture facing the screen.
 *
 * @param head The index of the first puff.
 * @param red The red of the colour of the puffs, of which the low 8 bits count.
 * @param green The green of the colour.
 * @param blue The blue of the colour.
 * @ghidraAddress USA: 0x8023d134
 * @ghidraAddress EUR: 0x8023e644
 * @ghidraAddress JPN: 0x8023e144
 */
void RenderBillboardParticles(u16 head, s32 red, s32 green, s32 blue);
#endif /* TGR_BOSS_RALLY */

#if TGR_BOSS_RALLY
/**
 * Set the billboard matrix and draw snapshot tyre smoke in the track colours, except at night.
 *
 * @param viewports The records of the views supplied to the particle renderer.
 * @ghidraAddress BRD3D: 0x100140e0
 * @ghidraAddress BRGlide: 0x10011650
 */
void RenderTireSmoke(const ViewportRect *viewports);
#else
/**
 * Set g_afBillboardMatrix for the current view, and draw the tyre smoke in the colours of the
 * track, except at night.
 *
 * @ghidraAddress USA: 0x8023d714
 * @ghidraAddress EUR: 0x8023ec24
 * @ghidraAddress JPN: 0x8023e724
 */
void RenderTireSmoke(void);
#endif /* TGR_BOSS_RALLY */

#if TGR_BOSS_RALLY
/**
 * Draw snapshot exhaust in snow, restore particle modes and draw the supplied racers' spray.
 *
 * @param viewports The records of the views for the billboard renderer.
 * @param slots The racers whose tyre spray is drawn.
 * @ghidraAddress BRD3D: 0x10014450
 * @ghidraAddress BRGlide: 0x100119c0
 */
void RenderExhaustAndTireSpray(const ViewportRect *viewports, const RacerSlot *slots);
#else
/**
 * Draw the exhaust in the snow with the matrix RenderTireSmoke() set, then the spray.
 *
 * @ghidraAddress USA: 0x8023dbc0
 * @ghidraAddress EUR: 0x8023f0d0
 * @ghidraAddress JPN: 0x8023ebd0
 */
void RenderExhaustAndTireSpray(void);
#endif /* TGR_BOSS_RALLY */

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_EFFECTS_H */
