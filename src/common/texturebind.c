#include <PR/gu.h>

#include "common/render.h"

#if TGR_PC
#include "win32/gbicommand.h"
#else
#include "common/texrect.h"
#endif

#if TGR_PC

void DrawTexturedTile(s32 index, ModelTexture *textures) {
    if (!MODEL_TEXTURE_ANIMATED(&textures[index])) {
        GbiCommand *command = gDisplayListPtr++;
        command->w0 = 0xdc000000 | (textures[index].textureEntry & 0xffffff);
        command->w1 = 1;
    }
}
#else

/* The original sets the palette only for a 4-bit texture, and the others read it unset. */
void DrawTexturedTile(s32 index, ModelTexture *textures) {
    ModelTexture *texture;
#ifdef ENABLE_PATCHES
    /* A texture of more than 4 bits takes palette 0. */
    s32 width, height, scaleS, scaleT, maskS, maskT, cms, cmt, tmem, palette = 0, format, size,
                                                                     tile;
#else
    s32 width, height, scaleS, scaleT, maskS, maskT, cms, cmt, tmem, palette, format, size, tile;
#endif /* ENABLE_PATCHES */

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
#endif /* TGR_PC */
