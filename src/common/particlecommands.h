#ifndef COASTLINE_PARTICLE_COMMANDS_H
#define COASTLINE_PARTICLE_COMMANDS_H

#include "common/system.h"
#include "common/weather.h"

#if TGR_PC
#include "win32/displaylist.h"
#include "win32/render.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Load the console particle-image commands retained by the exhaust path. */
#define LoadParticleTextureImage()                                                                 \
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

#if TGR_PC
static inline void WriteParticleCombineMode(void) {
    PackCombineMode(gDisplayListPtr++,
                    0,
                    0,
                    0,
                    1000 + G_CCMUX_PRIMITIVE,
                    1000 + G_ACMUX_TEXEL0,
                    0,
                    1000 + G_ACMUX_PRIMITIVE,
                    0,
                    0,
                    0,
                    0,
                    1000 + G_CCMUX_PRIMITIVE,
                    1000 + G_ACMUX_TEXEL0,
                    0,
                    1000 + G_ACMUX_PRIMITIVE,
                    0);
}

/* Configure the primitive-colour particle texture for the Windows renderer. */
static inline void ConfigureParticleTexture(void) {
    gDPSetRenderMode(gDisplayListPtr++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
    WriteParticleCombineMode();
    gDisplayListWords(gDisplayListPtr++,
                      ((u32)GBI_OP_BIND_TEXTURE_ENTRY << 24) | (g_dwParticleTextureId & 0xffffff),
                      1);
}

static inline void ConfigureExhaustParticleTexture(void) {
    gDPPipeSync(gDisplayListPtr++);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_1CYCLE);
    gSPTexture(gDisplayListPtr++, 0xffff, 0xffff, 0, G_TX_RENDERTILE, G_ON);
    gDPSetTextureFilter(gDisplayListPtr++, g_dwTextureFilterMode);
    WriteParticleCombineMode();
    gDisplayListWords(gDisplayListPtr++,
                      ((u32)GBI_OP_BIND_TEXTURE_ENTRY << 24) |
                          (g_dwHudOverlayHalfTextureId & 0xffffff),
                      1);
    LoadParticleTextureImage();
}

#else
/* Load the particle texture to be drawn as rectangles of the primitive colour. */
#define ConfigureParticleTexture()                                                                 \
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
    LoadParticleTextureImage()
#define ConfigureExhaustParticleTexture() ConfigureParticleTexture()

#endif /* TGR_PC */

/* Put back the modes the rest of the frame draws with. */
#define RestoreParticleModes()                                                                     \
    gDPPipeSync(gDisplayListPtr++);                                                                \
    gDPSetTexturePersp(gDisplayListPtr++, G_TP_PERSP);                                             \
    gDPSetDepthSource(gDisplayListPtr++, G_ZS_PIXEL);                                              \
    gDPSetColorDither(gDisplayListPtr++, g_dwColorDitherMode);                                     \
    gDPSetAlphaDither(gDisplayListPtr++, g_dwAlphaDitherMode);                                     \
    gDPSetAlphaCompare(gDisplayListPtr++, G_AC_THRESHOLD)

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_PARTICLE_COMMANDS_H */
