#ifndef COASTLINE_TEXTUREWORKSPACE_H
#define COASTLINE_TEXTUREWORKSPACE_H

#define TEXTURE_DECODE_WORKSPACE_BYTES 0x80000
/** The most pixels an image file of RGBA5551 has, 256 by 256. */
#define RGBA5551_IMAGE_PIXEL_COUNT 0x10000

#ifdef __cplusplus
extern "C" {
#endif

/** Return the shared decoded-pixel workspace. */
unsigned char *GetTextureDecodeBuffer(void);

/** Return the RGBA5551 input view within the decoded-pixel workspace. */
unsigned short *GetRgba5551FileBuffer(void);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_TEXTUREWORKSPACE_H */
