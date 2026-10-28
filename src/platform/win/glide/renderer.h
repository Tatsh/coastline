#ifndef COASTLINE_PLATFORM_WIN_GLIDE_RENDERER_H
#define COASTLINE_PLATFORM_WIN_GLIDE_RENDERER_H

/**
 * @file
 * The Glide device of the Glide renderer: its hardware, its window, and the render state each
 * window starts with.
 */

#include <glide.h>
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The units of texture of the Glide hardware.
 *
 * @ghidraAddress BRGlide: 0x105ccbd0
 */
extern int g_nGlideNumTmus;

/**
 * The Glide hardware grSstQueryHardware() found.
 *
 * @ghidraAddress BRGlide: 0x105ccbd8
 */
extern GrHwConfiguration g_GlideHwConfig;

/**
 * The megabytes of the frame buffer of the Glide hardware.
 *
 * @ghidraAddress BRGlide: 0x105ccc6c
 */
extern int g_dwGlideFbRamMB;

/**
 * Wait for the buffers Glide has pending, then show the frame drawn.
 *
 * @ghidraAddress BRGlide: 0x1001dd50
 */
void PresentFrameCallback_GlideBufferSwap(void);

/**
 * Open the Glide window at the size of the frame buffer of the configuration, and set the render
 * state it starts with. A size Glide has no resolution for does not open a window.
 *
 * @param nWidth The width, not read.
 * @param nHeight The height, not read.
 * @return 0 when Glide fails to open the window, and 1 otherwise.
 * @ghidraAddress BRGlide: 0x1001dd80
 */
int OpenGlideWindowForResolution(int nWidth, int nHeight);

/**
 * Set the render state a Glide window starts with: the clip window over the frame buffer of the
 * configuration, the w buffer, the clear of the buffers, and the combines of colour, alpha, and
 * texture.
 *
 * @param nWidth The width, not read.
 * @param nHeight The height, not read.
 * @ghidraAddress BRGlide: 0x1001dfb0
 */
void InitGlideDefaultRenderState(int nWidth, int nHeight);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_PLATFORM_WIN_GLIDE_RENDERER_H */
