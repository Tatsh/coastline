#pragma once

/**
 * @file
 * The part of Direct3D 5 that the display list interpreter of BRD3D.dll calls, with the values of
 * the Direct3D headers, drawn by the SDL GPU renderer.
 */

#include <cstdint>
#include <filesystem>

#include "sdl/tlrenderer.h"
#include "sdl/win32types.h"

#define STDMETHODCALLTYPE

typedef std::uint32_t D3DCOLOR;          /*!< A colour, 0xAARRGGBB. */
typedef coastline::TlVertex D3DTLVERTEX; /*!< A transformed and lit vertex. */
typedef DWORD D3DTEXTUREHANDLE;          /*!< A texture of the device, or 0. */

/** The render states, with their values in d3dtypes.h. */
enum D3DRENDERSTATETYPE {
    D3DRENDERSTATE_TEXTUREHANDLE = 1,      /*!< The texture. */
    D3DRENDERSTATE_TEXTUREPERSPECTIVE = 4, /*!< Perspective correction, always on here. */
    D3DRENDERSTATE_WRAPU = 5,              /*!< Cylindrical wrapping of u. */
    D3DRENDERSTATE_WRAPV = 6,              /*!< Cylindrical wrapping of v. */
    D3DRENDERSTATE_ZENABLE = 7,            /*!< The depth test. */
    D3DRENDERSTATE_SHADEMODE = 9,          /*!< Flat or Gouraud shading. */
    D3DRENDERSTATE_ZWRITEENABLE = 14,      /*!< Depth writes. */
    D3DRENDERSTATE_ALPHATESTENABLE = 15,   /*!< The alpha test. */
    D3DRENDERSTATE_TEXTUREMAG = 17,        /*!< The magnification filter. */
    D3DRENDERSTATE_TEXTUREMIN = 18,        /*!< The minification filter. */
    D3DRENDERSTATE_SRCBLEND = 19,          /*!< The blend factor of the source. */
    D3DRENDERSTATE_DESTBLEND = 20,         /*!< The blend factor of the destination. */
    D3DRENDERSTATE_CULLMODE = 22,          /*!< The culled winding. */
    D3DRENDERSTATE_ZFUNC = 23,             /*!< The depth comparison. */
    D3DRENDERSTATE_ALPHAREF = 24,          /*!< The reference of the alpha test. */
    D3DRENDERSTATE_ALPHAFUNC = 25,         /*!< The comparison of the alpha test. */
    D3DRENDERSTATE_DITHERENABLE = 26,      /*!< Dithering, which the renderer does not do. */
    D3DRENDERSTATE_ALPHABLENDENABLE = 27,  /*!< Blending. */
    D3DRENDERSTATE_FOGENABLE = 28,         /*!< Fog. */
    D3DRENDERSTATE_SPECULARENABLE = 29,    /*!< The specular colour. */
    D3DRENDERSTATE_FOGCOLOR = 34,          /*!< The colour of the fog. */
    D3DRENDERSTATE_FOGTABLEMODE = 35,      /*!< Table fog, 0 for vertex fog. */
    D3DRENDERSTATE_COLORKEYENABLE = 41,    /*!< The colour key of textures. */
    D3DRENDERSTATE_TEXTUREADDRESSU = 44,   /*!< The addressing of u. */
    D3DRENDERSTATE_TEXTUREADDRESSV = 45,   /*!< The addressing of v. */
};

/** The comparison functions. */
enum D3DCMPFUNC {
    D3DCMP_NEVER = 1,        /*!< Never passes. */
    D3DCMP_LESS = 2,         /*!< Passes below the reference. */
    D3DCMP_EQUAL = 3,        /*!< Passes at the reference. */
    D3DCMP_LESSEQUAL = 4,    /*!< Passes at or below the reference. */
    D3DCMP_GREATER = 5,      /*!< Passes above the reference. */
    D3DCMP_NOTEQUAL = 6,     /*!< Passes away from the reference. */
    D3DCMP_GREATEREQUAL = 7, /*!< Passes at or above the reference. */
    D3DCMP_ALWAYS = 8,       /*!< Always passes. */
};

/** The blend factors. */
enum D3DBLEND {
    D3DBLEND_ZERO = 1,          /*!< 0. */
    D3DBLEND_ONE = 2,           /*!< 1. */
    D3DBLEND_SRCCOLOR = 3,      /*!< The colour of the source. */
    D3DBLEND_INVSRCCOLOR = 4,   /*!< 1 less the colour of the source. */
    D3DBLEND_SRCALPHA = 5,      /*!< The alpha of the source. */
    D3DBLEND_INVSRCALPHA = 6,   /*!< 1 less the alpha of the source. */
    D3DBLEND_DESTALPHA = 7,     /*!< The alpha of the destination. */
    D3DBLEND_INVDESTALPHA = 8,  /*!< 1 less the alpha of the destination. */
    D3DBLEND_DESTCOLOR = 9,     /*!< The colour of the destination. */
    D3DBLEND_INVDESTCOLOR = 10, /*!< 1 less the colour of the destination. */
};

/** The culled windings. */
enum D3DCULL {
    D3DCULL_NONE = 1, /*!< Nothing is culled. */
    D3DCULL_CW = 2,   /*!< Clockwise triangles are culled. */
    D3DCULL_CCW = 3,  /*!< Counter-clockwise triangles are culled. */
};

/** The texture filters. */
enum D3DTEXTUREFILTER {
    D3DFILTER_NEAREST = 1, /*!< The nearest texel. */
    D3DFILTER_LINEAR = 2,  /*!< A bilinear filter. */
};

/** The texture addressing modes. */
enum D3DTEXTUREADDRESS {
    D3DTADDRESS_WRAP = 1,   /*!< The texture repeats. */
    D3DTADDRESS_MIRROR = 2, /*!< The texture repeats mirrored. */
    D3DTADDRESS_CLAMP = 3,  /*!< The edge texels extend. */
};

/** The shading modes. */
enum D3DSHADEMODE {
    D3DSHADE_FLAT = 1,    /*!< One colour a triangle. */
    D3DSHADE_GOURAUD = 2, /*!< Colours interpolated across a triangle. */
};

/** The primitive types. */
enum D3DPRIMITIVETYPE {
    D3DPT_TRIANGLELIST = 4, /*!< Independent triangles. */
};

/** The vertex types. */
enum D3DVERTEXTYPE {
    D3DVT_TLVERTEX = 3, /*!< D3DTLVERTEX. */
};

#define D3DDP_WAIT 0x00000001                    /*!< Wait for the device. */
#define D3DDP_DONOTCLIP 0x00000004               /*!< The vertices need no clipping. */
#define D3DDP_DONOTUPDATEEXTENTS 0x00000008      /*!< The extents are not updated. */
#define D3D_OK ((HRESULT)0)                      /*!< Success. */
#define DDERR_SURFACELOST ((HRESULT)0x887601C2)  /*!< The surface must be restored. */
#define D3DCLIPSTATUS_EXTENTS2 0x00000002        /*!< The extents of the clip status are valid. */
#define D3DSTATUS_CLIPINTERSECTIONALL 0x00000FFF /*!< Every clip plane. */
#define D3DCLEAR_TARGET 0x00000001               /*!< Clear the render target. */
#define D3DCLEAR_ZBUFFER 0x00000002              /*!< Clear the depth buffer. */
#define D3DPTEXTURECAPS_SQUAREONLY 0x00000020    /*!< Textures must be square. */
#define DDSCAPS_TEXTURE 0x00001000               /*!< A surface for a texture. */

/** The clip status of the device. */
struct D3DCLIPSTATUS {
    DWORD dwFlags;  /*!< The D3DCLIPSTATUS_ flags. */
    DWORD dwStatus; /*!< The clip planes. */
    float minx;     /*!< The least x. */
    float maxx;     /*!< The greatest x. */
    float miny;     /*!< The least y. */
    float maxy;     /*!< The greatest y. */
    float minz;     /*!< The least z. */
    float maxz;     /*!< The greatest z. */
};

/** A rectangle of the viewport. */
struct D3DRECT {
    LONG x1; /*!< The left edge. */
    LONG y1; /*!< The top edge. */
    LONG x2; /*!< The right edge. */
    LONG y2; /*!< The bottom edge. */
};

/** The capabilities of a surface. */
struct DDSCAPS {
    DWORD dwCaps; /*!< The DDSCAPS_ flags. */
};

/** The capabilities of a device for primitives. */
struct D3DPRIMCAPS {
    DWORD dwTextureCaps; /*!< The D3DPTEXTURECAPS_ flags. */
};

/** The capabilities of a device. */
struct D3DDEVICEDESC {
    D3DPRIMCAPS dpcTriCaps;   /*!< The capabilities for triangles. */
    DWORD dwMaxTextureWidth;  /*!< The widest texture, or 0 for no limit. */
    DWORD dwMaxTextureHeight; /*!< The tallest texture, or 0 for no limit. */
};

/**
 * The hardware device. The GPUs SDL drives take textures of any size the game makes, of any
 * aspect.
 *
 * @ghidraAddress BRD3D: 0x10277480
 */
extern D3DDEVICEDESC g_D3DHalDeviceDesc;

/**
 * The emulated device.
 *
 * @ghidraAddress BRD3D: 0x10277580
 */
extern D3DDEVICEDESC g_D3DHelDeviceDesc;

/**
 * The device of Direct3D 5 for the calls of the interpreter. Render states become states of the
 * renderer, and the scene draws when it ends.
 */
class IDirect3DDevice2 {
public:
    /**
     * @param renderer The renderer the device draws with.
     */
    explicit IDirect3DDevice2(coastline::TlRenderer &renderer) : m_renderer(renderer) {
    }

    /**
     * Set a render state.
     *
     * @param state The state.
     * @param value The value, in the units of Direct3D.
     * @return D3D_OK.
     */
    HRESULT SetRenderState(D3DRENDERSTATETYPE state, DWORD value);

    /**
     * Draw a triangle list.
     *
     * @param type D3DPT_TRIANGLELIST.
     * @param vertexType D3DVT_TLVERTEX.
     * @param vertices The vertices.
     * @param count The vertices, three for each triangle.
     * @param flags The D3DDP_ flags, which do not change the drawing.
     * @return D3D_OK.
     */
    HRESULT DrawPrimitive(
        D3DPRIMITIVETYPE type, D3DVERTEXTYPE vertexType, void *vertices, DWORD count, DWORD flags);

    /**
     * Draw an indexed triangle list.
     *
     * @param type D3DPT_TRIANGLELIST.
     * @param vertexType D3DVT_TLVERTEX.
     * @param vertices The vertices.
     * @param vertexCount The vertices.
     * @param indices Three indices for each triangle.
     * @param indexCount The indices.
     * @param flags The D3DDP_ flags, which do not change the drawing.
     * @return D3D_OK.
     */
    HRESULT DrawIndexedPrimitive(D3DPRIMITIVETYPE type,
                                 D3DVERTEXTYPE vertexType,
                                 void *vertices,
                                 DWORD vertexCount,
                                 WORD *indices,
                                 DWORD indexCount,
                                 DWORD flags);

    /**
     * Record the clip status, which the renderer does not use.
     *
     * @param status The clip status.
     * @return D3D_OK.
     */
    HRESULT SetClipStatus(D3DCLIPSTATUS *status);

    /** @return D3D_OK, with a new scene started. */
    HRESULT BeginScene();

    /** @return D3D_OK, with the scene drawn and presented. */
    HRESULT EndScene();

    /** @return The renderer of the device. */
    coastline::TlRenderer &Renderer() {
        return m_renderer;
    }

    /**
     * Save the frame of the scenes that end after this call as a PNG image.
     *
     * @param path The image, or null for no image.
     */
    void SetCapture(const std::filesystem::path *path) {
        m_capture = path;
    }

private:
    coastline::TlRenderer &m_renderer;
    const std::filesystem::path *m_capture = nullptr;
    std::uint32_t m_clearColor = 0xFF000000;
};

/** The device of the game, created with the window. */
extern IDirect3DDevice2 *g_pD3DDevice2;

/**
 * A texture of the device. The game gives it pixels of 32 bits, R, G, B, and A, and a texture
 * with a colour key draws nothing where the alpha of a pixel is below half.
 */
class D3DTexture {
public:
    /**
     * Create the texture with a colour key.
     *
     * @param device The device.
     * @param pixels The pixels, R, G, B, A.
     * @param width The width.
     * @param height The height.
     * @param pixelFormat The pixel format the PC release chose; the renderer stores 32 bits.
     * @param colorKey The colour key, which is not 0.
     * @return Whether the texture was created.
     */
    bool CreateD3DTexture(IDirect3DDevice2 *device,
                          const unsigned char *pixels,
                          int width,
                          int height,
                          int pixelFormat,
                          unsigned colorKey);

    /**
     * Create the texture without a colour key.
     *
     * @param device The device.
     * @param pixels The pixels, R, G, B, A.
     * @param width The width.
     * @param height The height.
     * @param pixelFormat The pixel format the PC release chose; the renderer stores 32 bits.
     * @return Whether the texture was created.
     */
    bool CreateD3DTextureDefault(IDirect3DDevice2 *device,
                                 const unsigned char *pixels,
                                 int width,
                                 int height,
                                 int pixelFormat);

    /**
     * Replace the pixels of the texture.
     *
     * @param pixels The pixels, R, G, B, A.
     * @param width The width.
     * @param height The height.
     */
    void UploadSurfaceToTexture(const void *pixels, int width, int height);

    /** @return The texture of the renderer. */
    D3DTEXTUREHANDLE GetTextureHandle() const {
        return m_handle;
    }

    /** Release the texture, which the renderer retains until it ends. */
    void ReleaseD3DTextureInterfaces() {
        m_handle = 0;
    }

private:
    D3DTEXTUREHANDLE m_handle = 0;
    bool m_colorKey = false;
};

/**
 * The textures of the device by identifier.
 *
 * @ghidraAddress BRD3D: 0x1057542c
 */
extern D3DTexture **g_ppTextureIdTable;
