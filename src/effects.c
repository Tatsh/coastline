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
