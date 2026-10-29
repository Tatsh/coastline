#ifndef COASTLINE_TEXTUREWORKSPACE_H
#define COASTLINE_TEXTUREWORKSPACE_H

#define TEXTURE_DECODE_WORKSPACE_BYTES 0x80000
/** The most pixels an image file of RGBA5551 has, 256 by 256. */
#define RGBA5551_IMAGE_PIXEL_COUNT 0x10000

#ifdef __cplusplus
extern "C" {
#endif

/** The shared workspace of decoded pixels. */
typedef union {
    unsigned char decoded[TEXTURE_DECODE_WORKSPACE_BYTES]; /*!< The decoded pixels. */
    /** The workspace as RGBA5551 pixels, with file pixels from RGBA5551_IMAGE_PIXEL_COUNT. */
    unsigned short rgba5551[TEXTURE_DECODE_WORKSPACE_BYTES / sizeof(unsigned short)];
} TextureDecodeWorkspace;

/**
 * @ghidraAddress BRD3D: 0x11829870
 * @ghidraAddress BRGlide: 0x1186c988
 */
extern TextureDecodeWorkspace g_TextureDecodeWorkspace;

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_TEXTUREWORKSPACE_H */
