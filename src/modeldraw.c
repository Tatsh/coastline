#include <PR/gu.h>

#include "model.h"
#include "render.h"
#include "system.h"
#include "util.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include "win32/combine.h"
#endif

#if TGR_PLATFORM == TGR_PLATFORM_WIN
/** @ghidraAddress BRD3D: 0x10690a1c */
s32 g_dwDrawModelListNoZBuffer;
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

void DrawModelList(Model *model, f32 m[4][4]) {
    ModelPart *part;
    LookAt *lookAt;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    f32(*block)[4];
#else
    Mtx *block;
#endif
    u32 i;
    s32 pass;

    gDPPipeSync(gDisplayListPtr++);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_2CYCLE);
    if (g_dwDrawModelListNoZBuffer) {
        gDPSetRenderMode(gDisplayListPtr++,
                         GBL_c1(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1),
                         AA_EN | CVG_DST_CLAMP | ZMODE_OPA | ALPHA_CVG_SEL |
                             GBL_c2(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM));
        g_dwDrawModelListNoZBuffer = FALSE;
    } else {
        gDPSetRenderMode(gDisplayListPtr++,
                         GBL_c1(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1),
                         AA_EN | Z_CMP | Z_UPD | CVG_DST_CLAMP | ZMODE_OPA | ALPHA_CVG_SEL |
                             GBL_c2(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM));
    }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    PackCombineMode(gDisplayListPtr++,
                    1000 + G_CCMUX_TEXEL1,
                    1000 + G_CCMUX_TEXEL0,
                    1000 + G_CCMUX_LOD_FRACTION,
                    1000 + G_CCMUX_TEXEL0,
                    1000 + G_ACMUX_TEXEL1,
                    1000 + G_ACMUX_TEXEL0,
                    1000 + G_CCMUX_LOD_FRACTION,
                    1000 + G_ACMUX_TEXEL0,
                    1000 + G_CCMUX_COMBINED,
                    0,
                    1000 + G_CCMUX_SHADE,
                    0,
                    0,
                    0,
                    0,
                    1000 + G_ACMUX_COMBINED);
#else
    gDPSetCombineLERP(gDisplayListPtr++,
                      TEXEL1,
                      TEXEL0,
                      LOD_FRACTION,
                      TEXEL0,
                      TEXEL1,
                      TEXEL0,
                      LOD_FRACTION,
                      TEXEL0,
                      COMBINED,
                      0,
                      SHADE,
                      0,
                      0,
                      0,
                      0,
                      COMBINED);
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
    gDPSetBlendColor(gDisplayListPtr++, 0, 0, 0, 0);
    gDPSetTextureDetail(gDisplayListPtr++, G_TD_CLAMP);
    gDPSetTextureLOD(gDisplayListPtr++, G_TL_TILE);
    gDPSetTextureLUT(gDisplayListPtr++, G_TT_NONE);
    gDPSetTextureFilter(gDisplayListPtr++, g_dwTextureFilterMode);
    gDPSetColorDither(gDisplayListPtr++, g_dwColorDitherMode);
    gDPSetAlphaDither(gDisplayListPtr++, g_dwAlphaDitherMode);
    gSPSetGeometryMode(gDisplayListPtr++, G_ZBUFFER);
    gDPSetAlphaCompare(gDisplayListPtr++, G_AC_THRESHOLD);
    /* The texture modes are set again, with the level of detail on. */
    gDPSetTextureDetail(gDisplayListPtr++, G_TD_CLAMP);
    gDPSetTextureLOD(gDisplayListPtr++, G_TL_LOD);
    gDPSetTextureLUT(gDisplayListPtr++, G_TT_NONE);
    gDPSetTextureFilter(gDisplayListPtr++, g_dwTextureFilterMode);
    gSPClearGeometryMode(gDisplayListPtr++,
                         G_CLIPPING | G_TEXTURE_GEN | G_FOG | G_CULL_BACK | G_CULL_FRONT |
                             G_SHADING_SMOOTH);
    gSPSetGeometryMode(
        gDisplayListPtr++,
        G_ZBUFFER | G_SHADE | G_LIGHTING | G_TEXTURE_GEN_LINEAR |
            (g_dwViewportMirrored != g_dwTrackMirrored ? G_CULL_FRONT : G_CULL_BACK) |
            G_SHADING_SMOOTH);
    lookAt = AllocLightBlock();
    guLookAtReflectF(
        g_afScratchMatrix, lookAt, 0.0f, -1.0f, 15.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    gSPLookAt(gDisplayListPtr++, lookAt);
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    block = AllocDisplayListBlock()->matrix;
#else
    block = (Mtx *)AllocDisplayListBlock();
#endif
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    CopyMatrix4x4(m, block);
#else
    guMtxF2L(m, block);
#endif
    gSPMatrix(gDisplayListPtr++, block, G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
    gSPTexture(gDisplayListPtr++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    gSPClearGeometryMode(gDisplayListPtr++, G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR);
    gDPTileSync(gDisplayListPtr++);
    gDPSetTile(
        gDisplayListPtr++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 0, G_TX_LOADTILE, 0, 0, 0, 0, 0, 0, 0);
    gDPSetTile(gDisplayListPtr++, G_IM_FMT_RGBA, G_IM_SIZ_4b, 0, 0x1F0, 6, 0, 0, 0, 0, 0, 0, 0);
    gDPSetTile(gDisplayListPtr++, G_IM_FMT_RGBA, G_IM_SIZ_4b, 0, 0x100, 5, 0, 0, 0, 0, 0, 0, 0);

    /* The opaque parts, then the translucent ones. */
    for (pass = 0; pass < 2; pass++) {
        for (i = 0, part = model->parts; i < model->partCount; i++, part++) {
            if (((part->flags & MODEL_PART_TRANSLUCENT) == 0) != (pass == 0)) {
                continue;
            }
            if (part->displayList == NULL) {
                continue;
            }
            if (part->flags & MODEL_PART_RAMPED) {
                gSPLightColor(gDisplayListPtr++, LIGHT_1, 0);
                gSPLightColor(gDisplayListPtr++,
                              LIGHT_2,
                              g_adwModelShadeColors[part->flags & MODEL_PART_LIGHT_RAMP]);
            }
            if (part->flags & MODEL_PART_TWO_SIDED) {
                gSPClearGeometryMode(gDisplayListPtr++, G_CULL_BACK | G_CULL_FRONT);
            }
            if ((part->flags & MODEL_PART_FLAT) && g_bSmoothShading) {
                gSPClearGeometryMode(gDisplayListPtr++, G_SHADING_SMOOTH);
            }
            gSPTexture(gDisplayListPtr++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
            gDPTileSync(gDisplayListPtr++);
#if TGR_PLATFORM == TGR_PLATFORM_WIN
            block = AllocDisplayListBlock()->matrix;
#else
            block = (Mtx *)AllocDisplayListBlock();
#endif
            guTranslateF(g_afMvpMatrix, part->position.x, part->position.y, part->position.z);
#if TGR_PLATFORM == TGR_PLATFORM_WIN
            CopyMatrix4x4(g_afMvpMatrix, block);
#else
            guMtxF2L(g_afMvpMatrix, block);
#endif
            gSPMatrix(gDisplayListPtr++, block, G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
            gSPDisplayList(gDisplayListPtr++, part->displayList);
            gSPPopMatrix(gDisplayListPtr++, G_MTX_MODELVIEW);
            if ((part->flags & MODEL_PART_FLAT) && g_bSmoothShading) {
                gSPSetGeometryMode(gDisplayListPtr++, G_SHADING_SMOOTH);
            }
            if (part->flags & MODEL_PART_TWO_SIDED) {
                gSPSetGeometryMode(gDisplayListPtr++,
                                   g_dwViewportMirrored != g_dwTrackMirrored ? G_CULL_FRONT :
                                                                               G_CULL_BACK);
            }
            if (part->flags & MODEL_PART_RAMPED) {
                gSPLightColor(gDisplayListPtr++, LIGHT_1, 0xFFFFFF00);
                gSPLightColor(gDisplayListPtr++, LIGHT_2, 0x40404000);
            }
        }
    }
    gSPPopMatrix(gDisplayListPtr++, G_MTX_MODELVIEW);
}
