#ifndef COASTLINE_PLATFORM_WIN_GLIDE_DISPLAYLIST_H
#define COASTLINE_PLATFORM_WIN_GLIDE_DISPLAYLIST_H

/**
 * @file
 * The parts of the interpreter of the display lists of the game that only the Glide renderer has:
 * the render state it records, its fog, and its vertices drawn without the depth buffer.
 */

#include "platform/win/displaylist.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The mode of grCullMode() last set.
 *
 * @ghidraAddress BRGlide: 0x105d17d8
 */
extern int g_dwGlideCullMode;

/**
 * The comparison of grDepthBufferFunction() last set.
 *
 * @ghidraAddress BRGlide: 0x105ce2e0
 */
extern int g_dwGlideDepthFunction;

/**
 * The comparison of grAlphaTestFunction() last set.
 *
 * @ghidraAddress BRGlide: 0x105ccc70
 */
extern int g_dwGlideAlphaTestFunction;

/**
 * The factor of the source colour of grAlphaBlendFunction() last set.
 *
 * @ghidraAddress BRGlide: 0x105d17a8
 */
extern int g_dwGlideBlendRgbSource;

/**
 * The factor of the destination colour of grAlphaBlendFunction() last set.
 *
 * @ghidraAddress BRGlide: 0x105d1758
 */
extern int g_dwGlideBlendRgbDest;

/**
 * The factor of the source alpha of grAlphaBlendFunction() last set.
 *
 * @ghidraAddress BRGlide: 0x105ccfd4
 */
extern int g_dwGlideBlendAlphaSource;

/**
 * The factor of the destination alpha of grAlphaBlendFunction() last set.
 *
 * @ghidraAddress BRGlide: 0x105ccfe4
 */
extern int g_dwGlideBlendAlphaDest;

/**
 * The table of fog UpdateVertexFogRange() gives Glide.
 *
 * @ghidraAddress BRGlide: 0x105d1718
 */
extern GrFog_t g_aGlideFogTable[GR_FOG_TABLE_SIZE];

/**
 * The integer that TransformVertexToScreen() rounds a coordinate through.
 *
 * @ghidraAddress BRGlide: 0x105ce310
 */
extern int g_nGbiVertexSnap;

/**
 * Take a vertex inside the view to the screen with a colour, as TransformVertexToScreen() does, and
 * give it the w of the far plane.
 *
 * @param pDest The vertex on the screen.
 * @param pSource The vertex as it was loaded.
 * @param fRed The red, from 0 to 255.
 * @param fGreen The green, from 0 to 255.
 * @param fBlue The blue, from 0 to 255.
 * @ghidraAddress BRGlide: 0x10023760
 */
void TransformVertexToScreenNoZBuffer(
    GrVertex *pDest, const GbiVertexSource *pSource, float fRed, float fGreen, float fBlue);

/**
 * Give Glide the table of fog of the weather. Without weather, the table last made is given again.
 *
 * @ghidraAddress BRGlide: 0x10023aa0
 */
void UpdateVertexFogRange(void);

/**
 * Convert pixels of eight bits a channel, red first, to pixels of five bits a colour and one of
 * alpha.
 *
 * @param pDest The converted pixels.
 * @param pSource The pixels to convert.
 * @param nBytes The bytes of the pixels to convert, rounded up to a whole pixel.
 * @return The bytes of the converted pixels.
 * @ghidraAddress BRGlide: 0x10023cb0
 */
int ConvertRgba32ToArgb1555(unsigned short *pDest, const unsigned char *pSource, int nBytes);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_PLATFORM_WIN_GLIDE_DISPLAYLIST_H */
