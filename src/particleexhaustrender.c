#include "effects.h"
#include "particlecommands.h"
#if TGR_PC
#include "win32/particlesnapshot.h"
#endif

#if TGR_PC
void RenderExhaustAndTireSpray(const ViewportRect *viewports, const RacerSlot *slots) {
#else
void RenderExhaustAndTireSpray(void) {
#endif
    ConfigureExhaustParticleTexture();
    gDPSetDepthSource(gDisplayListPtr++, G_ZS_PRIM);
    gDPSetColorDither(gDisplayListPtr++, G_CD_DISABLE);
    if (g_dwSnowMode) {
        /* The colour dither's bits, which leave the alpha dither at its pattern. */
        gDPSetAlphaDither(gDisplayListPtr++, G_CD_NOISE);
        gDPSetRenderMode(gDisplayListPtr++,
                         Z_CMP | IM_RD | CVG_DST_SAVE | ZMODE_XLU | FORCE_BL |
                             GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA),
                         GBL_c2(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA));
#if TGR_PC
        RenderBillboardParticles(viewports, g_wRenderExhaustParticleListHead, 0xE0, 0xE0, 0xFF);
#else
        RenderBillboardParticles(g_wExhaustParticleListHead, 0xE0, 0xE0, 0xFF);
#endif
    }
    RestoreParticleModes();
#if TGR_PC
    DrawTireSpray(slots);
#else
    DrawTireSpray();
#endif
}
