#include "effects.h"

#include <PR/gbi.h>
#include <PR/gu.h>
#include <PR/os_libc.h>

#include "race.h"
#include "render.h"
#include "system.h"
#include "track.h"
#include "trackselect.h"
#include "weather.h"

/* The vertices of the spray are in 127ths of a unit. */
#define SPRAY_UNITS 127.0f

/* The fixed point of the step of a frame that moves the spray, in seconds. */
#define SPRAY_STEP_SHIFT 12

/* The segment from which the spray fades out, and its texture coordinates across and along. */
#define SPRAY_FADE_SEGMENT 5
#define SPRAY_TEXTURE_NEAR 0x10
#define SPRAY_TEXTURE_FAR 0x410
#define SPRAY_TEXTURE_SEGMENT_SHIFT 11

/* The frames a spray goes on after its wheel leaves the ground. */
#define SPRAY_FRAMES 3.0f

/* How near a puff or a segment must be to the last to be scattered back towards it. */
#define SCATTER_DISTANCE_SQUARED 256.0f

/* The puffs that have faded below this are freed. */
#define PARTICLE_MIN_SHARE (1.0f / 32.0f)

/* The opacity of a puff from the product of its fade and its alpha. */
#define PARTICLE_SHARE (1.0f / 65280.0f)

/* A random number of 16 bits as a fraction. */
#define RANDOM_SHARE (1.0f / 65535.0f)

/** Set the colours of the two drawn vertices of the newest segment of a spray. */
static inline void
SetSprayColors(Vtx *head, s32 red, s32 green, s32 blue, s32 farRed, s32 farGreen, s32 farBlue) {
    head[0].v.cn[0] = red;
    head[0].v.cn[1] = green;
    head[0].v.cn[2] = blue;
    head[1].v.cn[0] = farRed;
    head[1].v.cn[1] = farGreen;
    head[1].v.cn[2] = farBlue;
}

/**
 * Colour the newest segment of a spray by its surface, the track, and the weather; a segment of no
 * spraying surface keeps the colours of the last and is marked as thrown from none.
 */
static inline void ColorSpraySegment(Vehicle *vehicle, s32 wheel) {
    Vtx *head = vehicle->sprayVertices[wheel][0];
    Vtx *last = vehicle->sprayVertices[wheel][1];
    s32 i;

    switch (vehicle->spraySegmentSurfaces[wheel][0]) {
    case 0:
        head[0].v.cn[0] = head[1].v.cn[0] = 0x30;
        head[0].v.cn[1] = head[1].v.cn[1] = 0x20;
        head[0].v.cn[2] = head[1].v.cn[2] = 0x10;
        return;
    case TRACK_SURFACE_PAVED:
        if (g_dwRainMode) {
            SetSprayColors(head, 0x64, 0x68, 0x6C, 0xA0, 0xA8, 0xB0);
            return;
        }
        break;
    case TRACK_SURFACE_SLOW:
        if (g_dwSelectedTrackId == 2 || g_dwSelectedTrackId == 7) {
            if (g_dwSnowMode) {
                SetSprayColors(head, 0x46, 0x64, 0x64, 0xD7, 0xEB, 0xC3);
            } else {
                SetSprayColors(head, 0x32, 0x64, 0x5F, 0xD2, 0xF0, 0xBE);
            }
        } else if (g_dwSelectedTrackId == 3 || g_dwSelectedTrackId == 8) {
            SetSprayColors(head, 0x10, 0x10, 0x00, 0x40, 0x40, 0x10);
        } else {
            SetSprayColors(head, 0x30, 0x18, 0x08, 0x80, 0x60, 0x40);
        }
        if (g_dwRainMode) {
            for (i = 0; i < 3; i++) {
                head[0].v.cn[i] = head[0].v.cn[i] / 2 + 0x20;
                head[1].v.cn[i] = head[1].v.cn[i] / 2 + 0x20;
            }
        } else if (g_dwNightMode) {
            for (i = 0; i < 3; i++) {
                head[0].v.cn[i] >>= 1;
                head[1].v.cn[i] >>= 1;
            }
        }
        return;
    }
    vehicle->spraySegmentSurfaces[wheel][0] = -1;
    head[1].v.cn[0] = last[1].v.cn[0];
    head[1].v.cn[1] = last[1].v.cn[1];
    head[1].v.cn[2] = last[1].v.cn[2];
    head[0].v.cn[0] = last[0].v.cn[0];
    head[0].v.cn[1] = last[0].v.cn[1];
    head[0].v.cn[2] = last[0].v.cn[2];
}

/** Copy a segment of a spray over the next older one. */
static inline void AgeSpraySegment(Vehicle *vehicle, s32 wheel, s32 age) {
    memcpy(vehicle->sprayVertices[wheel][age],
           vehicle->sprayVertices[wheel][age - 1],
           sizeof(vehicle->sprayVertices[wheel][age]));
    memcpy(vehicle->sprayVelocities[wheel][age],
           vehicle->sprayVelocities[wheel][age - 1],
           sizeof(vehicle->sprayVelocities[wheel][age]));
    vehicle->spraySegmentSurfaces[wheel][age] = vehicle->spraySegmentSurfaces[wheel][age - 1];
}

/** The texels of a puff left of or above the screen, from its step a pixel, within its sign. */
static inline s32 OffscreenTexels(s16 offscreen, s16 step) {
    s32 texels;

    if (step < 0) {
        texels = (offscreen * step) >> 7;
        return texels > 0 ? texels : 0;
    }
    texels = (offscreen * step) >> 7;
    return texels < 0 ? texels : 0;
}

/** Clamp a coordinate of the screen, taken to 16 bits, to its left or top edge. */
static inline s32 ClampToScreen(s32 coordinate) {
    s16 clamped = coordinate;

    return clamped > 0 ? clamped : 0;
}

/* Load the particle texture to be drawn as rectangles of the primitive colour. */
#define LoadParticleTexture()                                                                      \
    gDPPipeSync(gDisplayListPtr++);                                                                \
    gDPSetCycleType(gDisplayListPtr++, G_CYC_1CYCLE);                                              \
    gSPTexture(gDisplayListPtr++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);                       \
    gDPSetTextureFilter(gDisplayListPtr++, g_dwTextureFilterMode);                                 \
    gDPSetCombineLERP(gDisplayListPtr++,                                                           \
                      0,                                                                           \
                      0,                                                                           \
                      0,                                                                           \
                      PRIMITIVE,                                                                   \
                      TEXEL0,                                                                      \
                      0,                                                                           \
                      PRIMITIVE,                                                                   \
                      0,                                                                           \
                      0,                                                                           \
                      0,                                                                           \
                      0,                                                                           \
                      PRIMITIVE,                                                                   \
                      TEXEL0,                                                                      \
                      0,                                                                           \
                      PRIMITIVE,                                                                   \
                      0);                                                                          \
    gDPSetTextureImage(gDisplayListPtr++, G_IM_FMT_I, G_IM_SIZ_16b, 1, g_aParticleTexture);        \
    gDPSetTile(gDisplayListPtr++,                                                                  \
               G_IM_FMT_I,                                                                         \
               G_IM_SIZ_16b,                                                                       \
               0,                                                                                  \
               0,                                                                                  \
               G_TX_LOADTILE,                                                                      \
               0,                                                                                  \
               G_TX_WRAP,                                                                          \
               6,                                                                                  \
               G_TX_NOLOD,                                                                         \
               G_TX_WRAP,                                                                          \
               6,                                                                                  \
               G_TX_NOLOD);                                                                        \
    gDPLoadSync(gDisplayListPtr++);                                                                \
    gDPLoadBlock(gDisplayListPtr++, G_TX_LOADTILE, 0, 0, PARTICLE_LOAD_TEXELS, PARTICLE_LOAD_DXT); \
    gDPPipeSync(gDisplayListPtr++);                                                                \
    gDPSetTile(gDisplayListPtr++,                                                                  \
               G_IM_FMT_I,                                                                         \
               G_IM_SIZ_8b,                                                                        \
               8,                                                                                  \
               0,                                                                                  \
               G_TX_RENDERTILE,                                                                    \
               0,                                                                                  \
               G_TX_WRAP,                                                                          \
               6,                                                                                  \
               G_TX_NOLOD,                                                                         \
               G_TX_WRAP,                                                                          \
               6,                                                                                  \
               G_TX_NOLOD);                                                                        \
    gDPSetTileSize(gDisplayListPtr++, G_TX_RENDERTILE, 0, 0, 63 << 2, 63 << 2);                    \
    gDPSetTextureLUT(gDisplayListPtr++, G_TT_NONE);                                                \
    gDPSetTexturePersp(gDisplayListPtr++, G_TP_NONE)

/* Put back the modes the rest of the frame draws with. */
#define RestoreParticleModes()                                                                     \
    gDPPipeSync(gDisplayListPtr++);                                                                \
    gDPSetTexturePersp(gDisplayListPtr++, G_TP_PERSP);                                             \
    gDPSetDepthSource(gDisplayListPtr++, G_ZS_PIXEL);                                              \
    gDPSetColorDither(gDisplayListPtr++, g_dwColorDitherMode);                                     \
    gDPSetAlphaDither(gDisplayListPtr++, g_dwAlphaDitherMode);                                     \
    gDPSetAlphaCompare(gDisplayListPtr++, G_AC_THRESHOLD)

s32 ComputeScreenDepthZ(f32 z, f32 w) {
    return (s32)((z / w * (f32)g_dwViewportScaleZ + (f32)g_dwViewportTransZ) * 32.0f);
}

void SpawnExhaustParticle(Vehicle *vehicle) {
    Particle *particle;
    Vec3f exhaust;
    f32 revs, random, size;
    u16 index;

    random = (f32)(AdvanceRng() & 0x1FFF);
    revs = vehicle->engineRpm * 0.001f;
    vehicle->exhaustTimer += g_fFrameDeltaTime * (1.0f + revs + random * (1.0f / 65536.0f));
    if (!(vehicle->exhaustTimer > 0.25f)) {
        return;
    }
    index = g_wParticleFreeListHead;
    if (index == 0) {
        return;
    }
    vehicle->exhaustTimer = 0.0f;
    particle = &g_aParticlePool[index];
    g_wParticleFreeListHead = particle->next;
    particle->next = g_wExhaustParticleListHead;
    g_wExhaustParticleListHead = index;
    Vec3fScale(&particle->velocity, &vehicle->frame.forward, -1.5f - revs);

    /* The exhaust is behind and above the third wheel, and to its side. */
    Vec3fSubtract(&exhaust, &vehicle->wheelFrames[2].position, &vehicle->frame.forward);
    Vec3fMulAdd(&exhaust, &exhaust, &vehicle->frame.up, 0.2f);
    Vec3fMulAdd(&exhaust, &exhaust, &vehicle->frame.side, 0.2f);
    random = (f32)(AdvanceRng() & 0xFFFF) * RANDOM_SHARE;
    Vec3fSubtract(&particle->position, &vehicle->exhaustPosition, &exhaust);
    Vec3fMulAdd(&particle->position, &exhaust, &particle->position, random * random);
    vehicle->exhaustPosition = exhaust;

    size = 1.0f + revs * 0.1f;
    particle->size = size * 0.15f;
    particle->fade = (u32)(1.0f / (Vec3fLength(&vehicle->placedVelocity) + size) * 255.0f);
    particle->alpha = 255;
}

void EmitTireSpray(Vehicle *vehicle) {
    RigidBody *bodies[VEHICLE_WHEELS];
    Vtx *head;
    s16(*velocities)[3];
    Vec3f velocity, position, thrown;
    f32 speedShare, forwardSpeed, scale, slide, random;
    s32 sheltered, holding, throwing, surface, wheel;

    sheltered = FALSE;
    if (vehicle->groundInstanceCount &&
        (g_LoadedTrack.instances[vehicle->groundInstances[0]].flags & TRACK_INSTANCE_SHELTER)) {
        sheltered = TRUE;
    }
    if (g_dwSnowMode && g_dwSelectedTrackId != 2 && g_dwSelectedTrackId != 7) {
        return;
    }
    bodies[0] = &vehicle->wheelBodies[3];
    bodies[1] = &vehicle->wheelBodies[1];
    bodies[2] = &vehicle->wheelBodies[0];
    bodies[3] = &vehicle->wheelBodies[2];
    speedShare = (1.0f - 50.0f / (vehicle->speedKph + 50.0f)) * 3.0f;
    forwardSpeed = Vec3fDot(&vehicle->frame.forward, &vehicle->placedVelocity);
    if (forwardSpeed < 0.0f) {
        forwardSpeed = -forwardSpeed;
    }

    for (wheel = 0; wheel < VEHICLE_WHEELS; wheel++) {
        /* A new segment begins four times a second, and the spray holds still between. */
        vehicle->sprayPhases[wheel] += g_fFrameDeltaTime * 4.0f;
        if (vehicle->sprayPhases[wheel] > 0.75f) {
            holding = FALSE;
            if (vehicle->sprayPhases[wheel] >= 1.7f) {
                vehicle->sprayPhases[wheel] -= 1.0f;
            } else {
                vehicle->sprayPhases[wheel] = 0.0f;
            }
            vehicle->sprayAdvanced[wheel] = TRUE;
        } else {
            holding = TRUE;
            vehicle->sprayAdvanced[wheel] = FALSE;
        }

        if (bodies[wheel]->contactFrames != 0) {
            vehicle->sprayFramesLeft[wheel] = SPRAY_FRAMES;
            vehicle->spraySurfaces[wheel] = bodies[wheel]->surface;
        } else if (vehicle->sprayFramesLeft[wheel] != 0.0f) {
            vehicle->sprayFramesLeft[wheel] -= 1.0f;
        }
        surface = vehicle->spraySurfaces[wheel];

        throwing = FALSE;
        if (vehicle->sprayFramesLeft[wheel] != 0.0f) {
            if (holding) {
                scale = 1.5f;
                throwing = TRUE;
            } else if (surface == TRACK_SURFACE_SLOW) {
                scale = 2.5f;
                throwing = TRUE;
            } else if (surface == TRACK_SURFACE_PAVED && g_dwRainMode && !sheltered) {
                scale = 1.5f;
                throwing = TRUE;
                /* Water begins a segment every frame for the next seven. The original tests
                 * again that the spray is not holding still. */
                if (!holding) {
                    vehicle->sprayPhases[wheel] = 7.75f;
                }
            }
        }

        if (throwing) {
            if (!holding) {
                Vec3fScale(&velocity, &vehicle->frame.up, scale);
                /* The original tests the surface again for the rest. */
                if (surface != TRACK_SURFACE_PAVED) {
                    Vec3fMulAddInPlace(&velocity,
                                       &vehicle->frame.side,
                                       wheel == 0 ? scale :
                                       wheel < 3  ? -scale :
                                                    scale);
                }
                if (surface != TRACK_SURFACE_PAVED) {
                    Vec3fScaleInPlace(&velocity, speedShare);
                    Vec3fMulAddInPlace(&velocity, &vehicle->placedVelocity, 0.3f);
                    Vec3fMulAddInPlace(&velocity,
                                       &vehicle->frame.side,
                                       Vec3fDot(&vehicle->frame.side, &vehicle->placedVelocity) *
                                           0.5f);
                } else {
                    slide = (1.0f - 25.0f / (2.24f * forwardSpeed + 25.0f)) * 3.0f;
                    Vec3fScaleInPlace(&velocity, slide);
                    if (slide != 0.0f) {
                        if (wheel >= 2) {
                            velocity.z += velocity.z;
                        }
                        Vec3fMulAddInPlace(
                            &velocity,
                            &vehicle->frame.side,
                            Vec3fDot(&vehicle->frame.side, &vehicle->placedVelocity) * 0.3f);
                    }
                }
            }
            Vec3fMulAdd(
                &position, &vehicle->wheelFrames[wheel].position, &vehicle->frame.up, -0.25f);
            if (surface != TRACK_SURFACE_PAVED || wheel < 2) {
                Vec3fMulAddInPlace(
                    &position, &vehicle->frame.side, wheel != 0 && wheel < 3 ? -0.15f : 0.15f);
            }
            thrown = position;
            if (!holding && Vec3fDistanceSquared(&position, &vehicle->sprayPositions[wheel]) <
                                SCATTER_DISTANCE_SQUARED) {
                random = (f32)(AdvanceRng() & 0xFFFF) * RANDOM_SHARE;
                Vec3fLerp(&position, &vehicle->sprayPositions[wheel], &position, random * random);
            }
            vehicle->sprayPositions[wheel] = thrown;
        } else {
            Vec3fClear(&velocity);
            position = vehicle->wheelFrames[wheel].position;
        }

        if (!holding) {
            /* The top edge flies at the velocity, the bottom at half of it along the ground. */
            velocities = vehicle->sprayVelocities[wheel][0];
            velocities[1][0] = (s32)(velocity.x * SPRAY_UNITS);
            velocities[1][1] = (s32)(velocity.y * SPRAY_UNITS);
            velocities[1][2] = (s32)(velocity.z * SPRAY_UNITS);
            if (surface == TRACK_SURFACE_PAVED && g_dwRainMode) {
                velocities[0][2] = 0;
                velocities[0][0] = velocities[1][0];
                velocities[0][1] = velocities[1][1];
            } else {
                velocities[0][2] = 0;
                velocities[0][0] = velocities[1][0] >> 1;
                velocities[0][1] = velocities[1][1] >> 1;
            }
        }

        vehicle->spraySegmentSurfaces[wheel][0] = surface;
        head = vehicle->sprayVertices[wheel][0];
        head[1].v.ob[0] = (s32)((position.x - vehicle->sprayOrigin.x) * SPRAY_UNITS);
        head[1].v.ob[1] = (s32)((position.y - vehicle->sprayOrigin.y) * SPRAY_UNITS);
        head[1].v.ob[2] = (s32)((position.z - vehicle->sprayOrigin.z) * SPRAY_UNITS);
        head[0].v.ob[0] = head[1].v.ob[0];
        head[0].v.ob[1] = head[1].v.ob[1];
        head[0].v.ob[2] = head[1].v.ob[2];
    }
}

void DrawTireSpray(void) {
    RacerSlot *slot;
    Vehicle *vehicle;
    s16(*velocities)[SPRAY_SEGMENT_VERTICES][3];
    s32 wheel, first;

    if (g_dwSnowMode && g_dwSelectedTrackId != 2 && g_dwSelectedTrackId != 7) {
        return;
    }
    gDPPipeSync(gDisplayListPtr++);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_2CYCLE);
    gDPSetCombineLERP(gDisplayListPtr++,
                      0,
                      0,
                      0,
                      SHADE,
                      TEXEL0,
                      0,
                      SHADE,
                      0,
                      0,
                      0,
                      0,
                      COMBINED,
                      0,
                      0,
                      0,
                      COMBINED);
    gDPSetRenderMode(gDisplayListPtr++,
                     GBL_c1(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1),
                     Z_CMP | IM_RD | CVG_DST_FULL | ZMODE_XLU | FORCE_BL |
                         GBL_c2(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA));
    gSPSetGeometryMode(gDisplayListPtr++, G_SHADE);
    gSPClearGeometryMode(gDisplayListPtr++, G_CULL_FRONT | G_CULL_BACK | G_LIGHTING);
    gSPMatrix(gDisplayListPtr++, g_pProjectionMtx, G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    gDPSetTextureImage(gDisplayListPtr++, G_IM_FMT_I, G_IM_SIZ_16b, 1, g_aTireSprayTexture);
    gDPSetTile(gDisplayListPtr++,
               G_IM_FMT_I,
               G_IM_SIZ_16b,
               0,
               0,
               G_TX_LOADTILE,
               0,
               G_TX_WRAP,
               6,
               G_TX_NOLOD,
               G_TX_WRAP,
               5,
               G_TX_NOLOD);
    gDPLoadSync(gDisplayListPtr++);
    gDPLoadBlock(gDisplayListPtr++, G_TX_LOADTILE, 0, 0, 511, 0x400);
    gDPPipeSync(gDisplayListPtr++);
    gDPSetTile(gDisplayListPtr++,
               G_IM_FMT_I,
               G_IM_SIZ_4b,
               2,
               0,
               G_TX_RENDERTILE,
               0,
               G_TX_WRAP,
               6,
               G_TX_NOLOD,
               G_TX_WRAP,
               5,
               G_TX_NOLOD);
    gDPSetTileSize(gDisplayListPtr++, G_TX_RENDERTILE, 0, 0, 31 << 2, 63 << 2);
    /* The size is set again half a texel in, which replaces the first. */
    gDPSetTileSize(gDisplayListPtr++, G_TX_RENDERTILE, 2, 2, (31 << 2) + 2, (63 << 2) + 2);
    gSPTexture(gDisplayListPtr++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);

    for (slot = g_aRacerSlots; slot < &g_aRacerSlots[g_dwNumVehicles]; slot++) {
        vehicle = slot->vehicle;
        if (vehicle == NULL) {
            continue;
        }
        gSPMatrix(gDisplayListPtr++, &vehicle->sprayMtx, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_PUSH);
        for (wheel = 0; wheel < VEHICLE_WHEELS; wheel++) {
            /* The newest segment is not drawn, nor a quad whose two edges have stopped. */
            gSPVertex(gDisplayListPtr++,
                      vehicle->sprayVertices[wheel][1],
                      (SPRAY_SEGMENTS - 1) * SPRAY_SEGMENT_VERTICES,
                      0);
            velocities = &vehicle->sprayVelocities[wheel][1];
            for (first = 0; first < (SPRAY_SEGMENTS - 2) * SPRAY_SEGMENT_VERTICES;
                 first += SPRAY_SEGMENT_VERTICES, velocities++) {
                if (velocities[0][1][0] == 0 && velocities[0][1][1] == 0 &&
                    velocities[0][1][2] == 0 && velocities[1][1][0] == 0 &&
                    velocities[1][1][1] == 0 && velocities[1][1][2] == 0) {
                    continue;
                }
                gSP2Triangles(gDisplayListPtr++,
                              first,
                              first + SPRAY_SEGMENT_VERTICES,
                              first + 1,
                              0,
                              first + 1,
                              first + SPRAY_SEGMENT_VERTICES,
                              first + SPRAY_SEGMENT_VERTICES + 1,
                              0);
            }
        }
        gSPPopMatrix(gDisplayListPtr++, G_MTX_MODELVIEW);
    }
    gSPSetGeometryMode(gDisplayListPtr++, G_CULL_BACK);
}

void MoveTireSpray(void) {
    RacerSlot *slot;
    Vehicle *vehicle;
    Mtx scale;
    Vtx *segment;
    s16(*velocities)[3];
    s32 step, wheel, age, alpha;
    s16 fall, shiftX, shiftY, shiftZ, surface;

    step = (s32)(g_fFrameDeltaTime * (1 << SPRAY_STEP_SHIFT));
    fall = (s32)(9.81f * g_fFrameDeltaTime * SPRAY_UNITS);
    if (g_dwRacePauseState != 0) {
        return;
    }
    for (slot = g_aRacerSlots; slot < &g_aRacerSlots[g_dwNumVehicles]; slot++) {
        vehicle = slot->vehicle;
        if (vehicle == NULL) {
            continue;
        }

        /* The origin moves to the vehicle in whole units of the vertices, which move back. */
        shiftX = (s32)((vehicle->sprayOrigin.x - vehicle->frame.position.x) * SPRAY_UNITS);
        shiftY = (s32)((vehicle->sprayOrigin.y - vehicle->frame.position.y) * SPRAY_UNITS);
        shiftZ = (s32)((vehicle->sprayOrigin.z - vehicle->frame.position.z) * SPRAY_UNITS);
        vehicle->sprayOrigin.x -= (f32)shiftX * (1.0f / SPRAY_UNITS);
        vehicle->sprayOrigin.y -= (f32)shiftY * (1.0f / SPRAY_UNITS);
        vehicle->sprayOrigin.z -= (f32)shiftZ * (1.0f / SPRAY_UNITS);
        guTranslate(&vehicle->sprayMtx,
                    vehicle->sprayOrigin.x,
                    vehicle->sprayOrigin.y,
                    vehicle->sprayOrigin.z);
        guScale(&scale, 1.0f / SPRAY_UNITS, 1.0f / SPRAY_UNITS, 1.0f / SPRAY_UNITS);
        guMtxCatL(&scale, &vehicle->sprayMtx, &vehicle->sprayMtx);

        for (wheel = 0; wheel < VEHICLE_WHEELS; wheel++) {
            if (vehicle->sprayAdvanced[wheel]) {
                ColorSpraySegment(vehicle, wheel);
                AgeSpraySegment(vehicle, wheel, 1);
            }
            /* The second segment follows the newest every frame, and is aged again here. */
            for (age = SPRAY_SEGMENTS - 1; age != 0; age--) {
                if (age == 1 || vehicle->sprayAdvanced[wheel]) {
                    AgeSpraySegment(vehicle, wheel, age);
                }
                surface = vehicle->spraySegmentSurfaces[wheel][age];
                segment = vehicle->sprayVertices[wheel][age];
                if (surface == -1) {
                    alpha = 0;
                } else if (age >= SPRAY_FADE_SEGMENT) {
                    alpha = 255 * (SPRAY_SEGMENTS - 1 - age) / 4;
                } else {
                    alpha = 255;
                }
                segment[0].v.cn[3] = alpha;
                segment[1].v.cn[3] = alpha;
                if (surface == TRACK_SURFACE_PAVED) {
                    segment[0].v.tc[0] = SPRAY_TEXTURE_FAR;
                    segment[1].v.tc[0] = SPRAY_TEXTURE_NEAR;
                } else {
                    segment[0].v.tc[0] = SPRAY_TEXTURE_NEAR;
                    segment[1].v.tc[0] = SPRAY_TEXTURE_FAR;
                }
                segment[0].v.tc[1] = age << SPRAY_TEXTURE_SEGMENT_SHIFT;
                segment[1].v.tc[1] = age << SPRAY_TEXTURE_SEGMENT_SHIFT;

                /*
                 * The newest segments and those thrown from nothing only follow the origin. The
                 * oldest of the others does not follow it, since it is drawn clear.
                 */
                if (surface == -1 || age == 1 || (age == 2 && vehicle->sprayAdvanced[wheel])) {
                    segment[0].v.ob[0] += shiftX;
                    segment[0].v.ob[1] += shiftY;
                    segment[0].v.ob[2] += shiftZ;
                    segment[1].v.ob[0] += shiftX;
                    segment[1].v.ob[1] += shiftY;
                    segment[1].v.ob[2] += shiftZ;
                } else if (age != SPRAY_SEGMENTS - 1) {
                    velocities = vehicle->sprayVelocities[wheel][age];
                    segment[0].v.ob[0] = segment[0].v.ob[0] +
                                         ((velocities[0][0] * step) >> SPRAY_STEP_SHIFT) + shiftX;
                    segment[0].v.ob[1] = segment[0].v.ob[1] +
                                         ((velocities[0][1] * step) >> SPRAY_STEP_SHIFT) + shiftY;
                    segment[0].v.ob[2] += shiftZ;
                    segment[1].v.ob[0] = segment[1].v.ob[0] +
                                         ((velocities[1][0] * step) >> SPRAY_STEP_SHIFT) + shiftX;
                    segment[1].v.ob[1] = segment[1].v.ob[1] +
                                         ((velocities[1][1] * step) >> SPRAY_STEP_SHIFT) + shiftY;
                    segment[1].v.ob[2] = segment[1].v.ob[2] +
                                         ((velocities[1][2] * step) >> SPRAY_STEP_SHIFT) + shiftZ;
                    /* The top edge falls until it lands on the bottom one. */
                    if (segment[1].v.ob[2] < segment[0].v.ob[2]) {
                        segment[1].v.ob[2] = segment[0].v.ob[2];
                    } else {
                        velocities[1][2] -= fall;
                    }
                }
            }
        }
    }
}

void SpawnTireSmoke(Vehicle *vehicle) {
    RigidBody *bodies[VEHICLE_WHEELS];
    RigidBody *body;
    Particle *particle;
    Vec3f thrown;
    f32 rate, fade, random;
    s32 wheel;
    u16 index;

    if (vehicle->body.skidding == 0 || !(vehicle->speedKph > 40.0f)) {
        return;
    }
    rate = g_fFrameDeltaTime * 0.5f + vehicle->speedKph * 0.00066006600f;
    for (wheel = 0; wheel < VEHICLE_WHEELS; wheel++) {
        vehicle->smokeTimers[wheel] += rate * ((f32)vehicle->body.skidding * 0.03f);
        if (!(vehicle->smokeTimers[wheel] > 0.75f)) {
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
        fade = 1.0f - 50.0f / (vehicle->speedKph + 50.0f);
        Vec3fScaleInPlace(&particle->velocity, fade);

        Vec3fSubtract(
            &particle->position, &vehicle->wheelFrames[wheel].position, &vehicle->frame.forward);
        particle->position.z += vehicle->groundDistance * 0.5f;
        thrown = particle->position;
        if (Vec3fDistanceSquared(&particle->position, &vehicle->smokePositions[wheel]) <
            SCATTER_DISTANCE_SQUARED) {
            random = (f32)(AdvanceRng() & 0xFFFF) * RANDOM_SHARE;
            Vec3fLerp(&particle->position,
                      &vehicle->smokePositions[wheel],
                      &particle->position,
                      random * random);
        }
        vehicle->smokePositions[wheel] = thrown;
        particle->size = 0.4f;
        particle->fade = 25;
        particle->alpha = (s32)(239.0f * fade * 0.7f + 16.0f);
    }
}

void UpdateExhaustParticles(void) {
    Particle *particle;
    u16 *link;
    s32 index;
    u16 next;
    f32 grow, share, step;

    grow = 0.3f * g_fFrameDeltaTime;
    link = &g_wExhaustParticleListHead;
    for (index = *link; index != 0; index = next) {
        particle = &g_aParticlePool[index];
        step = g_fFrameDeltaTime;
        next = particle->next;
        share = (f32)(particle->alpha * particle->fade) * PARTICLE_SHARE;
        particle->size += grow;
        particle->position.x += g_vWindVelocity.x + particle->velocity.x * share * step;
        particle->position.y += g_vWindVelocity.y + particle->velocity.y * share * step;
        particle->position.z += g_vWindVelocity.z + (particle->velocity.z * share + 0.8f) * step;
        particle->fade = (s32)(5.7375f / (particle->size * particle->size));
        if (share < PARTICLE_MIN_SHARE) {
            *link = particle->next;
            particle->next = g_wParticleFreeListHead;
            g_wParticleFreeListHead = index;
        } else {
            link = &particle->next;
        }
    }
}

void UpdateTireSmoke(void) {
    Particle *particle;
    u16 *link;
    s32 index;
    u16 next;
    s32 list;
    f32 grow, share, step, rise;

    grow = 0.7f * g_fFrameDeltaTime;
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
            share = (f32)(particle->alpha * particle->fade) * PARTICLE_SHARE;
            particle->size += grow;
            particle->position.x += g_vWindVelocity.x + particle->velocity.x * share * step;
            particle->position.y += g_vWindVelocity.y + particle->velocity.y * share * step;
            particle->position.z += g_vWindVelocity.z + (rise * share + 0.8f) * step;
            particle->velocity.z = rise - step * 19.62f;
            particle->fade = (s32)(102.0f / particle->size);
            if (share < PARTICLE_MIN_SHARE || particle->velocity.z < -30.0f) {
                *link = particle->next;
                particle->next = g_wParticleFreeListHead;
                g_wParticleFreeListHead = index;
            } else {
                link = &particle->next;
            }
        }
    }
}

void InitParticleSystem(void) {
    s32 i;

    for (i = 1; i < PARTICLES; i++) {
        g_aParticlePool[i].next = i + 1;
    }
    /* The loop links the last puff past the pool, and this ends the list there. */
    g_aParticlePool[PARTICLES - 1].next = 0;
    g_wParticleFreeListHead = 1;
    for (i = 0; i < g_dwNumActiveVehicles; i++) {
        g_aVehicles[i].exhaustTimer = 0.0f;
    }
    g_wExhaustParticleListHead = 0;
    g_wTireSmokeListHead = 0;
    g_wPavedTireSmokeListHead = 0;
}

void UpdateAllVehicleEffects(void) {
    /* Whether InitParticleSystem() has run. */
    /**
     * @ghidraAddress USA: 0x8028c840
     * @ghidraAddress JPN: 0x8028e9f0
     * @ghidraAddress EUR: 0x8028efa0
     */
    static s32 g_dwVehicleEffectsInitialized = FALSE;
    s32 i;

    if (!g_dwVehicleEffectsInitialized) {
        InitParticleSystem();
        g_dwVehicleEffectsInitialized = TRUE;
    }
    if (g_dwSnowMode) {
        UpdateExhaustParticles();
        for (i = 0; i < g_dwNumVehicles; i++) {
            if (g_aRacerSlots[i].vehicle != NULL) {
                SpawnExhaustParticle(g_aRacerSlots[i].vehicle);
                EmitTireSpray(g_aRacerSlots[i].vehicle);
            }
        }
    } else if (!g_dwNightMode && !g_dwRainMode) {
        UpdateTireSmoke();
        for (i = 0; i < g_dwNumVehicles; i++) {
            if (g_aRacerSlots[i].vehicle != NULL) {
                SpawnTireSmoke(g_aRacerSlots[i].vehicle);
                EmitTireSpray(g_aRacerSlots[i].vehicle);
            }
        }
    } else {
        for (i = 0; i < g_dwNumVehicles; i++) {
            if (g_aRacerSlots[i].vehicle != NULL) {
                EmitTireSpray(g_aRacerSlots[i].vehicle);
            }
        }
    }
}

void RenderBillboardParticles(u16 head, s32 red, s32 green, s32 blue) {
    ViewportRect *view;
    Particle *particle;
    f32 clip[4];
    f32 span, scaleX, inverse, size, edge;
    s32 centerX, centerY, height, halfWidth, halfHeight, x, y, top, stepS, stepT, s, t;
    s16 left;
    u16 index;

    view = &g_aViewportRects[g_dwCurrentSplitScreenIndex];
    centerX = (view->x * 2 + view->width) * 2;
    centerY = (view->y * 2 + view->height) * 2;
    span = (f32)(view->width * 2);
    height = view->height;
    scaleX = g_dwViewportMirrored != g_dwTrackMirrored ? -span : span;

    for (index = head; index != 0; index = particle->next) {
        particle = &g_aParticlePool[index];
        Vec3fTransformByMatrix(clip, &particle->position, g_afBillboardMatrix);
        if (clip[3] <= 0.001f && clip[3] >= -0.001f) {
            continue;
        }
        inverse = 1.0f / clip[3];
        clip[2] *= inverse;
        if (clip[2] >= 1.0f || clip[2] <= -0.3f) {
            continue;
        }
        clip[0] *= inverse;
        size = particle->size * (4.0f * inverse);
        edge = 1.0f + size;
        if (clip[0] <= -edge || clip[0] >= edge) {
            continue;
        }
        clip[1] *= inverse;
        if (clip[1] <= -edge || clip[1] >= edge) {
            continue;
        }
        /* The width is three quarters of the height for the shape of the screen. */
        halfWidth = (s32)(span * 0.75f * size);
        if (halfWidth <= 0) {
            continue;
        }
        halfHeight = (s32)(span * size);
        if (halfHeight <= 0) {
            continue;
        }
        x = (s32)(clip[0] * scaleX) + centerX;
        y = (s32)(clip[1] * (f32)(height * 2)) + centerY;

        gDPPipeSync(gDisplayListPtr++);
        gDPSetPrimColor(gDisplayListPtr++,
                        0xFF,
                        0xFF,
                        red,
                        green,
                        blue,
                        (particle->alpha * particle->fade) >> 8);
        gDPSetPrimDepth(gDisplayListPtr++, (s32)(clip[2] * 16352.0f) + 0x3FE0, 0);

        /* A rectangle off the left or the top of the screen starts its texture further in. */
        left = x - halfWidth;
        top = y - halfHeight;
        stepS = PARTICLE_TEXTURE_STEPS / 2 / halfWidth;
        stepT = -(PARTICLE_TEXTURE_STEPS / 2) / halfHeight;
        s = left < 0 ? OffscreenTexels(left, stepS) : 0;
        t = top < 0 ? OffscreenTexels(top, stepT) : 0;
        gSPTextureRectangle(gDisplayListPtr++,
                            ClampToScreen(left),
                            ClampToScreen(top),
                            ClampToScreen(x + halfWidth),
                            ClampToScreen(y + halfHeight),
                            G_TX_RENDERTILE,
                            -s,
                            PARTICLE_TEXTURE_LAST_T - t,
                            stepS,
                            stepT);
    }
}

void RenderTireSmoke(void) {
    LoadParticleTexture();

    /* The view, then the axes of a puff to those of the screen, then the perspective. */
    MtxInvert4x4(g_afMvpMatrix, (f32(*)[4])g_pActiveCamera);
    g_afScratchMatrix[0][0] = 0.0f;
    g_afScratchMatrix[0][1] = 0.0f;
    g_afScratchMatrix[0][2] = -1.0f;
    g_afScratchMatrix[0][3] = 0.0f;
    g_afScratchMatrix[1][0] = -1.0f;
    g_afScratchMatrix[1][1] = 0.0f;
    g_afScratchMatrix[1][2] = 0.0f;
    g_afScratchMatrix[1][3] = 0.0f;
    g_afScratchMatrix[2][0] = 0.0f;
    g_afScratchMatrix[2][1] = -1.0f;
    g_afScratchMatrix[2][2] = 0.0f;
    g_afScratchMatrix[2][3] = 0.0f;
    g_afScratchMatrix[3][0] = 0.0f;
    g_afScratchMatrix[3][1] = 0.0f;
    g_afScratchMatrix[3][2] = 0.0f;
    g_afScratchMatrix[3][3] = 1.0f;
    Mtx4x4Multiply(g_afMvpMatrix, g_afMvpMatrix, g_afScratchMatrix);
    /* The copy is not read again. */
    memcpy(g_afScratchMatrix, g_afMvpMatrix, sizeof(g_afScratchMatrix));
    Mtx4x4Multiply(g_afBillboardMatrix, g_afMvpMatrix, g_afPerspectiveMatrix);

    gDPSetDepthSource(gDisplayListPtr++, G_ZS_PRIM);
    gDPSetColorDither(gDisplayListPtr++, G_CD_DISABLE);
    if (!g_dwNightMode) {
        /* The colour dither's bits, which leave the alpha dither at its pattern. */
        gDPSetAlphaDither(gDisplayListPtr++, G_CD_DISABLE);
        /* The alpha compare is set to dither the smoke along with the render mode. */
        gDPSetRenderMode(gDisplayListPtr++,
                         Z_CMP | Z_UPD | CVG_DST_FULL | ZMODE_OPA | G_AC_DITHER |
                             GBL_c1(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1),
                         GBL_c2(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1));
        gDPSetBlendColor(gDisplayListPtr++, 0, 0, 0, 0);
        switch (g_dwSelectedTrackId) {
        case 1:
        case 6:
            RenderBillboardParticles(g_wTireSmokeListHead, 0x60, 0x54, 0x38);
            RenderBillboardParticles(g_wPavedTireSmokeListHead, 0x60, 0x5C, 0x50);
            break;
        case 4:
        case 9:
            RenderBillboardParticles(g_wTireSmokeListHead, 0x70, 0x58, 0x38);
            RenderBillboardParticles(g_wPavedTireSmokeListHead, 0x70, 0x68, 0x58);
            break;
        default:
            RenderBillboardParticles(g_wTireSmokeListHead, 0xA0, 0x88, 0x60);
            RenderBillboardParticles(g_wPavedTireSmokeListHead, 0x70, 0x68, 0x58);
            break;
        }
    }
    RestoreParticleModes();
}

void RenderExhaustAndTireSpray(void) {
    LoadParticleTexture();
    gDPSetDepthSource(gDisplayListPtr++, G_ZS_PRIM);
    gDPSetColorDither(gDisplayListPtr++, G_CD_DISABLE);
    if (g_dwSnowMode) {
        /* The colour dither's bits, which leave the alpha dither at its pattern. */
        gDPSetAlphaDither(gDisplayListPtr++, G_CD_NOISE);
        gDPSetRenderMode(gDisplayListPtr++,
                         Z_CMP | IM_RD | CVG_DST_SAVE | ZMODE_XLU | FORCE_BL |
                             GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA),
                         GBL_c2(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA));
        RenderBillboardParticles(g_wExhaustParticleListHead, 0xE0, 0xE0, 0xFF);
    }
    RestoreParticleModes();
    DrawTireSpray();
}
