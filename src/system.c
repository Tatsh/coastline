#include "system.h"

#include <PR/R4300.h>
#include <PR/gu.h>
#include <PR/os.h>
#include <PR/os_cont.h>
#include <PR/os_libc.h>

#include "audio.h"
#include "camera.h"
#include "debugtext.h"
#include "fade.h"
#include "game.h"
#include "input.h"
#include "libultra/os/osint.h"
#include "platform/n64/boot.h"
#include "race.h"
#include "render.h"
#include "rumble.h"
#include "texrect.h"
#include "text.h"
#include "timing.h"
#include "track.h"
#include "trackrender.h"
#include "trackselect.h"
#include "util.h"
#include "vecmath.h"
#include "vehicle.h"
#include "weather.h"
#include "zlib.h"

/* The messages of the DMA that may wait at once. */
#define IO_MESSAGES 32

/* A DMA copies at most 4 KB at once. */
#define DMA_BLOCK_SIZE 0x1000

/* Each half of a buffer of compressed data, and the room the decompression of one gets. */
#define CHUNK_BUFFER_SIZE 16000
#define CHUNK_OUTPUT_SIZE 16000

/* The heap lent to zlib by a decompression that runs over several calls. */
#define INFLATE_HEAP_SIZE 100000

/* The words of the header of a compressed asset: its compressed and decompressed sizes. */
#define ASSET_HEADER_WORD 4

/* The retraces the counter of the vertical blank counts before it wraps. */
#define VBLANK_COUNTER_MASK 15

/* The fixed step of a race in nanoseconds, a frame of 1/30 of a second, or 1/25 at PAL rates. */
#if TGR_REGION == TGR_REGION_EU
#define RACE_STEP_NANOSECONDS 40000000
#else
#define RACE_STEP_NANOSECONDS 33333333
#endif

/* The priorities of the threads. */
#define FAULT_THREAD_ID 5
#define FAULT_THREAD_PRIORITY 50
#define MAIN_THREAD_ID 3
#define MAIN_THREAD_PRIORITY 10
#define IDLE_THREAD_ID 1
#define IDLE_THREAD_PRIORITY 10

/* The message the fault event sends. */
#define FAULT_MESSAGE 0x10

/* The block of arguments a development kit leaves in the cartridge space, 64 bytes. */
#define BOOT_ARGUMENTS_ADDRESS 0xFFB000
#define BOOT_ARGUMENT_WORDS 16

/* The two words the stacks are filled with, to show how deep they were used. */
#define STACK_FILL_HI 0x5015A1DB
#define STACK_FILL_LO 0xFED15C00

/* The screen the boot sets up. */
#define BOOT_SCREEN_WIDTH 320
#define BOOT_SCREEN_HEIGHT 240

/* The full-height video modes: the game's copies in the USA release, the SDK's in the others. */
#if TGR_REGION == TGR_REGION_US
#define VI_MODE_LAN1_MPAL g_ViModeMpalLan1
#define VI_MODE_LAN1 g_ViModeNtscLan1
#elif TGR_REGION == TGR_REGION_EU
#define VI_MODE_LAN1_MPAL osViModeMpalLan1
#define VI_MODE_LAN1 osViModePalLan1
#else
#define VI_MODE_LAN1_MPAL osViModeMpalLan1
#define VI_MODE_LAN1 osViModeNtscLan1
#endif

/* Whether the European and Japanese releases may show the screen: it is not held blank, no delay
 * before showing it runs, and no reset is pending. */
#define SCREEN_MAY_SHOW()                                                                          \
    (!g_dwKeepScreenBlanked && !g_dwScreenUnblankDelay && !g_dwPreNmiResetPending)

/* The European release adjusts its copies of the PAL modes: the vertical start of each field
 * (the bits outside the mask kept) and the vertical scale of the half-height and full-height
 * modes. */
#define PAL_VSTART_KEEP 0xFC00FC00
#define PAL_VSTART_FIELD0 0x0039025B
#define PAL_VSTART_FIELD1 0x003B025D
#define PAL_HAF1_YSCALE 0x6F0
#define PAL_LAN1_YSCALE 0x378

/* The memory the European and Japanese releases clear at boot, below the code of the game. */
#define BOOT_CLEAR_START 0x800D4000
#define BOOT_CLEAR_END 0x80200000

#define ARRAY_COUNT(array) (s32)(sizeof(array) / sizeof((array)[0]))

/* The address the RSP and the RDP read memory of KSEG0 at. */
#ifdef __GNUC__
#define TO_RSP_ADDRESS(p) ((u32)(p) - K0BASE)
#else
/* IDO folds the base into the relocation of a datum only through pointer arithmetic. */
#define TO_RSP_ADDRESS(p) ((u32)((u8 *)(p) - K0BASE))
#endif

/* The first command of the display list of the current frame buffer. */
#define DISPLAY_LIST_START()                                                                       \
    (g_pDisplayListBase + g_dwFrameBufferIndex * DISPLAY_LIST_COMMANDS + DISPLAY_LIST_RESERVED)

/* A fill of the frame buffer with a colour of 8-bit components, as two pixels. */
#define FILL_COLOR(red, green, blue)                                                               \
    ((u16)GPACK_RGBA5551(red, green, blue, 1) | (u16)GPACK_RGBA5551(red, green, blue, 1) << 16)

/* A fill of the depth buffer with the farthest depth, as two depths. */
#define FARTHEST_DEPTHS ((u32)GPACK_ZDZ(G_MAXFIXED, 0) << 16 | GPACK_ZDZ(G_MAXFIXED, 0))

/* A colour of a light, as the microcode reads it. */

/* A blend of the colour of the frame buffer and the colour drawn, by the alpha of the fog. */
#define RENDER_MODE_FOG_BLEND_C1                                                                   \
    (IM_RD | CVG_DST_FULL | ZMODE_OPA | FORCE_BL |                                                 \
     GBL_c1(G_BL_CLR_MEM, G_BL_A_FOG, G_BL_CLR_IN, G_BL_1MA))
#define RENDER_MODE_FOG_BLEND_C2 GBL_c2(G_BL_CLR_MEM, G_BL_A_FOG, G_BL_CLR_IN, G_BL_1MA)

/* The gap between the views and the edges of the screen. */
#define VIEWPORT_MARGIN 8

/* The size of the low resolution, the viewport of SetupViewportAndScissor(). */
#define LOW_RES_WIDTH 320
#define LOW_RES_HEIGHT 240

/* Half of the range of depths of a viewport. */
#define VIEWPORT_HALF_DEPTH 0x1FF

/* A frame of an animated texture with no texels or no palette on the cartridge. */
#define TEXTURE_ROM_NONE ((u32) - 1)

/* The bytes of the palettes of a 4-bit texture and of an 8-bit one. */
#define CI4_PALETTE_BYTES (16 * sizeof(u16))
#define PALETTE_BYTES (256 * sizeof(u16))

u64 g_qwTimestampBase = 0;

OSIoMesg *AllocIoMessage(void) {
    if (g_dwIoMessagePendingCount == IO_MESSAGES) {
        /* With every message in flight, one finished DMA frees its message. */
        osRecvMesg(&g_DmaMessageQueue, NULL, OS_MESG_BLOCK);
    } else {
        g_dwIoMessagePendingCount++;
    }
    g_dwIoMessagePoolIndex = (g_dwIoMessagePoolIndex + 1) % IO_MESSAGES;
    return &g_aIoMessagePool[g_dwIoMessagePoolIndex];
}

void WaitForDmaMessages(void) {
    if (g_dwIoMessagePendingCount != 0) {
        do {
            osRecvMesg(&g_DmaMessageQueue, NULL, OS_MESG_BLOCK);
        } while (--g_dwIoMessagePendingCount != 0);
    }
}

s32 UnusedVblankQueryStub(s32 unused0, s32 unused1) {
    return 0;
}

void ClearRenderStats(void) {
    g_dwStatUnused1 = g_dwStatUnused2 = g_dwStatUnused3 = 0;
    g_dwStatPolygonsDrawn = g_dwStatVehicleTriangles = 0;
    g_dwStatTrianglesDrawn = g_dwStatVerticesDrawn = g_dwStatUnused0 = 0;
}

/* The original sets the palette only for a 4-bit texture, and the others read it unset. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
void DrawTexturedTile(s32 index, ModelTexture *textures) {
    ModelTexture *texture;
    s32 width, height, scaleS, scaleT, maskS, maskT, cms, cmt, tmem, palette, format, size, tile;

    texture = &textures[index];
    width = texture->width;
    GetTextureScaleAndMask(width, &scaleS, &maskS);
    height = texture->height;
    GetTextureScaleAndMask(height, &scaleT, &maskT);
    cms = texture->clampS ? G_TX_CLAMP : G_TX_WRAP;
    if (texture->mirrorS) {
        cms |= G_TX_MIRROR;
    }
    cmt = texture->clampT ? G_TX_CLAMP : G_TX_WRAP;
    if (texture->mirrorT) {
        cmt |= G_TX_MIRROR;
    }
    gDPTileSync(gDisplayListPtr++);
    gDPSetTextureImage(gDisplayListPtr++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, texture->image);
    tmem = g_dwTextureTmemAddress & 0x1FF;
    gDPSetTile(gDisplayListPtr++,
               G_IM_FMT_RGBA,
               G_IM_SIZ_16b,
               0,
               tmem,
               G_TX_LOADTILE,
               0,
               G_TX_WRAP,
               maskT,
               G_TX_NOLOD,
               G_TX_WRAP,
               maskS,
               G_TX_NOLOD);
    gDPLoadSync(gDisplayListPtr++);
    switch (texture->format) {
    case TEXTURE_FORMAT_CI4:
        gDPLoadBlock(gDisplayListPtr++, G_TX_LOADTILE, 0, 0, ((width * height + 3) >> 2) - 1, 0);
        palette = TEXTURE_CI4_PALETTE;
        gDPLoadSync(gDisplayListPtr++);
        gDPLoadTLUT_pal16(gDisplayListPtr++, TEXTURE_CI4_PALETTE, texture->palette);
        gDPSetTextureLUT(gDisplayListPtr++, G_TT_RGBA16);
        format = G_IM_FMT_CI;
        size = G_IM_SIZ_4b;
        break;
    case TEXTURE_FORMAT_I8:
        cms = cmt = G_TX_CLAMP;
        /* The sine is discarded. */
        sins(0);
        gDPLoadBlock(gDisplayListPtr++, G_TX_LOADTILE, 0, 0, ((width * height + 1) >> 1) - 1, 0);
        gDPSetTextureLUT(gDisplayListPtr++, G_TT_NONE);
        tmem = g_dwTextureTmemAddress & 0x1FF;
        format = G_IM_FMT_I;
        size = G_IM_SIZ_8b;
        break;
    default:
        gDPLoadBlock(gDisplayListPtr++, G_TX_LOADTILE, 0, 0, width * height - 1, 0);
        gDPSetTextureLUT(gDisplayListPtr++, G_TT_NONE);
        format = G_IM_FMT_RGBA;
        size = G_IM_SIZ_16b;
        break;
    }
    tile = g_dwTextureRenderTile;
    gDPSetTile(gDisplayListPtr++,
               format,
               size,
               (width * (4 << size) + 63) >> 6,
               tmem,
               tile,
               palette,
               cmt,
               maskT,
               G_TX_NOLOD,
               cms,
               maskS,
               G_TX_NOLOD);
    gDPSetTileSize(gDisplayListPtr++,
                   tile,
                   texture->tileS0 * 4 + 2,
                   texture->tileT0 * 4 + 2,
                   texture->tileS1 * 4 + 2,
                   texture->tileT1 * 4 + 2);
    gSPTexture(gDisplayListPtr++, scaleS, scaleT, 0, tile, G_ON);
}
#pragma GCC diagnostic pop

void StubEmpty(void) {
}

void ClearDepthBuffer(void) {
    gDPPipeSync(gDisplayListPtr++);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_FILL);
    gDPSetRenderMode(gDisplayListPtr++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
    gDPSetColorImage(gDisplayListPtr++,
                     G_IM_FMT_RGBA,
                     G_IM_SIZ_16b,
                     g_dwScreenWidth << g_dwScreenScaleShift,
                     TO_RSP_ADDRESS(g_aDepthBuffer));
    gDPSetFillColor(gDisplayListPtr++, FARTHEST_DEPTHS);
    gDPFillRectangle(gDisplayListPtr++,
                     0,
                     0,
                     (g_dwScreenWidth << g_dwScreenScaleShift) - 1,
                     (g_dwScreenHeight << g_dwScreenScaleShift) - 1);
    gDPPipeSync(gDisplayListPtr++);
    gDPSetColorImage(gDisplayListPtr++,
                     G_IM_FMT_RGBA,
                     G_IM_SIZ_16b,
                     g_dwScreenWidth << g_dwScreenScaleShift,
                     TO_RSP_ADDRESS(g_apFramebuffers[g_dwFrameBufferIndex]));
}

void ClearZBufferRect(s32 x, s32 y, s32 width, s32 height) {
    gDPPipeSync(gDisplayListPtr++);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_FILL);
    gDPSetRenderMode(gDisplayListPtr++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
    gDPSetColorImage(gDisplayListPtr++,
                     G_IM_FMT_RGBA,
                     G_IM_SIZ_16b,
                     g_dwScreenWidth << g_dwScreenScaleShift,
                     TO_RSP_ADDRESS(g_aDepthBuffer));
    gDPSetFillColor(gDisplayListPtr++, FARTHEST_DEPTHS);
    /* The bottom edge is taken from x, not y. */
    gDPFillRectangle(gDisplayListPtr++, x, y, x + width - 1, x + height - 1);
    gDPPipeSync(gDisplayListPtr++);
    gDPSetColorImage(gDisplayListPtr++,
                     G_IM_FMT_RGBA,
                     G_IM_SIZ_16b,
                     g_dwScreenWidth << g_dwScreenScaleShift,
                     TO_RSP_ADDRESS(g_apFramebuffers[g_dwFrameBufferIndex]));
}

void ClearScreen(s32 red, s32 green, s32 blue) {
    gDPPipeSync(gDisplayListPtr++);
    gDPSetRenderMode(gDisplayListPtr++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_FILL);
    gDPSetFillColor(gDisplayListPtr++, FILL_COLOR(red, green, blue));
    gDPFillRectangle(gDisplayListPtr++,
                     0,
                     0,
                     (g_dwScreenWidth << g_dwScreenScaleShift) - 1,
                     (g_dwScreenHeight << g_dwScreenScaleShift) - 1);
    gDPPipeSync(gDisplayListPtr++);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_1CYCLE);
}

void DrawFilledRect(s32 x, s32 y, s32 width, s32 height, s32 red, s32 green, s32 blue) {
    if (g_dwScreenScaleShift) {
        x *= 2;
        y *= 2;
        width *= 2;
        height *= 2;
    }
    gDPPipeSync(gDisplayListPtr++);
    gDPSetRenderMode(gDisplayListPtr++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_FILL);
    gDPSetFillColor(gDisplayListPtr++, FILL_COLOR(red, green, blue));
    /* The lower right corner is doubled a second time in the high resolution. */
    gDPFillRectangle(gDisplayListPtr++,
                     x,
                     y,
                     ((x + width) << g_dwScreenScaleShift) - 1,
                     ((y + height) << g_dwScreenScaleShift) - 1);
    gDPPipeSync(gDisplayListPtr++);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_1CYCLE);
}

void BeginDisplayList(s32 highRes) {
    if (highRes != g_dwScreenScaleShift) {
        g_dwDisplayModeChangeCountdown = 1;
        g_dwScreenScaleShift = highRes;
    }
    AddTimingMarker(0, 0, 0x82, 0, 0xFF);
#if TGR_REGION == TGR_REGION_JP
    ResetGlyphCache();
#endif
    switch (g_dwNumViewports) {
    case 1:
        g_aViewportRects[0].x = VIEWPORT_MARGIN;
        g_aViewportRects[0].y = VIEWPORT_MARGIN;
        g_aViewportRects[0].width = g_dwScreenWidth - 2 * VIEWPORT_MARGIN;
        g_aViewportRects[0].height = g_dwScreenHeight - 2 * VIEWPORT_MARGIN;
        break;
    case 2:
        /* The two views stop 88 pixels short of the right of the screen. */
        g_aViewportRects[1].x = VIEWPORT_MARGIN;
        g_aViewportRects[1].y = (g_dwScreenHeight >> 1) + 1;
        g_aViewportRects[1].width = g_dwScreenWidth - 96;
        g_aViewportRects[1].height = (g_dwScreenHeight >> 1) - VIEWPORT_MARGIN;
        g_aViewportRects[0].x = VIEWPORT_MARGIN;
        g_aViewportRects[0].y = VIEWPORT_MARGIN;
        g_aViewportRects[0].width = g_dwScreenWidth - 96;
        g_aViewportRects[0].height = (g_dwScreenHeight >> 1) - VIEWPORT_MARGIN;
        break;
    }
    ClearRenderStats();
    ResetFrameBlocks();
    if (g_dwDisplayModeChangeCountdown != 0 && g_dwDisplayModeChangeCountdown - 1 == 0) {
        if (g_dwScreenScaleShift) {
            /* The frame buffers of the high resolution end where the first of the low starts. */
            g_apFramebuffers[0] = g_aFramebuffer0 - (g_aFramebuffer1 - g_aFramebuffer0) * 6;
            g_apFramebuffers[1] = g_aFramebuffer0 - (g_aFramebuffer1 - g_aFramebuffer0) * 2;
        } else {
            g_apFramebuffers[0] = g_aFramebuffer0;
            g_apFramebuffers[1] = g_aFramebuffer1;
        }
    }
    gDisplayListPtr = DISPLAY_LIST_START();
    g_dwTextureFilterMode = g_bBilinearFiltering ? G_TF_BILERP : G_TF_POINT;
    g_dwColorDitherMode = G_CD_BAYER;
    g_dwAlphaDitherMode = G_AD_PATTERN;
    gSPSegment(gDisplayListPtr++, 0, 0);
    gDPPipeSync(gDisplayListPtr++);
    SetScissorRect(0, 0, g_dwScreenWidth, g_dwScreenHeight);
    gDPSetCombineLERP(gDisplayListPtr++,
                      0,
                      0,
                      0,
                      PRIMITIVE,
                      0,
                      0,
                      0,
                      PRIMITIVE,
                      0,
                      0,
                      0,
                      COMBINED,
                      0,
                      0,
                      0,
                      COMBINED);
    gDPSetTextureLOD(gDisplayListPtr++, G_TL_TILE);
    gDPSetTextureLUT(gDisplayListPtr++, G_TT_NONE);
    gDPSetTextureDetail(gDisplayListPtr++, G_TD_CLAMP);
    gDPSetTexturePersp(gDisplayListPtr++, G_TP_PERSP);
    gDPSetTextureFilter(gDisplayListPtr++, g_dwTextureFilterMode);
    gDPSetTextureConvert(gDisplayListPtr++, G_TC_FILT);
    gDPSetCombineKey(gDisplayListPtr++, G_CK_NONE);
    gDPSetAlphaCompare(gDisplayListPtr++, G_AC_THRESHOLD);
    gDPSetRenderMode(gDisplayListPtr++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
    gDPSetColorDither(gDisplayListPtr++, g_dwColorDitherMode);
    /* The alpha dither goes to the field of the colour dither. */
    gDPSetColorDither(gDisplayListPtr++, g_dwAlphaDitherMode);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_1CYCLE);
    gDPSetBlendColor(gDisplayListPtr++, 0, 0, 0, 0);
    gSPMatrix(gDisplayListPtr++,
              TO_RSP_ADDRESS(&g_mtxIdentity),
              G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPClearGeometryMode(gDisplayListPtr++,
                         G_SHADE | G_SHADING_SMOOTH | G_CULL_BOTH | G_FOG | G_LIGHTING |
                             G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR | G_LOD);
    gSPSetGeometryMode(gDisplayListPtr++, G_CULL_BACK);
    if (g_bGbiClipping) {
        gSPSetGeometryMode(gDisplayListPtr++, G_CLIPPING);
    } else {
        gSPClearGeometryMode(gDisplayListPtr++, G_CLIPPING);
    }
    gSPDisplayList(gDisplayListPtr++, g_aClipRatioDisplayLists[g_dwClipRatioIndex]);
    gSPTexture(gDisplayListPtr++, 0, 0, 0, G_TX_RENDERTILE, G_OFF);
    gDPSetColorImage(gDisplayListPtr++,
                     G_IM_FMT_RGBA,
                     G_IM_SIZ_16b,
                     g_dwScreenWidth << g_dwScreenScaleShift,
                     TO_RSP_ADDRESS(g_apFramebuffers[g_dwFrameBufferIndex]));
    gDPSetDepthImage(gDisplayListPtr++, TO_RSP_ADDRESS(g_aDepthBuffer));
    osViSetSpecialFeatures(OS_VI_DITHER_FILTER_ON);
    osViSetSpecialFeatures(OS_VI_DIVOT_ON);
    osViSetSpecialFeatures(g_dwViGammaEnabled ? OS_VI_GAMMA_ON : OS_VI_GAMMA_OFF);
}

void StartLowResFrame(void) {
    BeginDisplayList(0);
}

void StartHighResFrame(void) {
    BeginDisplayList(1);
}

void StubEmptyVoid(s32 unused0, ...) {
}

void SetScissorRect(s32 x, s32 y, s32 width, s32 height) {
    if (x < g_dwFadeRectLeft) {
        width = width - g_dwFadeRectLeft + x;
        x = g_dwFadeRectLeft;
    }
    if (g_dwFadeRectRight < x + width) {
        width = g_dwFadeRectRight - x;
    }
    if (width < 0) {
        width = 0;
    }
    if (y < g_dwFadeRectTop) {
        height = height - g_dwFadeRectTop + y;
        y = g_dwFadeRectTop;
    }
    if (g_dwFadeRectBottom < y + height) {
        height = g_dwFadeRectBottom - y;
    }
    if (height < 0) {
        height = 0;
    }
    if (g_dwScreenScaleShift) {
        x *= 2;
        y *= 2;
        width *= 2;
        height *= 2;
    }
    gDPPipeSync(gDisplayListPtr++);
    gDPSetScissor(gDisplayListPtr++, G_SC_NON_INTERLACE, x, y, x + width, y + height);
}

void SetViewport(s32 x, s32 y, s32 width, s32 height, s32 setScissor) {
    g_dwViewportIndex = (g_dwViewportIndex + 1) & (VIEWPORTS - 1);
    if (setScissor) {
        SetScissorRect(x, y, width < 0 ? -width : width, height);
    }
    if (g_dwScreenScaleShift) {
        x *= 2;
        y *= 2;
        width *= 2;
        height *= 2;
    }
    /* A negative width mirrors the view, and a mirrored track mirrors it again. */
    if (width < 0) {
        if (g_dwTrackMirrored) {
            g_aViewports[g_dwViewportIndex].vp.vscale[0] = -width * 2;
        } else {
            g_aViewports[g_dwViewportIndex].vp.vscale[0] = width * 2;
        }
        width = -width;
        g_dwViewportMirrored = TRUE;
    } else {
        if (g_dwTrackMirrored) {
            g_aViewports[g_dwViewportIndex].vp.vscale[0] = -width * 2;
        } else {
            g_aViewports[g_dwViewportIndex].vp.vscale[0] = width * 2;
        }
        g_dwViewportMirrored = FALSE;
    }
    g_aViewports[g_dwViewportIndex].vp.vscale[1] = height * 2;
    g_aViewports[g_dwViewportIndex].vp.vscale[2] = VIEWPORT_HALF_DEPTH;
    g_aViewports[g_dwViewportIndex].vp.vscale[3] = 0;
    g_aViewports[g_dwViewportIndex].vp.vtrans[0] = (x * 2 + width) * 2;
    g_aViewports[g_dwViewportIndex].vp.vtrans[1] = (y * 2 + height) * 2;
    g_aViewports[g_dwViewportIndex].vp.vtrans[2] = VIEWPORT_HALF_DEPTH;
    g_aViewports[g_dwViewportIndex].vp.vtrans[3] = 0;
    gSPViewport(gDisplayListPtr++, &g_aViewports[g_dwViewportIndex]);
    g_dwViewportScaleZ = g_aViewports[g_dwViewportIndex].vp.vscale[2];
    g_dwViewportTransZ = g_aViewports[g_dwViewportIndex].vp.vtrans[2];
}

void SetupViewportAndScissor(s32 x, s32 y, s32 width, s32 height, s32 setScissor) {
    g_dwViewportIndex = (g_dwViewportIndex + 1) & (VIEWPORTS - 1);
    if (setScissor) {
        gDPPipeSync(gDisplayListPtr++);
        SetScissorRect(x, y, width < 0 ? -width : width, height);
    }
    g_dwViewportMirrored = FALSE;
    g_aViewports[g_dwViewportIndex].vp.vscale[0] = LOW_RES_WIDTH * 2;
    g_aViewports[g_dwViewportIndex].vp.vscale[1] = LOW_RES_HEIGHT * 2;
    g_aViewports[g_dwViewportIndex].vp.vscale[2] = VIEWPORT_HALF_DEPTH;
    g_aViewports[g_dwViewportIndex].vp.vscale[3] = 0;
    g_aViewports[g_dwViewportIndex].vp.vtrans[0] = LOW_RES_WIDTH * 2;
    g_aViewports[g_dwViewportIndex].vp.vtrans[1] = LOW_RES_HEIGHT * 2;
    g_aViewports[g_dwViewportIndex].vp.vtrans[2] = VIEWPORT_HALF_DEPTH;
    g_aViewports[g_dwViewportIndex].vp.vtrans[3] = 0;
    gSPViewport(gDisplayListPtr++, &g_aViewports[g_dwViewportIndex]);
    g_dwViewportScaleZ = g_aViewports[g_dwViewportIndex].vp.vscale[2];
    g_dwViewportTransZ = g_aViewports[g_dwViewportIndex].vp.vtrans[2];
}

void RestoreCurrentViewport(void) {
    gSPViewport(gDisplayListPtr++, &g_aViewports[g_dwViewportIndex]);
    g_dwViewportScaleZ = g_aViewports[g_dwViewportIndex].vp.vscale[2];
    g_dwViewportTransZ = g_aViewports[g_dwViewportIndex].vp.vtrans[2];
}

void DrawRdpOutlineRect(s32 x, s32 y, s32 width, s32 height) {
    gDPPipeSync(gDisplayListPtr++);
    /* With the alpha of the fog at 255, the blend retains the colour of the frame buffer. */
    gDPSetRenderMode(gDisplayListPtr++, RENDER_MODE_FOG_BLEND_C1, RENDER_MODE_FOG_BLEND_C2);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_1CYCLE);
    gDPSetFogColor(gDisplayListPtr++, 0, 0, 0, 0xFF);
    gDPFillRectangle(gDisplayListPtr++, x, y, x + width, y + 1);
    gDPFillRectangle(gDisplayListPtr++, x, y + height - 1, x + width, y + height);
    gDPFillRectangle(gDisplayListPtr++, x, y, x + 1, y + height);
    gDPFillRectangle(gDisplayListPtr++, x + width - 1, y, x + width, y + height);
}

void DrawRdpBorderRect(s32 x, s32 y, s32 width, s32 height) {
    gDPPipeSync(gDisplayListPtr++);
    gDPSetRenderMode(gDisplayListPtr++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_FILL);
    gDPSetFillColor(gDisplayListPtr++, FILL_COLOR(0, 0, 0));
    gDPSetFogColor(gDisplayListPtr++, 0, 0, 0, 0xFF);
    gDPFillRectangle(gDisplayListPtr++, x - 1, y - 2, x + width, y - 1);
    gDPFillRectangle(gDisplayListPtr++, x - 1, y + height, x + width, y + height + 1);
    gDPFillRectangle(gDisplayListPtr++, x - 2, y - 1, x - 1, y + height);
    gDPFillRectangle(gDisplayListPtr++, x + width, y - 1, x + width + 1, y + height);
    gDPPipeSync(gDisplayListPtr++);
    gDPSetRenderMode(gDisplayListPtr++, RENDER_MODE_FOG_BLEND_C1, RENDER_MODE_FOG_BLEND_C2);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_1CYCLE);
    gDPSetFogColor(gDisplayListPtr++, 0, 0, 0, 0xFF);
    gDPFillRectangle(gDisplayListPtr++, x, y, x + width, y + 1);
    gDPFillRectangle(gDisplayListPtr++, x, y + height - 1, x + width, y + height);
    gDPFillRectangle(gDisplayListPtr++, x, y, x + 1, y + height);
    gDPFillRectangle(gDisplayListPtr++, x + width - 1, y, x + width, y + height);
    gDPFillRectangle(gDisplayListPtr++, x - 3, y - 3, x + width + 3, y - 2);
    gDPFillRectangle(gDisplayListPtr++, x - 3, y + height + 2, x + width + 3, y + height + 3);
    gDPFillRectangle(gDisplayListPtr++, x - 3, y - 3, x - 2, y + height + 3);
    gDPFillRectangle(gDisplayListPtr++, x + width + 2, y - 3, x + width + 3, y + height + 3);
}

#if TGR_REGION == TGR_REGION_JP || TGR_REGION == TGR_REGION_EU
void BlankDisplay(void) {
#if TGR_REGION == TGR_REGION_EU
    if (!g_dwDisplayBlanked) {
        if (g_dwAppliedScreenScaleShift) {
            osViSetMode(&osViModePalHaf1);
        } else {
            osViSetMode(&osViModePalLan1);
        }
    }
#endif
    osViBlack(TRUE);
    g_dwDisplayBlanked = TRUE;
}

void UnblankDisplay(void) {
    osViBlack(FALSE);
#if TGR_REGION == TGR_REGION_EU
    if (g_dwDisplayBlanked) {
        if (g_dwAppliedScreenScaleShift) {
            osViSetMode(&g_ViModePalHaf1);
        } else {
            osViSetMode(&g_ViModePalLan1);
        }
    }
#endif
    g_dwDisplayBlanked = FALSE;
}
#endif

void ClearFramebuffersHighRes(void) {
#if TGR_REGION == TGR_REGION_US
    g_dwKeepScreenBlanked = TRUE;
    osViBlack(TRUE);
    StartHighResFrame();
    ClearScreen(0, 0, 0);
    FlushDisplayList();
    osViBlack(TRUE);
    StartHighResFrame();
    ClearScreen(0, 0, 0);
    FlushDisplayList();
    g_dwKeepScreenBlanked = FALSE;
#else
    u32 *word;
    u32 *end;

    g_dwKeepScreenBlanked = TRUE;
    StartLowResFrame();
    ClearScreen(0, 0, 0);
    FlushDisplayList();
    /* The high resolution frame buffers lie from the end of the interface heap to the program. */
    word = (u32 *)0x800D4000;
    end = (u32 *)0x80200000;
    for (; word < end; word++) {
        *word = 0;
    }
    StartHighResFrame();
    ClearScreen(0, 0, 0);
    FlushDisplayList();
    g_dwKeepScreenBlanked = FALSE;
#endif
}

void ClearFramebuffersLowRes(void) {
#if TGR_REGION == TGR_REGION_US
    g_dwKeepScreenBlanked = TRUE;
    osViBlack(TRUE);
    StartLowResFrame();
    ClearScreen(0, 0, 0);
    FlushDisplayList();
    osViBlack(TRUE);
    StartLowResFrame();
    ClearScreen(0, 0, 0);
    FlushDisplayList();
    g_dwKeepScreenBlanked = FALSE;
#else
    u32 *word;
    u32 *end;

    g_dwKeepScreenBlanked = TRUE;
    StartHighResFrame();
    ClearScreen(0, 0, 0);
    FlushDisplayList();
    StartHighResFrame();
    ClearScreen(0, 0, 0);
    FlushDisplayList();
    /* The two low resolution frame buffers end where the program starts. */
    word = (u32 *)0x801B5000;
    end = (u32 *)0x80200000;
    for (; word < end; word++) {
        *word = 0;
    }
    StartLowResFrame();
    ClearScreen(0, 0, 0);
    FlushDisplayList();
    g_dwKeepScreenBlanked = FALSE;
#endif
}

void StreamAnimatedTrackTextures(void) {
    ModelTexture *texture;
    TextureAnimation *animation;
    OSIoMesg *message;
    s32 snowTint, i, frame, frameCount;
    u32 phase, imageRom, paletteRom;

    snowTint = g_dwSnowMode != 0 && g_dwSelectedTrackId != TRACK_COASTLINE &&
               g_dwSelectedTrackId != TRACK_MIRROR_COASTLINE;
    for (i = 0; i < g_LoadedTrack.textureCount; i++) {
        texture = &g_LoadedTrack.textures[i];
        if (texture->image == NULL) {
            continue;
        }
        if (!texture->animated) {
            if (g_dwSnowMode == (u32)g_dwStreamedSnowMode) {
                continue;
            }
            paletteRom = (texture->source.paletteRomIndex & 0xFFF) << 5;
        } else {
            animation = texture->source.animation;
            frameCount = animation->frameCount;
            /* The phase is taken from the time of the first frame onward, as if it were 0. */
            phase = (u32)g_dwElapsedTimeMs %
                    (animation->frames[frameCount - 1].time - animation->frames[0].time);
            if (frameCount == 2 && animation->frames[0].time == TEXTURE_ROM_NONE) {
                /* A texture of the snow, then the texture of the other weathers. */
                if ((u32)g_dwStreamedSnowMode == g_dwSnowMode) {
                    continue;
                }
                frame = g_dwSnowMode == 0;
                imageRom = frame ? TEXTURE_ROM_NONE : animation->frames[frame].imageRom;
                paletteRom = animation->frames[frame].paletteRom;
            } else {
                if (g_dwRacePauseState) {
                    continue;
                }
                if (snowTint && texture->format == TEXTURE_FORMAT_SNOW_STILL) {
                    frame = 0;
                } else {
                    for (frame = 1; frame < frameCount; frame++) {
                        if (phase < animation->frames[frame].time) {
                            break;
                        }
                    }
                    frame--;
                }
                imageRom = animation->frames[frame].imageRom;
                paletteRom = animation->frames[frame].paletteRom;
            }
            if (texture->size != 0 && imageRom != TEXTURE_ROM_NONE) {
                message = AllocIoMessage();
                texture = &g_LoadedTrack.textures[i];
                osPiStartDma(message,
                             OS_MESG_PRI_NORMAL,
                             OS_READ,
                             imageRom + g_dwTrackTextureRomOffset,
                             texture->image,
                             texture->size,
                             &g_DmaMessageQueue);
                texture = &g_LoadedTrack.textures[i];
            }
        }
        if (texture->palette != NULL && paletteRom != TEXTURE_ROM_NONE) {
            message = AllocIoMessage();
            texture = &g_LoadedTrack.textures[i];
            osPiStartDma(message,
                         OS_MESG_PRI_NORMAL,
                         OS_READ,
                         paletteRom + g_dwTrackTextureRomOffset,
                         texture->palette,
                         texture->format == TEXTURE_FORMAT_CI4 ? CI4_PALETTE_BYTES : PALETTE_BYTES,
                         &g_DmaMessageQueue);
        }
    }
    g_dwStreamedSnowMode = g_dwSnowMode;
}

void FlushDisplayList(void) {
    OSTask *task;
    u16 *framebuffer;
    u32 *pairsOut, *pairsIn;
    s32 commands, count, pairs, i;

    gDPFullSync(gDisplayListPtr++);
    gSPEndDisplayList(gDisplayListPtr++);
    task = &g_aGfxTask[g_dwFrameBufferIndex];
    task->t.type = M_GFXTASK;
    task->t.flags = OS_TASK_DP_WAIT | OS_TASK_LOADABLE;
    task->t.ucode = g_aGspUcodeText;
    task->t.ucode_data = g_abRspUcodeData;
    task->t.output_buff = g_pRdpOutputBuffer;
    task->t.ucode_size = SP_UCODE_SIZE;
    task->t.output_buff_size = g_pRdpOutputBufferEnd - g_dwOutputBuffReserve;
    task->t.ucode_data_size = SP_UCODE_DATA_SIZE;
    task->t.dram_stack = (u64 *)(((u32)g_abRspDramStackBuffer + 15) & ~15);
    task->t.dram_stack_size = SP_DRAM_STACK_SIZE8;
    task->t.ucode_boot = g_aRspBootUcode;
    task->t.ucode_boot_size = (g_aGspUcodeText - g_aRspBootUcode) * sizeof(u64);
    /* The original works out the start of the display list again for each use. */
    task->t.data_ptr = (u64 *)DISPLAY_LIST_START();
    task->t.data_size = (gDisplayListPtr - DISPLAY_LIST_START()) * sizeof(Gfx);
    commands = gDisplayListPtr - DISPLAY_LIST_START();
    if (g_dwPeakDisplayListCommands < commands) {
        g_dwPeakDisplayListCommands = commands;
    }
    count = g_pShadowDisplayListWritePtr - g_pShadowDisplayListStart;
    if (g_dwPeakShadowCommandCount < count) {
        g_dwPeakShadowCommandCount = count;
    }
    count = g_pShadowVertexWritePtr - g_pShadowVertexStart;
    if (g_dwPeakShadowVertexCount < count) {
        g_dwPeakShadowVertexCount = count;
    }
    g_dwDisplayListCommandCount = commands;
    if (commands > DISPLAY_LIST_COMMANDS) {
        SetDebugMessage("HUGE GLIST ERROR");
    }
    osWritebackDCacheAll();
    if (g_dwFirstFlushDone) {
        AddTimingMarker(0, 0, 0, 0, 0xFF);
        osRecvMesg(&g_RspDoneMsgQueue, NULL, OS_MESG_BLOCK);
        AddTimingMarker(0, 0xFF, 0xFF, 0, 0xFF);
        if (g_pfnNextScreen != NULL) {
            g_pfnNextScreen();
            g_pfnNextScreen = NULL;
        }
        AddTimingMarker(0, 0, 0, 0, 0xFF);
        osRecvMesg(&g_RdpDoneMsgQueue, NULL, OS_MESG_BLOCK);
        if (g_bShowDepthBuffer) {
            /* The depth buffer is copied over the frame about to show, two depths at a time. */
            pairs = ((g_dwScreenScaleShift ? 4 : 1) * g_dwScreenWidth * g_dwScreenHeight) >> 1;
            pairsOut = (u32 *)g_apFramebuffers[g_dwFrameBufferIndex ^ 1];
            pairsIn = (u32 *)g_aDepthBuffer;
            for (i = 0; i < pairs; i++) {
                pairsOut[i] = pairsIn[i];
            }
        }
        /* The test is of the second routine, but the call is of the next. The next is NULL here
         * unless a routine has set it since it was cleared. */
        if (g_pfnSecondaryScreen != NULL) {
            g_pfnNextScreen();
            g_pfnNextScreen = NULL;
        }
#if TGR_REGION == TGR_REGION_JP
        LoadCachedGlyphTextures();
#endif
        osWritebackDCacheAll();
        g_dwFlushDuration = osGetCount();
        g_dwFlushInterval = g_dwFlushDuration - g_dwFlushStartTime;
        if (g_dwDisplayModeChangeCountdown != 0 && --g_dwDisplayModeChangeCountdown == 0) {
#if TGR_REGION == TGR_REGION_EU
            /* The European release changes the mode only while the screen may show. */
            if (g_dwScreenScaleShift) {
                if (SCREEN_MAY_SHOW()) {
                    osViSetMode(&g_ViModePalHaf1);
                }
            } else if (SCREEN_MAY_SHOW()) {
                osViSetMode(&g_ViModePalLan1);
            }
#else
            if (g_dwScreenScaleShift) {
                osViSetMode(osTvType == OS_TV_MPAL ? &g_ViModeMpalHaf1 : &g_ViModeNtscHaf1);
            } else {
                osViSetMode(osTvType == OS_TV_MPAL ? &VI_MODE_LAN1_MPAL : &VI_MODE_LAN1);
            }
#endif
#if TGR_REGION == TGR_REGION_US
            osViBlack(TRUE);
#endif
            g_dwAppliedScreenScaleShift = g_dwScreenScaleShift;
        }
        AddTimingMarker(1, 0x20, 0x20, 0x20, 0xFF);
        AddTimingMarker(2, 0x20, 0x20, 0x20, 0xFF);
        AddTimingMarker(0, 0x20, 0x20, 0x20, 0xFF);
        framebuffer = g_apFramebuffers[g_dwFrameBufferIndex ^ 1];
        osRecvMesg(&g_ViRetraceMesgQueue, NULL, OS_MESG_BLOCK);
        osViSwapBuffer(framebuffer);
        osRecvMesg(&g_ViRetraceMesgQueue, NULL, OS_MESG_BLOCK);
        if (osViGetCurrentFramebuffer() != framebuffer) {
            osRecvMesg(&g_ViRetraceMesgQueue, NULL, OS_MESG_BLOCK);
        }
#if TGR_REGION == TGR_REGION_US
        if (!g_dwKeepScreenBlanked && !g_dwScreenUnblankDelay) {
            osViBlack(FALSE);
        }
#else
        if (SCREEN_MAY_SHOW()) {
            UnblankDisplay();
        }
#endif
        if (g_dwScreenUnblankDelay) {
            g_dwScreenUnblankDelay--;
        }
        BeginTimingFrame();
        g_dwFlushStartTime = osGetCount();
        g_dwFlushDuration = osGetCount() - g_dwFlushDuration;
    } else {
        g_dwFirstFlushDone++;
    }
    AddTimingMarker(0, 0xC8, 0, 0xC8, 0xFF);
    g_dwLastFlushValue = g_dwFlushValueSource;
    AddTimingMarker(2, 0xC8, 0, 0, 0xFF);
    AddTimingMarker(1, 0xC8, 0x64, 0, 0xFF);
    osSpTaskLoad(task);
    osSpTaskStartGo(task);
    g_dwFrameBufferIndex ^= 1;
}

void RspDoneThread(void *arg) {
    osCreateMesgQueue(&g_RspEventMsgQueue, g_aRspEventMsgBuf, 1);
    osSetEventMesg(OS_EVENT_SP, &g_RspEventMsgQueue, (OSMesg)g_dwDummyOsMesg);
    for (;;) {
        osRecvMesg(&g_RspEventMsgQueue, NULL, OS_MESG_BLOCK);
        AddTimingMarker(1, 0, 0, 0, 0xFF);
        osSendMesg(&g_RspDoneMsgQueue, (OSMesg)g_dwDummyOsMesg, OS_MESG_BLOCK);
    }
}

void RdpDoneThread(void *arg) {
    osCreateMesgQueue(&g_RdpEventMsgQueue, g_aRdpEventMsgBuf, 1);
    osSetEventMesg(OS_EVENT_DP, &g_RdpEventMsgQueue, (OSMesg)g_dwDummyOsMesg);
    for (;;) {
        osRecvMesg(&g_RdpEventMsgQueue, NULL, OS_MESG_BLOCK);
        AddTimingMarker(2, 0, 0, 0, 0xFF);
        osSendMesg(&g_RdpDoneMsgQueue, (OSMesg)g_dwDummyOsMesg, OS_MESG_BLOCK);
    }
}

#if TGR_REGION == TGR_REGION_JP || TGR_REGION == TGR_REGION_EU
void PreNmiResetHandlerThread(void *arg) {
    osCreateMesgQueue(&g_PreNmiMesgQueue, g_aPreNmiMesgBuf, 1);
    osSetEventMesg(OS_EVENT_PRENMI, &g_PreNmiMesgQueue, (OSMesg)g_dwDummyOsMesg);
    for (;;) {
        osRecvMesg(&g_PreNmiMesgQueue, NULL, OS_MESG_BLOCK);
        g_bSfxMasterVolume = 0;
        g_bBgmVolume = 0;
        g_dwPreNmiResetPending = TRUE;
        BlankDisplay();
    }
}
#endif

void ViRetraceThread(void *arg) {
    osCreateMesgQueue(&g_ViRetracePollMesgQueue, g_ViRetracePollMesgBuf, 1);
    osViSetEvent(&g_ViRetracePollMesgQueue, (OSMesg)g_dwDummyOsMesg, 1);
    for (;;) {
        osRecvMesg(&g_ViRetracePollMesgQueue, NULL, OS_MESG_BLOCK);
        osSendMesg(&g_ViRetraceMesgQueue, (OSMesg)g_dwDummyOsMesg, OS_MESG_BLOCK);
        g_dwVblankCounter = (g_dwVblankCounter + 1) & VBLANK_COUNTER_MASK;
    }
}

void ResetFrameClock(void) {
    g_qwTimestampBase = 0;
    g_dwFrameTimestamp = 0;
    g_dwPrevElapsedTimeMs = g_dwElapsedTimeMs = 0;
    g_dwFrameDeltaMs = 0;
    g_fFrameDeltaTime = 0.0f;
}

void UpdateFrameTiming(void) {
    u32 now;
    u32 delta;

    if (g_bRaceScreenActive) {
        /* A race steps a fixed time a frame. */
        g_qwTimestampBase += (u64)RACE_STEP_NANOSECONDS * osClockRate / 1000000000;
        g_dwFrameTimestamp = osGetCount();
    } else if (g_qwTimestampBase != 0) {
        now = osGetCount();
        g_qwTimestampBase += (u32)(now - g_dwFrameTimestamp);
        g_dwFrameTimestamp = now;
    } else {
        g_qwTimestampBase = g_dwFrameTimestamp = osGetCount();
    }
    g_dwPrevElapsedTimeMs = g_dwElapsedTimeMs;
    g_dwElapsedTimeMs = g_qwTimestampBase * 1000000 / osClockRate / 1000;
    UpdateControllerState();
    DebugFrameStubA();
    DebugFrameStubB();
    delta = g_dwElapsedTimeMs - g_dwPrevElapsedTimeMs;
    g_fFrameDeltaTime = (f32)delta / 1000.0f;
    g_dwFrameDeltaMs = delta;
}

void UnusedFrameTimingStub(void) {
}

void DmaRead(void *dst, u32 romOffset, s32 size) {
    osInvalDCache(dst, size);
    while (size > DMA_BLOCK_SIZE) {
        osPiStartDma(AllocIoMessage(),
                     OS_MESG_PRI_NORMAL,
                     OS_READ,
                     romOffset,
                     dst,
                     DMA_BLOCK_SIZE,
                     &g_DmaMessageQueue);
        size -= DMA_BLOCK_SIZE;
        romOffset += DMA_BLOCK_SIZE;
        dst = (u8 *)dst + DMA_BLOCK_SIZE;
    }
    osPiStartDma(
        AllocIoMessage(), OS_MESG_PRI_NORMAL, OS_READ, romOffset, dst, size, &g_DmaMessageQueue);
    WaitForDmaMessages();
}

u32 DmaReadAssetSize(u32 romOffset) {
    DmaRead(&g_dwDmaAssetSizeBuffer, romOffset + ASSET_HEADER_WORD, sizeof(g_dwDmaAssetSizeBuffer));
    return g_dwDmaAssetSizeBuffer;
}

u32 DmaReadWord(u32 romOffset) {
    DmaRead(&g_dwDmaReadWordBuffer, romOffset, sizeof(g_dwDmaReadWordBuffer));
    return g_dwDmaReadWordBuffer;
}

void InitDmaRequest(DecompressJob *job, u8 *buffer) {
    job->dst = NULL;
    job->buffer = buffer;
}

/* The size of a chunk on the cartridge: its length word, then its bytes, padded to an even count.
 */
#define CHUNK_SPAN(size) (((size) + 5) & ~1)

u32 DecompressFromRom(void *dst, u32 romOffset, DecompressJob *job) {
    u8 *out = dst;
    u8 *buffer;
    u32 compressedSize;
    u32 decompressedSize;
    u32 remaining;
    u32 half;
    u32 chunkSize;
    uLong outputSize;
    u32 start;
    s32 first;

    if (job != NULL && job->dst != NULL) {
        romOffset = job->romOffset;
        out = job->dst;
        compressedSize = job->compressedSize;
        remaining = job->remaining;
        decompressedSize = job->decompressedSize;
        buffer = job->buffer;
        half = job->bufferHalf;
        first = FALSE;
        g_dwDmaChunkSize = job->pendingChunkSize;
    } else {
        DmaRead(&g_dwDmaChunkSize, romOffset, sizeof(g_dwDmaChunkSize));
        compressedSize = g_dwDmaChunkSize;
        remaining = g_dwDmaChunkSize;
        romOffset += ASSET_HEADER_WORD;
        DmaRead(&g_dwDmaChunkSize, romOffset, sizeof(g_dwDmaChunkSize));
        decompressedSize = g_dwDmaChunkSize;
        romOffset += ASSET_HEADER_WORD;
        remaining -= 2 * ASSET_HEADER_WORD;
        buffer = job != NULL ? job->buffer : g_aDmaWorkBuffers;
        half = 0;
        first = TRUE;
        g_dwDmaChunkSize = 0;
    }
    start = osGetCount();

    /* While one half of the buffer is read, the chunk in the other is decompressed. */
    for (;;) {
        chunkSize = g_dwDmaChunkSize;
        if (CHUNK_SPAN(g_dwDmaChunkSize) < remaining) {
            if (romOffset & 1) {
                romOffset++;
            }
            DmaRead(&g_dwDmaChunkSize, romOffset, sizeof(g_dwDmaChunkSize));
            romOffset += ASSET_HEADER_WORD;
            osInvalDCache(&buffer[half], CHUNK_BUFFER_SIZE);
            osPiStartDma(AllocIoMessage(),
                         OS_MESG_PRI_NORMAL,
                         OS_READ,
                         romOffset,
                         &buffer[half],
                         (g_dwDmaChunkSize + 1) & ~1,
                         &g_DmaMessageQueue);
            romOffset += (g_dwDmaChunkSize + 1) & ~1;
        } else {
            WaitForDmaMessages();
        }
        osInvalDCache(&buffer[half], CHUNK_BUFFER_SIZE);
        half ^= CHUNK_BUFFER_SIZE;
        if (first) {
            first = FALSE;
        } else {
            outputSize = CHUNK_OUTPUT_SIZE;
            if (job != NULL) {
                g_pInflateHeapBase = g_aInflateHeapBuffer;
                g_pInflateHeapEnd = &g_aInflateHeapBuffer[INFLATE_HEAP_SIZE];
            }
            uncompress(out, &outputSize, &buffer[half], chunkSize);
            if (job != NULL) {
                g_pInflateHeapEnd = NULL;
                g_pInflateHeapBase = NULL;
            }
            remaining -= CHUNK_SPAN(chunkSize);
            out += outputSize;
            if (remaining == 0) {
                break;
            }
        }
        /* A decompression over several calls takes one step a call. */
        if (job != NULL) {
            break;
        }
    }
    /* The time it took, in microseconds, is worked out and not kept. */
    start = osGetCount() - start;
    (void)((u64)start * 1000000 / osClockRate);

    if (job != NULL) {
        job->romOffset = romOffset;
        job->remaining = remaining;
        job->dst = out;
        job->compressedSize = compressedSize;
        job->bufferHalf = half;
        job->decompressedSize = decompressedSize;
        job->buffer = buffer;
        job->pendingChunkSize = g_dwDmaChunkSize;
    }
    return decompressedSize;
}

void WaitForVblankMessage(void) {
    osRecvMesg(&g_ViRetraceMesgQueue, NULL, OS_MESG_BLOCK);
}

void StubFunction(s32 unused) {
}

void SetDebugMessage(const char *message) {
    g_pszDebugMessage = message;
    StubFunction(0);
}

void FaultThread(void *arg) {
    OSMesg message;

    osSetEventMesg(OS_EVENT_FAULT, &g_FaultMsgQueue, (OSMesg)FAULT_MESSAGE);
    g_dwFaultState = 0;
    for (;;) {
        osRecvMesg(&g_FaultMsgQueue, &message, OS_MESG_BLOCK);
        osSyncPrintf("\n=> faultproc - got a fault message...\n");
        g_pFaultedThread = __osGetCurrFaultedThread();
        if (g_pFaultedThread != NULL) {
            StubFunction((s32)g_pFaultedThread);
        }
    }
}

void BootIdleThread(void *arg) {
    u32 arguments[BOOT_ARGUMENT_WORDS];
    s32 i;
    char *p;
    char c;
#if TGR_REGION == TGR_REGION_EU || TGR_REGION == TGR_REGION_JP
    u32 *clear;
#endif

    for (i = 0; i < BOOT_ARGUMENT_WORDS; i++) {
        osPiRawReadIo(BOOT_ARGUMENTS_ADDRESS + i * sizeof(u32), &arguments[i]);
    }

    /* The words are read as text, split at the spaces into g_apszBootArgv. */
    p = (char *)arguments;
    if (*p != '\0') {
        c = *p;
        do {
            while (c != '\0' && c == ' ') {
                c = p[1];
                *p++ = '\0';
            }
            if (c != '\0') {
                /* There is no limit on the count. The original reads the count again to raise
                 * it. */
                g_apszBootArgv[g_dwBootArgc] = p;
                g_dwBootArgc++;
                c = *p;
            }
            while (c != '\0' && c != ' ') {
                c = p[1];
                p++;
            }
        } while (c != '\0');
    }
    g_ppszMainArgv = g_ppszBootArgv;
    g_pdwMainArgc = &g_dwBootArgc;

    osCreateViManager(OS_PRIORITY_VIMGR);
    g_dwScreenWidth = BOOT_SCREEN_WIDTH;
    g_dwScreenHeight = BOOT_SCREEN_HEIGHT;
#if TGR_REGION == TGR_REGION_EU
    memcpy(&g_ViModePalHaf1, &osViModePalHaf1, sizeof(OSViMode));
    memcpy(&g_ViModePalLan1, &osViModePalLan1, sizeof(OSViMode));
    g_ViModePalHaf1.fldRegs[0].vStart =
        (osViModePalHaf1.fldRegs[0].vStart & PAL_VSTART_KEEP) | PAL_VSTART_FIELD0;
    g_ViModePalHaf1.fldRegs[1].vStart =
        (osViModePalHaf1.fldRegs[1].vStart & PAL_VSTART_KEEP) | PAL_VSTART_FIELD1;
    g_ViModePalHaf1.fldRegs[0].yScale = PAL_HAF1_YSCALE;
    g_ViModePalHaf1.fldRegs[1].yScale = PAL_HAF1_YSCALE;
    g_ViModePalLan1.fldRegs[0].vStart =
        (osViModePalLan1.fldRegs[0].vStart & PAL_VSTART_KEEP) | PAL_VSTART_FIELD0;
    g_ViModePalLan1.fldRegs[1].vStart =
        (osViModePalLan1.fldRegs[1].vStart & PAL_VSTART_KEEP) | PAL_VSTART_FIELD1;
    g_ViModePalLan1.fldRegs[0].yScale = PAL_LAN1_YSCALE;
    g_ViModePalLan1.fldRegs[1].yScale = PAL_LAN1_YSCALE;
#endif
    if (osTvType == OS_TV_MPAL) {
        osViSetMode(&VI_MODE_LAN1_MPAL);
    } else {
        osViSetMode(&VI_MODE_LAN1);
    }
    osViBlack(TRUE);
#if TGR_REGION == TGR_REGION_EU || TGR_REGION == TGR_REGION_JP
    g_dwDisplayBlanked = TRUE;
    for (clear = (u32 *)BOOT_CLEAR_START; clear < (u32 *)BOOT_CLEAR_END; clear++) {
        *clear = 0;
    }
#endif
    osCreatePiManager(OS_PRIORITY_PIMGR, &g_PiMgrCmdQueue, g_aPiMgrCmdMsgBuf, IO_MESSAGES);
    osCreateMesgQueue(&g_FaultMsgQueue, g_aFaultMsgBuf, 1);
    osCreateThread(&g_FaultThreadObj,
                   FAULT_THREAD_ID,
                   FaultThread,
                   g_ppszBootArgv,
                   &g_adwFaultThreadStack[FAULT_STACK_WORDS],
                   FAULT_THREAD_PRIORITY);
    osStartThread(&g_FaultThreadObj);

    /* Fill the stacks with a pattern that shows how deep they were used. */
    for (i = 0; i < MAIN_STACK_WORDS / 2; i++) {
        g_adwMainThreadStack[i * 2] = STACK_FILL_HI;
        g_adwMainThreadStack[i * 2 + 1] = STACK_FILL_LO;
    }
    for (i = 0; i < THREAD_STACK_WORDS / 2; i++) {
        g_adwRspDoneThreadStack[i * 2] = STACK_FILL_HI;
        g_adwRspDoneThreadStack[i * 2 + 1] = STACK_FILL_LO;
    }
    for (i = 0; i < THREAD_STACK_WORDS / 2; i++) {
        g_adwRdpDoneThreadStack[i * 2] = STACK_FILL_HI;
        g_adwRdpDoneThreadStack[i * 2 + 1] = STACK_FILL_LO;
    }
    osCreateThread(&g_MainGameThreadObj,
                   MAIN_THREAD_ID,
                   (void (*)(void *))InitGameAndShowMenus,
                   arg,
                   &g_adwMainThreadStack[MAIN_STACK_WORDS],
                   MAIN_THREAD_PRIORITY);
    osStartThread(&g_MainGameThreadObj);
    osSetThreadPri(NULL, OS_PRIORITY_IDLE);
    for (;;) {
    }
}

void OsBootEntry(void) {
#if TGR_REGION == TGR_REGION_EU
    /* The PAL release forces the television standard before osInitialize() reads it. */
    osTvType = OS_TV_PAL;
    osViClock = VI_PAL_CLOCK;
#endif
    osInitialize();
    osCreateThread(&g_BootIdleThreadObj,
                   IDLE_THREAD_ID,
                   BootIdleThread,
                   NULL,
                   &g_adwIdleThreadStack[IDLE_STACK_WORDS],
                   IDLE_THREAD_PRIORITY);
    osStartThread(&g_BootIdleThreadObj);
}
