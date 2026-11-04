#include "common/effects.h"
#include "common/particlecommands.h"
#if TGR_BOSS_RALLY
#include "common/particlesnapshot.h"
#endif

#if TGR_BOSS_RALLY
void RenderExhaustAndTireSpray(const ViewportRect *viewports, const RacerSlot *slots) {
#else
void RenderExhaustAndTireSpray(void) {
#endif
#if TGR_BOSS_RALLY
    ConfigureExhaustParticleTexture();
#else
    /* Separate command blocks preserve IDO's word-store order. */
    gDPPipeSync(gDisplayListPtr++);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_1CYCLE);
    {
        Gfx *command = gDisplayListPtr++;
        command->words.w0 = 0xbb000001;
        command->words.w1 = 0xffffffff;
    }
    gDPSetTextureFilter(gDisplayListPtr++, g_dwTextureFilterMode);
    gDPSetCombineLERP(gDisplayListPtr++,
                      0,
                      0,
                      0,
                      PRIMITIVE,
                      TEXEL0,
                      0,
                      PRIMITIVE,
                      0,
                      0,
                      0,
                      0,
                      PRIMITIVE,
                      TEXEL0,
                      0,
                      PRIMITIVE,
                      0);
    {
        Gfx *command = gDisplayListPtr++;
        command->words.w0 = 0xfd900000;
        command->words.w1 = (u32)g_aParticleTexture;
    }
    {
        Gfx *command = gDisplayListPtr++;
        command->words.w0 = 0xf5900000;
        command->words.w1 = 0x07018060;
    }
    gDPLoadSync(gDisplayListPtr++);
    {
        Gfx *command = gDisplayListPtr++;
        command->words.w0 = 0xf3000000;
        command->words.w1 = 0x077ff100;
    }
    gDPPipeSync(gDisplayListPtr++);
    {
        Gfx *command = gDisplayListPtr++;
        command->words.w0 = 0xf5881000;
        command->words.w1 = 0x00018060;
    }
    {
        Gfx *command = gDisplayListPtr++;
        command->words.w0 = 0xf2000000;
        command->words.w1 = 0x000fc0fc;
    }
    gDPSetTextureLUT(gDisplayListPtr++, G_TT_NONE);
    gDPSetTexturePersp(gDisplayListPtr++, G_TP_NONE);
#endif
    gDPSetDepthSource(gDisplayListPtr++, G_ZS_PRIM);
    gDPSetColorDither(gDisplayListPtr++, G_CD_DISABLE);
    if (g_dwSnowMode) {
        /* The colour dither's bits, which leave the alpha dither at its pattern. */
        gDPSetAlphaDither(gDisplayListPtr++, G_CD_NOISE);
        gDPSetRenderMode(gDisplayListPtr++,
                         Z_CMP | IM_RD | CVG_DST_SAVE | ZMODE_XLU | FORCE_BL |
                             GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA),
                         GBL_c2(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA));
#if TGR_BOSS_RALLY
        RenderBillboardParticles(viewports, g_wRenderExhaustParticleListHead, 0xE0, 0xE0, 0xFF);
#else
        RenderBillboardParticles(g_wExhaustParticleListHead, 0xE0, 0xE0, 0xFF);
#endif
    }
    RestoreParticleModes();
#if TGR_BOSS_RALLY
    DrawTireSpray(slots);
#else
    DrawTireSpray();
#endif
}
