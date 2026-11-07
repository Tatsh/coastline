#include <PR/gu.h>

#include "common/debugmessage.h"
#include "common/render.h"
#include "common/texrect.h"

#if TGR_PLATFORM == TGR_PLATFORM_N64
#include <PR/os_libc.h>
#else
#include <stdio.h>
#endif

#if TGR_BOSS_RALLY
#include "common/gbicommand.h"
#endif

void GetTextureScaleAndMask(u32 size, s32 *scale, s32 *maskBits) {
#ifdef ENABLE_PATCHES
    /* The debug display retains the message, which needs 42 bytes for the largest size. The console
     * BSS may not grow, so the buffer is placed in the data. */
#if defined(__GNUC__) && TGR_PLATFORM == TGR_PLATFORM_N64
    static char message[48] __attribute__((section(".data")));
#else
    static char message[48];
#endif
#else
    /* The original stores the address of a local message for the debug display. */
    char message[40];
#endif /* ENABLE_PATCHES */

    /* The size becomes the largest coordinate. */
    size--;
    if (size >> 8) {
        if (size >> 10) {
            /* In the original the message may overrun the buffer, and the debug display retains a
             * pointer to the stack this returns from. */
            sprintf(message, "ERROR: unhandled texture size: %d", size);
            SetDebugMessage(message);
        } else if (size >> 9) {
            *maskBits = 10;
        } else {
            *maskBits = 9;
        }
    } else if (size & 0xF0) {
        if (size & 0xC0) {
            if (size & 0x80) {
                *maskBits = 8;
            } else {
                *maskBits = 7;
            }
        } else if (size & 0xE0) {
            *maskBits = 6;
        } else {
            *maskBits = 5;
        }
    } else if (size & 0xFC) {
        if (size & 0xF8) {
            *maskBits = 4;
        } else {
            *maskBits = 3;
        }
    } else if (size & 0xFE) {
        *maskBits = 2;
    } else if (size != 0) {
        *maskBits = 1;
    } else {
        *maskBits = 0;
    }
    *scale = 0xFFFF;
}

#if TGR_BOSS_RALLY

void DrawTexturedTile(s32 index, ModelTexture *textures) {
    if (!MODEL_TEXTURE_ANIMATED(&textures[index])) {
        GbiCommand *command = gDisplayListPtr++;
        command->w0 = 0xdc000000 | (textures[index].textureEntry & 0xffffff);
        command->w1 = 1;
    }
}
#else

void DrawTexturedTile(s32 index, ModelTexture *textures) {
    static s32 tile = 0;
    static s32 base = 0;
    ModelTexture *texture;
    s32 maskS;
    s32 maskT;
    s32 bitsS;
    s32 bitsT;
    s32 cms;
    s32 cmt;
    s32 width;
    s32 height;
    s32 format;
    s32 size;
    s32 tmem;
#ifdef ENABLE_PATCHES
    s32 palette = 0;
#else
    s32 palette;
#endif

    /* The unused local preserves IDO's stack layout. */
    (void)tmem;

    texture = &textures[index];
    width = texture->width;
    GetTextureScaleAndMask(width, &maskS, &bitsS);
    height = texture->height;
    GetTextureScaleAndMask(height, &maskT, &bitsT);
    cms = texture->clampS ? 2 : 0;
    if (texture->mirrorS) {
        cms |= 1;
    }
    cmt = texture->clampT ? 2 : 0;
    if (texture->mirrorT) {
        cmt |= 1;
    }
    gDPTileSync(gDisplayListPtr++);
    gDPSetTextureImage(gDisplayListPtr++, 0, 2, 1, texture->image);
    gDPSetTile(gDisplayListPtr++, 0, 2, 0, base, 7, 0, 0, bitsT, 0, 0, bitsS, 0);
    gDPLoadSync(gDisplayListPtr++);
    if (texture->format == 1) {
        gDPLoadBlock(gDisplayListPtr++, 7, 0, 0, ((width * height + 3) >> 2) - 1, 0);
        palette = 15;
        gDPLoadSync(gDisplayListPtr++);
        gDPSetTextureImage(gDisplayListPtr++, 0, 2, 1, texture->palette);
        gDPTileSync(gDisplayListPtr++);
        gDPSetTile(gDisplayListPtr++, 0, 0, 0, 0x1f0, 7, 0, 0, 0, 0, 0, 0, 0);
        gDPLoadSync(gDisplayListPtr++);
        gDPLoadTLUTCmd(gDisplayListPtr++, 7, 15);
        gDPPipeSync(gDisplayListPtr++);
        gSPSetOtherMode(gDisplayListPtr++, G_SETOTHERMODE_H, 14, 2, 0x8000);
        format = 2;
        size = 0;
    } else if (texture->format == 4) {
        cmt = cms = 2;
        sins(0);
        gDPLoadBlock(gDisplayListPtr++, 7, 0, 0, ((width * height + 1) >> 1) - 1, 0);
        gSPSetOtherMode(gDisplayListPtr++, G_SETOTHERMODE_H, 14, 2, 0);
        format = 4;
        size = 1;
    } else {
        gDPLoadBlock(gDisplayListPtr++, 7, 0, 0, width * height - 1, 0);
        gSPSetOtherMode(gDisplayListPtr++, G_SETOTHERMODE_H, 14, 2, 0);
        format = 0;
        size = 2;
    }
    gDPSetTile(gDisplayListPtr++,
               format,
               size,
               (width * (4 << size) + 63) >> 6,
               base,
               tile,
               palette,
               cmt,
               bitsT,
               0,
               cms,
               bitsS,
               0);
    gDPSetTileSize(gDisplayListPtr++,
                   tile,
                   texture->tileS0 * 4 + 2,
                   texture->tileT0 * 4 + 2,
                   texture->tileS1 * 4 + 2,
                   texture->tileT1 * 4 + 2);
    gSPTexture(gDisplayListPtr++, maskS, maskT, 0, tile, 1);
}
#endif /* TGR_BOSS_RALLY */
