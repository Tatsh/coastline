#ifndef COASTLINE_WIN_RENDERBACKEND_H
#define COASTLINE_WIN_RENDERBACKEND_H

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

/** @ghidraAddress BRD3D: 0x118aa0ac */
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

/** @ghidraAddress BRD3D: 0x118aa0a8 */
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
 */
extern int g_bForceTextureClone;

#ifdef __cplusplus
}
#endif

#endif
