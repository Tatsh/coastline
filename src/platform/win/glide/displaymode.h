/**
 * @file
 * The display modes of the Glide library, the fixed resolutions that fit in the frame buffer
 * memory of the board.
 */
#ifndef COASTLINE_PLATFORM_WIN_GLIDE_DISPLAYMODE_H
#define COASTLINE_PLATFORM_WIN_GLIDE_DISPLAYMODE_H

/** A display mode of the Glide library, linked in ascending order of size. */
struct GlideDisplayModeNode {
    int nWidth;                  /*!< The width in pixels. */
    int nHeight;                 /*!< The height in pixels. */
    int nBitDepth;               /*!< The bits of a pixel, 16 for every mode. */
    int nRefreshRate;            /*!< The refresh rate in hertz, 0 for every mode. */
    GlideDisplayModeNode *pNext; /*!< The next larger mode. */

    /**
     * Construct an empty mode.
     *
     * @ghidraAddress BRGlide: 0x10058c70
     */
    GlideDisplayModeNode();

    /**
     * Delete the modes that follow this one.
     *
     * @ghidraAddress BRGlide: 0x10058c90
     */
    void DeleteChain();

    /**
     * Whether a mode is no larger than the mode that follows this one in each of its bit depth,
     * width, and height.
     *
     * @param pMode The mode.
     * @return False also when the mode or the next mode is missing.
     * @ghidraAddress BRGlide: 0x10058cc0
     */
    bool IsLessOrEqual(const GlideDisplayModeNode *pMode) const;

    /**
     * Insert a mode after this one, before the first following mode it is no larger than. The
     * mode is never inserted before this one.
     *
     * @param pMode The mode.
     * @ghidraAddress BRGlide: 0x10058d00
     */
    void InsertSorted(GlideDisplayModeNode *pMode);
};

/** A resolution of g_aGlideResolutions. */
struct GlideResolution {
    int nWidth;  /*!< The width in pixels. */
    int nHeight; /*!< The height in pixels. */
};

/** The number of entries of g_aGlideResolutions. */
#define GLIDE_RESOLUTION_COUNT 7

/**
 * The resolutions the Glide library offers.
 *
 * @ghidraAddress BRGlide: 0x100ac908
 */
extern const GlideResolution g_aGlideResolutions[GLIDE_RESOLUTION_COUNT];

/**
 * The display modes, smallest first.
 *
 * @ghidraAddress BRGlide: 0x10ac5dc8
 */
extern GlideDisplayModeNode *g_pGlideDisplayModeList;

/**
 * The number of modes of g_pGlideDisplayModeList.
 *
 * @ghidraAddress BRGlide: 0x10ac5dcc
 */
extern int g_dwGlideDisplayModeCount;

/**
 * Build g_pGlideDisplayModeList from the resolutions whose two 16-bit buffers and depth buffer fit
 * in the frame buffer memory.
 *
 * @ghidraAddress BRGlide: 0x10058d40
 */
void BuildGlideDisplayModeList(void);

#endif /* COASTLINE_PLATFORM_WIN_GLIDE_DISPLAYMODE_H */
