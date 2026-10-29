#include <cstdlib>

#include <zlib.h>

#include "common/fileio.h"
#include "common/settings.h"
#include "win32/app.h"
#include "win32/d3d/renderer.h"
#include "win32/glide/texturecache.h"
#include "win32/render.h"

namespace {

// @ghidraAddress BRGlide: 0x1184c488
unsigned char g_abGlideSplashPixels[0x20000];

// The side of the square texture of the image, in texels.
constexpr int SPLASH_TEXTURE_SIZE = 256;

void DrawGlideSplashQuad(GrVertex &topLeft,
                         GrVertex &topRight,
                         GrVertex &bottomLeft,
                         GrVertex &bottomRight) {
    grClipWindow(0, 0, g_dwConfigFramebufferWidth, g_dwConfigFramebufferHeight);
    grBufferClear(0, 0, 0xffff);
    grDrawTriangle(&topLeft, &topRight, &bottomLeft);
    grDrawTriangle(&topRight, &bottomRight, &bottomLeft);
    g_pfnPresentFrame();
}

} // namespace

void ShowGlideSplashImage(const char *pszPath, int nChecksum) {
    GameFile *pFile = CHK_FReadOpen(pszPath);
    int width;
    int height;
    CHK_FRead(&width, 4, 1, pFile);
    CHK_FRead(&height, 4, 1, pFile);
    int bytes = height * width * 2;
    // The word after the size is read into the pixels, and the pixels are read over it.
    CHK_FRead(g_abGlideSplashPixels, 4, 1, pFile);
    CHK_FRead(g_abGlideSplashPixels, bytes, 1, pFile);
    CHK_FClose(pFile);
    if (nChecksum && adler32(adler32(0, nullptr, 0), g_abGlideSplashPixels, bytes) !=
                         static_cast<uLong>(nChecksum)) {
        std::exit(1);
    }
    ResetGlideTmuTextureAllocator();
    int slot = AllocateGlideTmuTexture(GR_TMU0,
                                       GR_MIPMAPLEVELMASK_BOTH,
                                       128,
                                       32,
                                       GR_TEXFMT_ARGB_1555,
                                       GR_MIPMAP_NEAREST_DITHER,
                                       GR_LOD_256,
                                       GR_LOD_256,
                                       GR_ASPECT_1x1,
                                       GR_TEXTURECLAMP_WRAP,
                                       GR_TEXTURECLAMP_WRAP,
                                       GR_TEXTUREFILTER_POINT_SAMPLED,
                                       GR_TEXTUREFILTER_POINT_SAMPLED,
                                       0.0f,
                                       FXFALSE);
    DownloadGlideTextureMipMap(slot, g_abGlideSplashPixels, 0);
    BindGlideTextureSource(slot);
    grTexCombine(GR_TMU0,
                 GR_COMBINE_FUNCTION_LOCAL,
                 GR_COMBINE_FACTOR_NONE,
                 GR_COMBINE_FUNCTION_LOCAL,
                 GR_COMBINE_FACTOR_NONE,
                 FXFALSE,
                 FXFALSE);
    grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,
                   GR_COMBINE_FACTOR_ONE,
                   GR_COMBINE_LOCAL_CONSTANT,
                   GR_COMBINE_OTHER_TEXTURE,
                   FXFALSE);
    float left = (g_dwConfigFramebufferWidth - SPLASH_TEXTURE_SIZE) / 2;
    if (left < 0.0f) {
        left = 0.0f;
    }
    float top = (g_dwConfigFramebufferHeight - SPLASH_TEXTURE_SIZE) / 2;
    if (top < 0.0f) {
        top = 0.0f;
    }
    float right = static_cast<float>(width) + left - 1.0f;
    float bottom = static_cast<float>(height) + top - 1.0f;
    float maxS = static_cast<float>(width - 1);
    float maxT = static_cast<float>(height - 1);
    // The depth members are never set, and the image is drawn upside down.
    GrVertex vertices[4];
    auto place = [&vertices](int index, float x, float y, float s, float t) {
        GrVertex &vertex = vertices[index];
        vertex.x = x;
        vertex.y = y;
        vertex.r = 255.0f;
        vertex.g = 255.0f;
        vertex.b = 255.0f;
        vertex.a = 255.0f;
        vertex.oow = 1.0f;
        vertex.tmuvtx[0].sow = s;
        vertex.tmuvtx[0].tow = t;
    };
    place(0, left, bottom, 0.0f, 0.0f);
    place(1, right, bottom, maxS, 0.0f);
    place(2, left, top, 0.0f, maxT);
    place(3, right, top, maxS, maxT);
    DrawGlideSplashQuad(vertices[0], vertices[1], vertices[2], vertices[3]);
    DrawGlideSplashQuad(vertices[0], vertices[1], vertices[2], vertices[3]);
    ResetGlideTmuTextureAllocator();
}
