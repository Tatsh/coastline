#ifndef COASTLINE_RENDERBACKEND_H
#define COASTLINE_RENDERBACKEND_H

struct GbiCommand;

#ifdef __cplusplus
extern "C" {
#endif

#if TGR_PLATFORM != TGR_PLATFORM_SDL
/**
 * Submit a terminated display list through the selected render backend.
 *
 * @ghidraAddress BRD3D: 0x10b501d0
 */
extern void (*g_pfnRenderBackendSubmitDisplayList)(struct GbiCommand *pDisplayList);

/**
 * Replace one texture entry's device surface with another entry's surface.
 *
 * @ghidraAddress BRD3D: 0x118aa0a4
 */
extern void (*g_pfnAliasTextureEntrySurface)(int nEntry, int nSourceEntry);

/**
 * Create a backend texture from an N64 image, palette, dimensions, and tile options.
 * The result is a texture-entry index, as returned by CreateTextureFromN64Image().
 *
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
 * Decode an N64 image into caller-provided pixel storage and return its decoded byte count.
 * Dimensions are in texels; lineBytes, size, and format describe the source RDP tile.
 *
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
#endif

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

#if TGR_PLATFORM != TGR_PLATFORM_SDL
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
 * The byte stride between adjacent converted alpha-ramp texels.
 *
 * @ghidraAddress BRD3D: 0x11829318
 */
extern int g_dwAlphaRampStripRowCount;
#endif

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
#if TGR_PLATFORM != TGR_PLATFORM_SDL
extern int g_adwPrecipitationTextureIds[PRECIPITATION_TEXTURE_COUNT];
#endif

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

#if TGR_PLATFORM == TGR_PLATFORM_SDL
/** Renderer hooks and effect texture entries for one SDL race engine. */
typedef struct {
    /** Submit a terminated display list through the active renderer. */
    void (*submitDisplayList)(struct GbiCommand *displayList);
    /** Alias entry to source; the default SDL hook does nothing. */
    void (*aliasTexture)(int entry, int source);
    /**
     * Create a texture entry from an N64 image and palette, returning its index.
     * Dimensions are in texels; lineBytes, size, and format describe the source RDP tile.
     * Remaining options describe mirroring, clamping, coordinate shifts, and row swapping.
     * The default SDL hook accepts only 8-bit colour-indexed images, returning zero otherwise.
     */
    unsigned (*createTexture)(const unsigned char *image,
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
     * Decode an N64 image into caller-provided storage and return its decoded byte count.
     * Dimensions are in texels; lineBytes, size, and format describe the source RDP tile.
     * The default SDL hook returns zero for formats other than 8-bit colour-indexed images.
     */
    int (*decodeTexture)(void *destination,
                         const unsigned char *image,
                         const unsigned char *palette,
                         int width,
                         int height,
                         int lineBytes,
                         int size,
                         int format);
    void (*initialize)(void);         /*!< Initialise the selected render backend. */
    void (*initializeTextures)(void); /*!< Initialise its texture subsystem. */
    void (*shutdown)(void);           /*!< Release the active render backend. */
    void (*shutdownTextures)(void);   /*!< Release its texture subsystem. */
    void (*updateFog)(void);          /*!< Apply the current race fog settings to the renderer. */
    int forceTextureClone; /*!< Bypass texture metadata matching when creating a device texture. */
    int disableCarShadow;  /*!< Suppress car shadow rendering when nonzero. */
    unsigned hudOverlay;   /*!< The texture entry for the race HUD overlay. */
    unsigned hudOverlayHalf; /*!< The half-scale HUD overlay entry, or hudOverlay when shared. */
    unsigned particles;      /*!< The texture entry for race particle effects. */
    unsigned tireTrail;      /*!< The texture entry for tyre trails. */
    unsigned vehicleEnvironment;  /*!< The night-time vehicle reflection texture entry. */
    unsigned fallbackEnvironment; /*!< The daytime vehicle reflection texture entry. */
    int alphaRampRows; /*!< The byte stride between adjacent converted alpha-ramp texels. */
    int precipitation[PRECIPITATION_TEXTURE_COUNT]; /*!< Texture entries for precipitation. */
} SdlRenderBackendState;

/** @return Renderer storage for the active SDL texture collection. */
SdlRenderBackendState *GetSdlRenderBackendState(void);

/** @copydoc SdlRenderBackendState::submitDisplayList */
#define g_pfnRenderBackendSubmitDisplayList (GetSdlRenderBackendState()->submitDisplayList)
/** @copydoc SdlRenderBackendState::aliasTexture */
#define g_pfnAliasTextureEntrySurface (GetSdlRenderBackendState()->aliasTexture)
/** @copydoc SdlRenderBackendState::createTexture */
#define g_pfnCreateTextureFromN64Image (GetSdlRenderBackendState()->createTexture)
/** @copydoc SdlRenderBackendState::decodeTexture */
#define g_pfnDecodeN64TextureToPixelBuffer (GetSdlRenderBackendState()->decodeTexture)
/** @copydoc SdlRenderBackendState::initialize */
#define g_pfnRenderBackendInit (GetSdlRenderBackendState()->initialize)
/** @copydoc SdlRenderBackendState::initializeTextures */
#define g_pfnInitializeTextureSubsystem (GetSdlRenderBackendState()->initializeTextures)
/** @copydoc SdlRenderBackendState::shutdown */
#define g_pfnRenderBackendShutdown (GetSdlRenderBackendState()->shutdown)
/** @copydoc SdlRenderBackendState::shutdownTextures */
#define g_pfnShutdownTextureSubsystem (GetSdlRenderBackendState()->shutdownTextures)
/** @copydoc SdlRenderBackendState::updateFog */
#define g_pfnRenderBackendUpdateFog (GetSdlRenderBackendState()->updateFog)
/** @copydoc SdlRenderBackendState::forceTextureClone */
#define g_bForceTextureClone (GetSdlRenderBackendState()->forceTextureClone)
/** @copydoc SdlRenderBackendState::disableCarShadow */
#define g_bDisableCarShadow (GetSdlRenderBackendState()->disableCarShadow)
/** @copydoc SdlRenderBackendState::hudOverlay */
#define g_dwHudOverlayTextureId (GetSdlRenderBackendState()->hudOverlay)
/** @copydoc SdlRenderBackendState::hudOverlayHalf */
#define g_dwHudOverlayHalfTextureId (GetSdlRenderBackendState()->hudOverlayHalf)
/** @copydoc SdlRenderBackendState::particles */
#define g_dwParticleTextureId (GetSdlRenderBackendState()->particles)
/** @copydoc SdlRenderBackendState::tireTrail */
#define g_dwTireTrailTextureId (GetSdlRenderBackendState()->tireTrail)
/** @copydoc SdlRenderBackendState::vehicleEnvironment */
#define g_dwVehicleEnvMapTextureId (GetSdlRenderBackendState()->vehicleEnvironment)
/** @copydoc SdlRenderBackendState::fallbackEnvironment */
#define g_dwFallbackEnvMapTextureId (GetSdlRenderBackendState()->fallbackEnvironment)
/** @copydoc SdlRenderBackendState::alphaRampRows */
#define g_dwAlphaRampStripRowCount (GetSdlRenderBackendState()->alphaRampRows)
/** @copydoc SdlRenderBackendState::precipitation */
#define g_adwPrecipitationTextureIds (GetSdlRenderBackendState()->precipitation)
#endif

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_RENDERBACKEND_H */
