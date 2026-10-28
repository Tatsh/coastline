/**
 * @file
 * The Glide window of the screen, which the Glide library opens and closes with the activation of
 * the application.
 */
#ifndef COASTLINE_PLATFORM_WIN_GLIDE_DEVICE_H
#define COASTLINE_PLATFORM_WIN_GLIDE_DEVICE_H

#ifdef __cplusplus
extern "C" {
#endif

/** The values of g_dwGlideSstWindowState. */
enum GlideSstWindowState {
    GLIDE_SST_WINDOW_CLOSED = 1, /*!< The application went behind and closed the Glide window. */
    GLIDE_SST_WINDOW_OPEN = 2,   /*!< The Glide window was opened again. */
};

/**
 * Whether the Glide window is open, one of #GlideSstWindowState.
 *
 * @ghidraAddress BRGlide: 0x100a9354
 */
extern int g_dwGlideSstWindowState;

/**
 * Open the Glide window at the resolution of g_dwConfigFramebufferWidth and
 * g_dwConfigFramebufferHeight.
 *
 * @param nWidth The width, which the routine does not read.
 * @param nHeight The height, which the routine does not read.
 * @return Nonzero when the window opened.
 * @ghidraAddress BRGlide: 0x1001dd80
 */
int OpenGlideWindowForResolution(int nWidth, int nHeight);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_PLATFORM_WIN_GLIDE_DEVICE_H */
