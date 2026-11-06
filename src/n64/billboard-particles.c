#include <PR/gbi.h>

#include "common/effects.h"
#include "common/render.h"
#include "common/system.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

void RenderBillboardParticles(s32 head, s32 red, s32 green, s32 blue) {
    float clip[4];
    Particle *unusedParticle TGR_MATCHING_UNUSED;
    int index;
    float inverse;
    float size;
    float edge;
    int width;
    int height;
    int centerX;
    int centerY;
    float scaleX;
    float span;
    int halfWidth;
    int halfHeight;
    int screenX;
    int screenY;

    width = g_aViewportRects[g_dwCurrentSplitScreenIndex].width;
    height = g_aViewportRects[g_dwCurrentSplitScreenIndex].height;
    centerX = (g_aViewportRects[g_dwCurrentSplitScreenIndex].x * 2 + width) * 2;
    centerY = (g_aViewportRects[g_dwCurrentSplitScreenIndex].y * 2 + height) * 2;
    span = (float)(width * 2);
    scaleX = span;
    if (g_dwViewportMirrored ^ g_dwTrackMirrored) {
        scaleX = -span;
    }
    for (index = head; index != 0; index = g_aParticlePool[index].next) {
        Vec3fTransformByMatrix(clip, &g_aParticlePool[index].position, g_afBillboardMatrix);
        if (clip[3] <= 0.001f && clip[3] >= -0.001f) {
            continue;
        }
        inverse = 1.0f / clip[3];
        clip[2] *= inverse;
        if (clip[2] >= 1.0f || clip[2] <= -0.3f) {
            continue;
        }
        clip[0] *= inverse;
        size = g_aParticlePool[index].size * (4.0f * inverse);
        edge = 1.0f + size;
        if (clip[0] <= -edge || clip[0] >= edge) {
            continue;
        }
        clip[1] *= inverse;
        if (clip[1] <= -edge || clip[1] >= edge) {
            continue;
        }
        halfWidth = span * 0.75f * size;
        if (halfWidth <= 0) {
            continue;
        }
        halfHeight = span * size;
        if (halfHeight <= 0) {
            continue;
        }
        screenX = (int)(clip[0] * scaleX) + centerX;
        screenY = (int)(clip[1] * (float)(height * 2)) + centerY;
        gDPPipeSync(gDisplayListPtr++);
        gDPSetPrimColor(gDisplayListPtr++,
                        0xff,
                        0xff,
                        red,
                        green,
                        blue,
                        (g_aParticlePool[index].fade * g_aParticlePool[index].alpha) >> 8);
        gDPSetPrimDepth(gDisplayListPtr++, (int)(clip[2] * 16352.0f) + 0x3fe0, 0);
        {
            Gfx *command = (Gfx *)(gDisplayListPtr++);

            command->words.w0 =
                (_SHIFTL(G_TEXRECT, 24, 8) | _SHIFTL(MAX((s16)(screenX + halfWidth), 0), 12, 12) |
                 _SHIFTL(MAX((s16)(screenY + halfHeight), 0), 0, 12));
            command->words.w1 =
                (_SHIFTL(0, 24, 3) | _SHIFTL(MAX((s16)(screenX - halfWidth), 0), 12, 12) |
                 _SHIFTL(MAX((s16)(screenY - halfHeight), 0), 0, 12));
            gImmp1(
                gDisplayListPtr++,
                G_RDPHALF_1,
                (_SHIFTL(
                     (0 -
                      (((s16)(screenX - halfWidth) < 0) ?
                           (((s16)(0x1f800 / halfWidth) < 0) ?
                                (MAX((((s16)(screenX - halfWidth) * (s16)(0x1f800 / halfWidth)) >>
                                      7),
                                     0)) :
                                (MIN((((s16)(screenX - halfWidth) * (s16)(0x1f800 / halfWidth)) >>
                                      7),
                                     0))) :
                           0)),
                     16,
                     16) |
                 _SHIFTL((0x7e0 - (((screenY - halfHeight) < 0) ?
                                       (((s16)(-0x1f800 / halfHeight) < 0) ?
                                            (MAX((((s16)(screenY - halfHeight) *
                                                   (s16)(-0x1f800 / halfHeight)) >>
                                                  7),
                                                 0)) :
                                            (MIN((((s16)(screenY - halfHeight) *
                                                   (s16)(-0x1f800 / halfHeight)) >>
                                                  7),
                                                 0))) :
                                       0)),
                         0,
                         16)));
            gImmp1(
                gDisplayListPtr++,
                G_RDPHALF_2,
                (_SHIFTL((0x1f800 / halfWidth), 16, 16) | _SHIFTL((-0x1f800 / halfHeight), 0, 16)));
        }
    }
}
