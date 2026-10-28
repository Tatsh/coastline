#ifndef COASTLINE_TEXRECT_H
#define COASTLINE_TEXRECT_H

/**
 * @file
 * The textures of the interface and their drawing as rectangles of the screen.
 */

#include <PR/ultratypes.h>

#ifdef __cplusplus
extern "C" {
#endif

/** A compressed texture of the cartridge and the memory it is loaded into. */
typedef struct {
    u32 romStart; /*!< The cartridge offset of the compressed texture. */
    u32 romEnd; /*!< The cartridge offset of the end of the texture and start of the next asset. */
    u8 *pixels; /*!< The decompressed texels, or NULL before the texture is loaded. */
    u16 width;  /*!< The width in texels. */
    u16 height; /*!< The height in texels. */
    u8 format;  /*!< The texel format, a G_IM_FMT_* value. */
    u8 size;    /*!< The texel size, a G_IM_SIZ_* value. */
} Texture;

/** An allocator of memory for a loaded asset. */
typedef void *(*AssetAllocator)(u32 size);

/** How DrawTexturedRect() combines the texels with its colors. */
enum {
    TEXRECT_BLEND_TEXTURE,               /*!< The texels as they are, blended over the picture. */
    TEXRECT_BLEND_COLOR_THROUGH_TEXTURE, /*!< The primitive color through the texels as a mask. */
    TEXRECT_BLEND_TEXTURE_2,             /*!< The same as TEXRECT_BLEND_TEXTURE. */
    TEXRECT_BLEND_SHADED, /*!< The texels shading from the primitive to the environment color. */
    TEXRECT_BLEND_SHADED_OPAQUE, /*!< The same shading, opaque, alpha times shade. */
};

/**
 * Allocate memory for an uncompressed texture and copy it there from the cartridge.
 *
 * @param texture The texture to load.
 * @param alloc The allocator of the memory.
 * @ghidraAddress USA: 0x8023df00
 * @ghidraAddress EUR: 0x8023f410
 * @ghidraAddress JPN: 0x8023ef10
 */
void AllocAndDmaLoadAsset(Texture *texture, AssetAllocator alloc);

/**
 * Allocate memory for a texture and decompress it there from the cartridge.
 *
 * @param texture The texture to load.
 * @param alloc The allocator of the memory.
 * @ghidraAddress USA: 0x8023df4c
 * @ghidraAddress EUR: 0x8023f45c
 * @ghidraAddress JPN: 0x8023ef5c
 */
void LoadAndDecompressAsset(Texture *texture, AssetAllocator alloc);

/**
 * Draw a texture over a rectangle of the screen, in strips that fit the texture memory, upside
 * down. The width counts without its sign. The blend mode picks how the primitive and the
 * environment colors and the texels combine.
 *
 * @param texture The loaded texture.
 * @param x The left edge, in pixels of the low-resolution screen.
 * @param y The top edge, in pixels of the low-resolution screen.
 * @param width The width, in pixels of the low-resolution screen.
 * @param height The height, in pixels of the low-resolution screen.
 * @param primRed The red component of the primitive color, from 0 to 255.
 * @param primGreen The green component of the primitive color, from 0 to 255.
 * @param primBlue The blue component of the primitive color, from 0 to 255.
 * @param primAlpha The alpha component of the primitive color, from 0 to 255.
 * @param envRed The red component of the environment color, from 0 to 255.
 * @param envGreen The green component of the environment color, from 0 to 255.
 * @param envBlue The blue component of the environment color, from 0 to 255.
 * @param envAlpha The alpha component of the environment color, from 0 to 255.
 * @param blendMode One of the TEXRECT_BLEND_ values.
 * @ghidraAddress USA: 0x8023df9c
 * @ghidraAddress EUR: 0x8023f4ac
 * @ghidraAddress JPN: 0x8023efac
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
 * @param size The length of the side, in texels.
 * @param scale Receives the texture coordinate scale.
 * @param maskBits Receives the number of bits of the mask.
 * @ghidraAddress USA: 0x80217614
 * @ghidraAddress EUR: 0x80218634
 * @ghidraAddress JPN: 0x80217f84
 * @ghidraAddress BRD3D: 0x10031347
 * @ghidraAddress BRGlide: 0x1002a9f7
 */
void GetTextureScaleAndMask(u32 size, s32 *scale, s32 *maskBits);

/**
 * Draw a texture stretched over a rectangle and upside down, shaded from black to red by its
 * intensity. Nothing calls it.
 *
 * @param texture The texels, 4-bit intensities.
 * @param width The width of the texture, in texels.
 * @param height The height of the texture, in texels.
 * @param x The left edge, in pixels of the low-resolution screen.
 * @param y The top edge, in pixels of the low-resolution screen.
 * @param drawWidth The width drawn, in pixels of the low-resolution screen, without its sign.
 * @param drawHeight The height drawn, in pixels of the low-resolution screen.
 * @ghidraAddress USA: 0x80239220
 * @ghidraAddress EUR: 0x8023a730
 * @ghidraAddress JPN: 0x8023a230
 */
void DrawRedIntensityRect(
    u8 *texture, s32 width, s32 height, s32 x, s32 y, s32 drawWidth, s32 drawHeight);

/**
 * Draw a colour through a texture stretched over a rectangle and upside down, with the intensity
 * of the texture as its opacity. A negative drawn width also mirrors it. Nothing calls it.
 *
 * @param texture The texels, 4-bit intensities.
 * @param width The width of the texture, in texels.
 * @param height The height of the texture, in texels.
 * @param x The left edge, in pixels of the low-resolution screen.
 * @param y The top edge, in pixels of the low-resolution screen.
 * @param drawWidth The width drawn, in pixels of the low-resolution screen, negative to mirror.
 * @param drawHeight The height drawn, in pixels of the low-resolution screen.
 * @param red The red component of the colour, from 0 to 255.
 * @param green The green component of the colour, from 0 to 255.
 * @param blue The blue component of the colour, from 0 to 255.
 * @ghidraAddress USA: 0x80239750
 * @ghidraAddress EUR: 0x8023ac60
 * @ghidraAddress JPN: 0x8023a760
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

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_TEXRECT_H */
