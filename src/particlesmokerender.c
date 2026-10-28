#include "effects.h"
#include "particlecommands.h"
#include "trackselect.h"

#if TGR_PC
#include <string.h>

#include "win32/particlesnapshot.h"
#else
#include <PR/os_libc.h>
#endif

#if TGR_PC

#define SMOKE_LIST_HEAD g_wRenderTireSmokeListHead
#define PAVED_SMOKE_LIST_HEAD g_wRenderPavedTireSmokeListHead
#define RENDER_SMOKE_LIST(head, red, green, blue)                                                  \
    RenderBillboardParticles(viewports, head, red, green, blue)
#else
#define SMOKE_LIST_HEAD g_wTireSmokeListHead
#define PAVED_SMOKE_LIST_HEAD g_wPavedTireSmokeListHead
#define RENDER_SMOKE_LIST(head, red, green, blue) RenderBillboardParticles(head, red, green, blue)
#endif /* TGR_PC */

#if TGR_PC
void RenderTireSmoke(const ViewportRect *viewports) {
#else
void RenderTireSmoke(void) {
#endif
    ConfigureParticleTexture();

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
#if TGR_PLATFORM == TGR_PLATFORM_N64
        /* The colour dither's bits, which leave the alpha dither at its pattern. */
        gDPSetAlphaDither(gDisplayListPtr++, G_CD_DISABLE);
        /* The alpha compare is set to dither the smoke along with the render mode. */
        gDPSetRenderMode(gDisplayListPtr++,
                         Z_CMP | Z_UPD | CVG_DST_FULL | ZMODE_OPA | G_AC_DITHER |
                             GBL_c1(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1),
                         GBL_c2(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1));
        gDPSetBlendColor(gDisplayListPtr++, 0, 0, 0, 0);
#endif /* TGR_PLATFORM == TGR_PLATFORM_N64 */
        switch (g_dwSelectedTrackId) {
        case TRACK_MOUNTAIN:
        case TRACK_MIRROR_MOUNTAIN:
            RENDER_SMOKE_LIST(SMOKE_LIST_HEAD, 0x60, 0x54, 0x38);
            RENDER_SMOKE_LIST(PAVED_SMOKE_LIST_HEAD, 0x60, 0x5C, 0x50);
            break;
        case TRACK_JUNGLE:
        case TRACK_MIRROR_JUNGLE:
            RENDER_SMOKE_LIST(SMOKE_LIST_HEAD, 0x70, 0x58, 0x38);
            RENDER_SMOKE_LIST(PAVED_SMOKE_LIST_HEAD, 0x70, 0x68, 0x58);
            break;
        default:
            RENDER_SMOKE_LIST(SMOKE_LIST_HEAD, 0xA0, 0x88, 0x60);
            RENDER_SMOKE_LIST(PAVED_SMOKE_LIST_HEAD, 0x70, 0x68, 0x58);
            break;
        }
    }
    RestoreParticleModes();
}
