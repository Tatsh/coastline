#ifndef COASTLINE_TEXRECT_H
#define COASTLINE_TEXRECT_H

/**
 * @file
 * The textures of the interface and their drawing as rectangles of the screen.
 */

#include <PR/ultratypes.h>

/** A compressed texture of the cartridge and the memory it is loaded into. */
typedef struct {
    /** Where the compressed texture starts on the cartridge. */
    u32 romStart;
    /** Where it ends, which is where the next asset starts. */
    u32 romEnd;
    /** The decompressed texels, or NULL before the texture is loaded. */
    u8 *pixels;
    u16 width;
    u16 height;
    /** The format and texel size, as the G_IM_FMT_ and G_IM_SIZ_ values. */
    u8 format;
    u8 size;
} Texture;

/** An allocator of memory for a loaded asset. */
typedef void *(*AssetAllocator)(u32 size);

/** How DrawTexturedRect() combines the texels with its colors. */
enum {
    /** The texels as they are, blended over the picture. */
    TEXRECT_BLEND_TEXTURE,
    /** The primitive color through the texels as a mask. */
    TEXRECT_BLEND_COLOR_THROUGH_TEXTURE,
    /** The same as TEXRECT_BLEND_TEXTURE. */
    TEXRECT_BLEND_TEXTURE_2,
    /** The texels shading from the primitive color to the environment color, blended. */
    TEXRECT_BLEND_SHADED,
    /** The same shading, opaque, with the alpha of the texels times the shade. */
    TEXRECT_BLEND_SHADED_OPAQUE,
};

/**
 * Allocate memory for an uncompressed texture and copy it there from the cartridge.
 *
 * @ghidraAddress USA: 0x8023df00
 */
void AllocAndDmaLoadAsset(Texture *texture, AssetAllocator alloc);

/**
 * Allocate memory for a texture and decompress it there from the cartridge.
 *
 * @ghidraAddress USA: 0x8023df4c
 */
void LoadAndDecompressAsset(Texture *texture, AssetAllocator alloc);

/**
 * Draw a texture over a rectangle of the screen, in strips that fit the texture memory, upside
 * down. The width counts without its sign. The blend mode picks how the primitive and the
 * environment colors and the texels combine.
 *
 * @ghidraAddress USA: 0x8023df9c
 */
void DrawTexturedRect(Texture *texture,
                      s32 x,
                      s32 y,
                      s32 width,
                      s32 height,
                      s32 primRed,
                      s32 primGreen,
                      s32 primBlue,
                      s32 primAlpha,
                      s32 envRed,
                      s32 envGreen,
                      s32 envBlue,
                      s32 envAlpha,
                      s32 blendMode);

/**
 * Get the texture coordinate scale of a texture side, and the bits of the mask that wraps it. A
 * side of more than 1024 texels is reported on the debug display and leaves the mask unset.
 *
 * @ghidraAddress USA: 0x80217614
 */
void GetTextureScaleAndMask(s32 size, s32 *scale, s32 *maskBits);

/**
 * Draw a texture stretched over a rectangle and upside down, shaded from black to red by its
 * intensity. Nothing calls it.
 *
 * @ghidraAddress USA: 0x80239220
 */
void DrawRedIntensityRect(
    u8 *texture, s32 width, s32 height, s32 x, s32 y, s32 drawWidth, s32 drawHeight);

/**
 * Draw a colour through a texture stretched over a rectangle and upside down, with the intensity
 * of the texture as its opacity. A negative drawn width also mirrors it. Nothing calls it.
 *
 * @ghidraAddress USA: 0x80239750
 */
void DrawColoredTexturedRect(u8 *texture,
                             s32 width,
                             s32 height,
                             s32 x,
                             s32 y,
                             s32 drawWidth,
                             s32 drawHeight,
                             s32 red,
                             s32 green,
                             s32 blue);

#endif
