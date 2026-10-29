#include "win32/glide/renderer.h"

#include "common/screen.h"
#include "common/settings.h"
#include "win32/render.h"

// The size of the window SetDisplayModeWithFallback() opens when the size requested fails.
#define FALLBACK_MODE_WIDTH 640
#define FALLBACK_MODE_HEIGHT 480

// The colour buffers and the auxiliary buffers of each Glide window.
#define GLIDE_COLOR_BUFFERS 2
#define GLIDE_AUX_BUFFERS 1

// The value of the alpha test a window starts with. The render modes change it.
#define GLIDE_ALPHA_TEST_REFERENCE 0x80

int g_nGlideNumTmus;
GrHwConfiguration g_GlideHwConfig;
int g_dwGlideFbRamMB;

void PresentFrameCallback_GlideBufferSwap(void) {
    if (grBufferNumPending() > 0) {
        while (grBufferNumPending() > 0) {
        }
    }
    grBufferSwap(1);
}

// The resolution of Glide of a size, or GR_RESOLUTION_NONE for a size Glide has none for.
static inline GrScreenResolution_t GlideResolution(int nWidth, int nHeight) {
    if (nWidth == 320 && nHeight == 200) {
        return GR_RESOLUTION_320x200;
    }
    if (nWidth == 320 && nHeight == 240) {
        return GR_RESOLUTION_320x240;
    }
    if (nWidth == 400 && nHeight == 256) {
        return GR_RESOLUTION_400x256;
    }
    if (nWidth == 512 && nHeight == 384) {
        return GR_RESOLUTION_512x384;
    }
    if (nWidth == 640 && nHeight == 200) {
        return GR_RESOLUTION_640x200;
    }
    if (nWidth == 640 && nHeight == 350) {
        return GR_RESOLUTION_640x350;
    }
    if (nWidth == 640 && nHeight == 400) {
        return GR_RESOLUTION_640x400;
    }
    if (nWidth == 640 && nHeight == 480) {
        return GR_RESOLUTION_640x480;
    }
    if (nWidth == 800 && nHeight == 600) {
        return GR_RESOLUTION_800x600;
    }
    if (nWidth == 960 && nHeight == 720) {
        return GR_RESOLUTION_960x720;
    }
    if (nWidth == 856 && nHeight == 480) {
        return GR_RESOLUTION_856x480;
    }
    if (nWidth == 512 && nHeight == 256) {
        return GR_RESOLUTION_512x256;
    }
    if (nWidth == 1024 && nHeight == 768) {
        return GR_RESOLUTION_1024x768;
    }
    if (nWidth == 1280 && nHeight == 1024) {
        return GR_RESOLUTION_1280x1024;
    }
    if (nWidth == 1600 && nHeight == 1200) {
        return GR_RESOLUTION_1600x1200;
    }
    if (nWidth == 400 && nHeight == 300) {
        return GR_RESOLUTION_400x300;
    }
    return GR_RESOLUTION_NONE;
}

int OpenGlideWindowForResolution(int nWidth, int nHeight) {
    GrScreenResolution_t nResolution =
        GlideResolution(g_dwConfigFramebufferWidth, g_dwConfigFramebufferHeight);
    // A size Glide has no resolution for is not opened, and its render state is set all the same.
    if (nResolution != GR_RESOLUTION_NONE && !grSstWinOpen(0,
                                                           nResolution,
                                                           GR_REFRESH_60Hz,
                                                           GR_COLORFORMAT_RGBA,
                                                           GR_ORIGIN_LOWER_LEFT,
                                                           GLIDE_COLOR_BUFFERS,
                                                           GLIDE_AUX_BUFFERS)) {
        return 0;
    }
    InitGlideDefaultRenderState(g_dwConfigFramebufferWidth, g_dwConfigFramebufferHeight);
    return 1;
}

void InitGlideDefaultRenderState(int nWidth, int nHeight) {
    grClipWindow(0, 0, g_dwConfigFramebufferWidth, g_dwConfigFramebufferHeight);
    grDepthBufferMode(GR_DEPTHBUFFER_WBUFFER);
    grDepthBufferFunction(GR_CMP_ALWAYS);
    grDepthMask(FXTRUE);
    grBufferClear(0, 0, GR_WDEPTHVALUE_FARTHEST);
    grCullMode(GR_CULL_DISABLE);
    grAlphaCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,
                   GR_COMBINE_FACTOR_ONE,
                   GR_COMBINE_LOCAL_CONSTANT,
                   GR_COMBINE_OTHER_TEXTURE,
                   FXFALSE);
    grAlphaTestFunction(GR_CMP_ALWAYS);
    grAlphaTestReferenceValue(GLIDE_ALPHA_TEST_REFERENCE);
    grTexFilterMode(GR_TMU0, GR_TEXTUREFILTER_POINT_SAMPLED, GR_TEXTUREFILTER_POINT_SAMPLED);
    if (g_nGlideNumTmus > 1) {
        grTexFilterMode(GR_TMU1, GR_TEXTUREFILTER_POINT_SAMPLED, GR_TEXTUREFILTER_POINT_SAMPLED);
    }
    grConstantColorValue(0xffffffff);
    grColorCombine(GR_COMBINE_FUNCTION_LOCAL,
                   GR_COMBINE_FACTOR_ZERO,
                   GR_COMBINE_LOCAL_ITERATED,
                   GR_COMBINE_OTHER_CONSTANT,
                   FXFALSE);
    if (g_nGlideNumTmus > 1) {
        // The second unit samples, and the first passes its texel on.
        grTexCombine(GR_TMU1,
                     GR_COMBINE_FUNCTION_LOCAL,
                     GR_COMBINE_FACTOR_ZERO,
                     GR_COMBINE_FUNCTION_LOCAL,
                     GR_COMBINE_FACTOR_ZERO,
                     FXFALSE,
                     FXFALSE);
        grTexCombine(GR_TMU0,
                     GR_COMBINE_FUNCTION_SCALE_OTHER,
                     GR_COMBINE_FACTOR_ONE,
                     GR_COMBINE_FUNCTION_SCALE_OTHER,
                     GR_COMBINE_FACTOR_ONE,
                     FXFALSE,
                     FXFALSE);
    } else {
        grTexCombine(GR_TMU0,
                     GR_COMBINE_FUNCTION_LOCAL,
                     GR_COMBINE_FACTOR_ZERO,
                     GR_COMBINE_FUNCTION_LOCAL,
                     GR_COMBINE_FACTOR_ZERO,
                     FXFALSE,
                     FXFALSE);
    }
}

int InitGlideRenderDevice(void) {
    g_pfnPresentFrame = PresentFrameCallback_GlideBufferSwap;
    // grGlideShutdown() is __stdcall. A routine without arguments is called the same way as
    // __cdecl.
    g_pfnRenderDeviceShutdown = reinterpret_cast<void (*)(void)>(grGlideShutdown);
    grGlideInit();
    if (!grSstQueryHardware(&g_GlideHwConfig)) {
        return 0;
    }
    grSstSelect(0);
    switch (g_GlideHwConfig.SSTs[0].type) {
    case GR_SSTTYPE_VOODOO:
    case GR_SSTTYPE_Voodoo2:
        g_nGlideNumTmus = g_GlideHwConfig.SSTs[0].sstBoard.VoodooConfig.nTexelfx;
        g_dwGlideFbRamMB = g_GlideHwConfig.SSTs[0].sstBoard.VoodooConfig.fbRam;
        if (g_GlideHwConfig.SSTs[0].sstBoard.VoodooConfig.sliDetect) {
            g_dwGlideFbRamMB += g_dwGlideFbRamMB;
        }
        break;
    case GR_SSTTYPE_SST96:
        g_nGlideNumTmus = g_GlideHwConfig.SSTs[0].sstBoard.SST96Config.nTexelfx;
        g_dwGlideFbRamMB = g_GlideHwConfig.SSTs[0].sstBoard.SST96Config.fbRam;
        break;
    default:
        g_nGlideNumTmus = 1;
        g_dwGlideFbRamMB = 2;
        break;
    }
    return OpenGlideWindowForResolution(g_dwConfigFramebufferWidth, g_dwConfigFramebufferHeight);
}

// Record the size of the frame buffer of the screen and of the configuration.
static inline void SetFramebufferSize(int nWidth, int nHeight) {
    g_dwScreenWidth = nWidth;
    g_dwConfigFramebufferWidth = nWidth;
    g_dwScreenHeight = nHeight;
    g_dwConfigFramebufferHeight = nHeight;
}

int SetDisplayModeWithFallback(int nWidth, int nHeight, int nBitDepth, int nExtra) {
    if (nWidth == g_dwConfigFramebufferWidth && nHeight == g_dwConfigFramebufferHeight) {
        SetFramebufferSize(nWidth, nHeight);
        InitGlideDefaultRenderState(nWidth, nHeight);
        return 1;
    }
    grSstWinClose();
    SetFramebufferSize(nWidth, nHeight);
    if (OpenGlideWindowForResolution(nWidth, nHeight)) {
        return 1;
    }
    SetFramebufferSize(FALLBACK_MODE_WIDTH, FALLBACK_MODE_HEIGHT);
    return OpenGlideWindowForResolution(FALLBACK_MODE_WIDTH, FALLBACK_MODE_HEIGHT);
}
