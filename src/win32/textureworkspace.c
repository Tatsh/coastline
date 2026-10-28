#include "win32/textureworkspace.h"

typedef union {
    unsigned char decoded[TEXTURE_DECODE_WORKSPACE_BYTES];
    unsigned short rgba5551[TEXTURE_DECODE_WORKSPACE_BYTES / sizeof(unsigned short)];
} TextureDecodeWorkspace;

/* @ghidraAddress BRD3D: 0x11829870 */
/* @ghidraAddress BRGlide: 0x1186c988 */
static TextureDecodeWorkspace g_TextureDecodeWorkspace;

unsigned char *GetTextureDecodeBuffer(void) {
    return g_TextureDecodeWorkspace.decoded;
}

unsigned short *GetRgba5551FileBuffer(void) {
    return g_TextureDecodeWorkspace.rgba5551 + RGBA5551_IMAGE_PIXEL_COUNT;
}
