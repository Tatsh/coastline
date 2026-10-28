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
    GLIDE_SST_WINDOW_OPEN = 2,   /*!< The Glide window is open, as it starts. */
};

/**
 * Whether the Glide window is open, one of #GlideSstWindowState.
 *
 * @ghidraAddress BRGlide: 0x100a9354
 */
extern int g_dwGlideSstWindowState;

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_PLATFORM_WIN_GLIDE_DEVICE_H */
