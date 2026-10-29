#ifndef COASTLINE_RENDERBACKEND_H
#define COASTLINE_RENDERBACKEND_H

struct GbiCommand;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ghidraAddress BRD3D: 0x10b501d0
 */
extern void (*g_pfnRenderBackendSubmitDisplayList)(struct GbiCommand *pDisplayList);

/**
 * @ghidraAddress BRD3D: 0x118aa0a4
 */
extern void (*g_pfnAliasTextureEntrySurface)(int nEntry, int nSourceEntry);

/**
 * @ghidraAddress BRD3D: 0x118aa0ac
 * @ghidraAddress BRGlide: 0x118ed1c4
 */
extern unsigned (*g_pfnCreateTextureFromN64Image)(const unsigned char *image,
                                                  const unsigned char *palette,
                                                  int width,
                                                  int height,
                                                  int lineBytes,
                                                  int size,
                                                  int format,
                                                  int option7,
                                                  int option8,
                                                  int option9,
                                                  int option10,
                                                  int option11,
                                                  int option12,
                                                  int option13,
                                                  int option14);

/**
 * @ghidraAddress BRD3D: 0x118aa0a8
 * @ghidraAddress BRGlide: 0x118ed1c0
 */
extern int (*g_pfnDecodeN64TextureToPixelBuffer)(void *destination,
                                                 const unsigned char *image,
                                                 const unsigned char *palette,
                                                 int width,
                                                 int height,
                                                 int lineBytes,
                                                 int size,
                                                 int format);

/**
 * Bypass texture metadata matching while creating a new device texture.
 * @ghidraAddress BRD3D: 0x118aa09c
 * @ghidraAddress BRGlide: 0x118ed1b4
 */
extern int g_bForceTextureClone;

/**
 * Start the renderer of the backend, set by SetRenderBackendMode().
 *
 * @ghidraAddress BRD3D: 0x10b501c8
 * @ghidraAddress BRGlide: 0x10b73528
 */
extern void (*g_pfnRenderBackendInit)(void);

/**
 * Start the texture subsystem, set by SetRenderBackendMode().
 *
 * @ghidraAddress BRD3D: 0x100b8c94
 * @ghidraAddress BRGlide: 0x100b849c
 */
extern void (*g_pfnInitializeTextureSubsystem)(void);

/**
 * Release the renderer of the backend, if one is set.
 *
 * @ghidraAddress BRD3D: 0x10b501cc
 * @ghidraAddress BRGlide: 0x10b7352c
 */
extern void (*g_pfnRenderBackendShutdown)(void);

/**
 * Release the texture subsystem, if one is set.
 *
 * @ghidraAddress BRD3D: 0x118aa0d0
 * @ghidraAddress BRGlide: 0x118ed1e8
 */
extern void (*g_pfnShutdownTextureSubsystem)(void);

/**
 * Do nothing.
 *
 * @param pMemory Not read.
 * @ghidraAddress BRD3D: 0x10075330
 * @ghidraAddress BRGlide: 0x1006e590
 */
void NullStub2(void *pMemory);

/**
 * Link every vertex of g_aShadowClipVertexPool into its free list, in order.
 *
 * @ghidraAddress BRD3D: 0x1000f460
 * @ghidraAddress BRGlide: 0x1000c9c0
 */
void ResetRenderCommandBufferStack(void);

/**
 * Clear the tracks of skid marks.
 *
 * @ghidraAddress BRD3D: 0x1000f620
 * @ghidraAddress BRGlide: 0x1000cb80
 */
void ClearSplitScreenRenderState(void);

/**
 * The texture of the overlay of the HUD.
 *
 * @ghidraAddress BRD3D: 0x11829110
 * @ghidraAddress BRGlide: 0x1184c470
 */
extern unsigned g_dwHudOverlayTextureId;

/**
 * The texture of the overlay of the HUD at half scale, or g_dwHudOverlayTextureId where the
 * renderer does not scale it.
 *
 * @ghidraAddress BRD3D: 0x1182931c
 * @ghidraAddress BRGlide: 0x1184c478
 */
extern unsigned g_dwHudOverlayHalfTextureId;

/**
 * The texture of the particles.
 *
 * @ghidraAddress BRD3D: 0x11829328
 * @ghidraAddress BRGlide: 0x1184c484
 */
extern unsigned g_dwParticleTextureId;

/**
 * The texture of the trails of the tyres.
 *
 * @ghidraAddress BRD3D: 0x11829108
 * @ghidraAddress BRGlide: 0x1184c468
 */
extern unsigned g_dwTireTrailTextureId;

/**
 * The texture of the map of the environment the vehicles reflect at night.
 *
 * @ghidraAddress BRD3D: 0x11829314
 * @ghidraAddress BRGlide: 0x1184c474
 */
extern unsigned g_dwVehicleEnvMapTextureId;

/**
 * The texture of the map of the environment the vehicles reflect by day.
 *
 * @ghidraAddress BRD3D: 0x11829324
 * @ghidraAddress BRGlide: 0x1184c480
 */
extern unsigned g_dwFallbackEnvMapTextureId;

/**
 * @ghidraAddress BRD3D: 0x11829318
 */
extern int g_dwAlphaRampStripRowCount;

/**
 * Load and create the textures every race uses: the overlay and the glyphs of the display of the
 * race, the particles, the trails of the tyres, the maps of the environment, and the ramps of alpha
 * and precipitation.
 *
 * @ghidraAddress BRD3D: 0x10074dd0
 * @ghidraAddress BRGlide: 0x1006e030
 */
void CreateGlobalEffectTextures(void);

/**
 * Empty the pool of the data of the models.
 *
 * @ghidraAddress BRD3D: 0x1002b950
 * @ghidraAddress BRGlide: 0x100189c0
 */
void ResetModelDataPool(void);

/** The number of entries of g_adwPrecipitationTextureIds. */
#define PRECIPITATION_TEXTURE_COUNT 2

/**
 * The entries of g_pTextureEntries of the textures of precipitation.
 *
 * @ghidraAddress BRD3D: 0x11829100
 * @ghidraAddress BRGlide: 0x1184c460
 */
extern int g_adwPrecipitationTextureIds[PRECIPITATION_TEXTURE_COUNT];

#if TGR_BRD3D
/**
 * Read the size of a texture of precipitation into g_dwPrecipTextureWidth and
 * g_dwPrecipTextureHeight.
 *
 * @param nIndex The entry of g_adwPrecipitationTextureIds.
 * @ghidraAddress BRD3D: 0x1001a4b0
 */
void GetPrecipitationTextureDimensions(int nIndex);

/**
 * Copy one converted ramp texel into a scaled precipitation texture position.
 *
 * @param nTileIndex The texture page.
 * @param nSrcX The coordinate scaled by texture height.
 * @param nSrcY The coordinate scaled by texture width.
 * @param nSrcRow The converted ramp texel.
 * @ghidraAddress BRD3D: 0x1001a4d0
 */
void CopyTileColumnScaled(int nTileIndex, int nSrcX, int nSrcY, int nSrcRow);
#endif /* TGR_BRD3D */

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_RENDERBACKEND_H */
