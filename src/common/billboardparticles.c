#if TGR_BOSS_RALLY
#include <PR/gbi.h>

#include "common/effects.h"
#include "common/gbicommand.h"
#include "common/hudcanvas.h"
#include "common/particlesnapshot.h"
#include "common/render.h"
#include "common/system.h"
#include "common/weather.h"
#define BILLBOARD_INDEX u32
#define BILLBOARD_PARTICLES g_pParticleListData
#define BILLBOARD_ADD(a, b) ((s32)((u32)(a) + (u32)(b)))
#define BILLBOARD_SUBTRACT(a, b) ((s32)((u32)(a) - (u32)(b)))

static inline u32 BillboardRectangleCoordinate(s32 coordinate) {
#ifdef ENABLE_PATCHES
    s32 pixel = coordinate >> 2;
    return pixel > 0 ? (u32)pixel : 0;
#else
    s16 pixel = coordinate >> 2;
    return pixel > 0 ? (u32)pixel & 0xfff : 0;
#endif
}

static inline void WriteBillboardRectangle(s32 x, s32 y, s32 halfWidth, s32 halfHeight) {
    u32 right = BillboardRectangleCoordinate(BILLBOARD_ADD(x, halfWidth));
    u32 bottom = BillboardRectangleCoordinate(BILLBOARD_ADD(y, halfHeight));
    u32 left = BillboardRectangleCoordinate(BILLBOARD_SUBTRACT(x, halfWidth));
    u32 top = BillboardRectangleCoordinate(BILLBOARD_SUBTRACT(y, halfHeight));
#ifdef ENABLE_PATCHES
    gDisplayListWords(gDisplayListPtr++, (u32)GBI_OP_FULL_TEXTURE_RECTANGLE << 24, left);
    gDisplayListWords(gDisplayListPtr++, top, right);
    gDisplayListWords(gDisplayListPtr++, bottom, 0);
#else
    gDisplayListWords(gDisplayListPtr++,
                      ((u32)GBI_OP_TEXTURE_RECTANGLE << 24) | (right << 12) | bottom,
                      (left << 12) | top);
#endif /* ENABLE_PATCHES */
}

void RenderBillboardParticles(
    const ViewportRect *viewports, u32 head, s32 red, s32 green, s32 blue) {
    const ViewportRect *view;
    const Particle *particle;
    f32 clip[4];
    f32 span, scaleX, inverse, size, edge;
    s32 centerX, centerY, height, halfWidth, halfHeight, x, y;
    BILLBOARD_INDEX index;
    long double rawInverse, rawDepth, rawSize, rawEdge, rawY, heightPixels;

    view = &viewports[g_dwCurrentSplitScreenIndex];
    centerX = BILLBOARD_ADD(BILLBOARD_ADD(view->x, view->x), view->width);
    centerX = BILLBOARD_ADD(centerX, centerX);
    centerY = BILLBOARD_ADD(BILLBOARD_ADD(view->y, view->y), view->height);
    centerY = BILLBOARD_ADD(centerY, centerY);
    span = (f32)BILLBOARD_ADD(view->width, view->width);
    scaleX = span;
    height = BILLBOARD_ADD(view->height, view->height);

    for (index = head; index != 0; index = particle->next) {
        particle = &BILLBOARD_PARTICLES[index];
        Vec3fTransformByMatrix(clip, &particle->position, g_afBillboardMatrix);
        if (clip[3] <= 0.001f && clip[3] >= -0.001f) {
            continue;
        }
        rawInverse = 1.0L / clip[3];
        inverse = rawInverse;
        rawDepth = rawInverse * clip[2];
        clip[2] = rawDepth;
        if (rawDepth >= 1.0f || !(clip[2] > -0.3f)) {
            continue;
        }
        clip[0] = (long double)inverse * clip[0];
        rawSize = (long double)particle->size * inverse * 4.0f;
        size = rawSize;
#ifdef ENABLE_PATCHES
        /* Preserve the 4:3 puff size against viewport height, rather than ultrawide width. */
        rawEdge = rawSize * view->height / view->width + 1.0L;
#else
        rawEdge = rawSize - -1.0f;
#endif
        edge = rawEdge;
        volatile long double negativeEdge = -rawEdge;
        f32 lowerEdge = negativeEdge;
        if (!(clip[0] > lowerEdge) || clip[0] >= edge) {
            continue;
        }
        rawY = (long double)inverse * clip[1];
        clip[1] = rawY;
#ifdef ENABLE_PATCHES
        rawEdge = rawSize * (4.0L / 3.0L) + 1.0L;
        edge = rawEdge;
        negativeEdge = -rawEdge;
        lowerEdge = negativeEdge;
#endif
        if (!(rawY > lowerEdge) || clip[1] >= edge) {
            continue;
        }
#ifdef ENABLE_PATCHES
        heightPixels = (long double)view->height * (8.0L / 3.0L) * size;
#else
        heightPixels = (long double)span * size;
#endif
        halfWidth = (s32)(s64)(heightPixels * 0.75f);
        if (halfWidth <= 0) {
            continue;
        }
        halfHeight = (s32)(s64)heightPixels;
        if (halfHeight <= 0) {
            continue;
        }
        x = BILLBOARD_ADD((s32)(s64)((long double)clip[0] * scaleX), centerX);
        y = BILLBOARD_ADD((s32)(s64)((long double)clip[1] * (f32)height), centerY);

        gDPPipeSync(gDisplayListPtr++);
        gDPSetPrimColor(gDisplayListPtr++,
                        0xFF,
                        0xFF,
                        red,
                        green,
                        blue,
                        (particle->alpha * particle->fade) >> 8);
        gSPClearGeometryMode(gDisplayListPtr++, G_CULL_BOTH);
        gDisplayListPtr->w0 = (u32)GBI_OP_SET_TEX_RECT_W << 24;
        gDisplayListPtr++->f1 = inverse;
        gDisplayListPtr->w0 = (u32)GBI_OP_SET_TEX_RECT_DEPTH << 24;
        gDisplayListPtr++->f1 = clip[2];
        WriteBillboardRectangle(x, y, halfWidth, halfHeight);
    }
}
#endif /* TGR_BOSS_RALLY */
