#ifndef COASTLINE_EFFECTS_H
#define COASTLINE_EFFECTS_H

/**
 * @file
 * What the wheels and the exhausts of the vehicles throw up: the spray of the tyres, the smoke of
 * the tyres in the dry, and the exhaust in the snow.
 */

#include <PR/ultratypes.h>

#include "vecmath.h"
#include "vehicle.h"

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
 * @ghidraAddress JPN: 0x80367bf0
 * @ghidraAddress EUR: 0x80368f80
 */
extern Particle g_aParticlePool[PARTICLES];

/**
 * The list of the free puffs of the pool.
 *
 * @ghidraAddress USA: 0x8028c830
 * @ghidraAddress JPN: 0x8028e9e0
 * @ghidraAddress EUR: 0x8028ef90
 */
extern u16 g_wParticleFreeListHead;

/**
 * The list of the exhaust.
 *
 * @ghidraAddress USA: 0x8028c834
 * @ghidraAddress JPN: 0x8028e9e4
 * @ghidraAddress EUR: 0x8028ef94
 */
extern u16 g_wExhaustParticleListHead;

/**
 * The list of the tyre smoke off the paved road.
 *
 * @ghidraAddress USA: 0x8028c838
 * @ghidraAddress JPN: 0x8028e9e8
 * @ghidraAddress EUR: 0x8028ef98
 */
extern u16 g_wTireSmokeListHead;

/**
 * The list of the tyre smoke on the paved road.
 *
 * @ghidraAddress USA: 0x8028c83c
 * @ghidraAddress JPN: 0x8028e9ec
 * @ghidraAddress EUR: 0x8028ef9c
 */
extern u16 g_wPavedTireSmokeListHead;

/**
 * The matrix from the world to the screen of the current view, with the axes of the screen as the
 * puffs take them.
 *
 * @ghidraAddress USA: 0x80368a80
 * @ghidraAddress JPN: 0x80369bf0
 * @ghidraAddress EUR: 0x8036af80
 */
extern f32 g_afBillboardMatrix[4][4];

/**
 * The texture of the spray, 32 by 64 texels of 4-bit intensity.
 *
 * @ghidraAddress USA: 0x802a2978
 * @ghidraAddress JPN: 0x802a4bd8
 * @ghidraAddress EUR: 0x802a5138
 */
extern u8 g_aTireSprayTexture[];

/**
 * Take a depth to the units of the depth buffer through the viewport.
 *
 * @param z The depth.
 * @param w The w it is divided by.
 * @return The depth in the units of the depth buffer.
 * @ghidraAddress USA: 0x8023b130
 */
s32 ComputeScreenDepthZ(f32 z, f32 w);

/**
 * Count down to the next puff of exhaust, faster as the engine revs, and blow it out behind the
 * vehicle when the pool has a free puff.
 *
 * @param vehicle The vehicle whose exhaust puffs.
 * @ghidraAddress USA: 0x8023b178
 * @ghidraAddress JPN: 0x8023c188
 * @ghidraAddress EUR: 0x8023c688
 */
void SpawnExhaustParticle(Vehicle *vehicle);

/**
 * Throw the newest segment of the spray of each wheel from the surface under it, while the wheel
 * is on the ground and a few frames after.
 *
 * @param vehicle The vehicle whose wheels throw the spray.
 * @ghidraAddress USA: 0x8023b418
 * @ghidraAddress JPN: 0x8023c428
 * @ghidraAddress EUR: 0x8023c928
 */
void EmitTireSpray(Vehicle *vehicle);

/**
 * Draw the spray of the wheels of every racer, except in the snow away from the tracks that show
 * it there.
 *
 * @ghidraAddress USA: 0x8023bb50
 * @ghidraAddress JPN: 0x8023cb60
 * @ghidraAddress EUR: 0x8023d060
 */
void DrawTireSpray(void);

/**
 * Keep the spray of every racer around its vehicle, move each segment by its velocity, age the
 * segments by one when a wheel begins a new one, and fade the old.
 *
 * @ghidraAddress USA: 0x8023bf60
 * @ghidraAddress JPN: 0x8023cf70
 * @ghidraAddress EUR: 0x8023d470
 */
void MoveTireSpray(void);

/**
 * Count down to the next puff of smoke of each wheel, faster with the speed of the vehicle and the
 * spin of the wheel, and raise one from a wheel on a surface that smokes.
 *
 * @param vehicle The vehicle whose wheels smoke.
 * @ghidraAddress USA: 0x8023c800
 * @ghidraAddress JPN: 0x8023d810
 * @ghidraAddress EUR: 0x8023dd10
 */
void SpawnTireSmoke(Vehicle *vehicle);

/**
 * Drift, grow, and fade the exhaust, and free the puffs that have faded.
 *
 * @ghidraAddress USA: 0x8023cbfc
 * @ghidraAddress JPN: 0x8023dc0c
 * @ghidraAddress EUR: 0x8023e10c
 */
void UpdateExhaustParticles(void);

/**
 * Drift, grow, and fade the tyre smoke, and free the puffs that have faded or fall too fast.
 *
 * @ghidraAddress USA: 0x8023cd60
 * @ghidraAddress JPN: 0x8023dd70
 * @ghidraAddress EUR: 0x8023e270
 */
void UpdateTireSmoke(void);

/**
 * Free every puff of the pool and stop the exhaust of the vehicles.
 *
 * @ghidraAddress USA: 0x8023cf30
 * @ghidraAddress JPN: 0x8023df40
 * @ghidraAddress EUR: 0x8023e440
 */
void InitParticleSystem(void);

/**
 * Update the effects of every racer for the weather: the exhaust in the snow, the tyre smoke in
 * the dry, and the spray in all.
 *
 * @ghidraAddress USA: 0x8023cfc4
 * @ghidraAddress JPN: 0x8023dfd4
 * @ghidraAddress EUR: 0x8023e4d4
 */
void UpdateAllVehicleEffects(void);

/**
 * Draw a list of puffs of the pool as rectangles of the particle texture facing the screen.
 *
 * @param head The index of the first puff.
 * @param red The red of the colour of the puffs, of which the low 8 bits count.
 * @param green The green of the colour.
 * @param blue The blue of the colour.
 * @ghidraAddress USA: 0x8023d134
 * @ghidraAddress JPN: 0x8023e144
 * @ghidraAddress EUR: 0x8023e644
 */
void RenderBillboardParticles(u16 head, s32 red, s32 green, s32 blue);

/**
 * Set g_afBillboardMatrix for the current view, and draw the tyre smoke in the colours of the
 * track, except at night.
 *
 * @ghidraAddress USA: 0x8023d714
 * @ghidraAddress JPN: 0x8023e724
 * @ghidraAddress EUR: 0x8023ec24
 */
void RenderTireSmoke(void);

/**
 * Draw the exhaust in the snow with the matrix RenderTireSmoke() set, then the spray.
 *
 * @ghidraAddress USA: 0x8023dbc0
 * @ghidraAddress JPN: 0x8023ebd0
 * @ghidraAddress EUR: 0x8023f0d0
 */
void RenderExhaustAndTireSpray(void);

#endif
