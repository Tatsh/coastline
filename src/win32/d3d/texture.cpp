#include "win32/d3d/texture.h"

#include <bit>
#include <concepts>
#include <cstdlib>
#include <cstring>

#include <zlib.h>

#include "common/fileio.h"
#include "common/weather.h"
#include "win32/d3d/renderer.h"
#include "win32/render.h"

// The pixel format LoadRgba5551ImageFile() converts an image to.
#define RGBA5551_IMAGE_PIXEL_FORMAT 0xb

namespace {
// @ghidraAddress BRD3D: 0x100a6230
TexturePixelFormat g_aTexturePixelFormats[TEXTURE_PIXEL_FORMAT_COUNT] = {
    {2, 0, {}, {}}, {3, 0, {}, {}}, {4, 0, {}, {}}, {0xb, 0, {}, {}}, {0xc, 0, {}, {}}};
} // namespace

DDPIXELFORMAT *LookupPixelFormatById(int nId) {
    for (TexturePixelFormat &format : g_aTexturePixelFormats) {
        if (format.nId == nId) {
            return &format.ddpf;
        }
    }
    return nullptr;
}

int CountBitWidth(unsigned nMask) {
    int nWidth = 0;

    for (; nMask != 0; nMask >>= 1) {
        nWidth++;
    }
    return nWidth;
}

int CountTrailingZeroBits32(unsigned nMask) {
    int nCount = 0;

    // An empty mask loops for ever, as in the original.
    while (!(nMask & 1)) {
        nMask >>= 1;
        nCount++;
    }
    return nCount;
}

int PopCount32(unsigned nMask) {
    int nCount = 0;

    for (unsigned nBit = 1; nBit != 0; nBit <<= 1) {
        if (nMask & nBit) {
            nCount++;
        }
    }
    return nCount;
}

static inline int AlphaBitCount(const DDPIXELFORMAT &format) {
    return (format.dwFlags & (DDPF_ALPHAPIXELS | DDPF_ALPHA)) ?
               PopCount32(format.dwRGBAlphaBitMask) :
               0;
}

// A candidate with more alpha bits than the current format, within a limit, is better.
static inline bool HasMoreAlphaBits(const DDPIXELFORMAT &current, int nAlphaBits, int nLimit) {
    if (current.dwRGBBitCount == 0) {
        return true;
    }
    if (nAlphaBits == 0) {
        return false;
    }
    if (PopCount32(current.dwRGBAlphaBitMask) == 0) {
        return true;
    }
    return nAlphaBits > PopCount32(current.dwRGBAlphaBitMask) && nAlphaBits <= nLimit;
}

static inline bool IsBetterTextureFormat(const TexturePixelFormat &entry,
                                         const DDPIXELFORMAT &candidate) {
    const DDPIXELFORMAT &current = entry.ddpf;
    int nAlphaBits;
    int nRed, nGreen, nBlue;

    switch (entry.nId) {
    case 2:
        return HasMoreAlphaBits(current, AlphaBitCount(candidate), 8);
    case 3:
        // The alpha bits are counted and not used.
        AlphaBitCount(candidate);
        if (!(candidate.dwFlags & DDPF_RGB)) {
            return false;
        }
        nRed = PopCount32(candidate.dwRBitMask);
        nGreen = PopCount32(candidate.dwGBitMask);
        nBlue = PopCount32(candidate.dwBBitMask);
        if (current.dwRGBBitCount == 0) {
            return true;
        }
        if (nRed != nGreen || nRed != nBlue || nRed <= 0) {
            return false;
        }
        // A current format whose channels differ in width loses to one with equal channels.
        if (PopCount32(current.dwGBitMask) != PopCount32(current.dwRBitMask) ||
            PopCount32(current.dwBBitMask) != PopCount32(current.dwRBitMask)) {
            return true;
        }
        return nRed <= 8 && nRed > PopCount32(current.dwRBitMask);
    case 4:
    case 0xc:
        nAlphaBits = AlphaBitCount(candidate);
        return (candidate.dwFlags & DDPF_RGB) && HasMoreAlphaBits(current, nAlphaBits, 32);
    case 0xb:
        nAlphaBits = AlphaBitCount(candidate);
        if (!(candidate.dwFlags & DDPF_RGB)) {
            return false;
        }
        if (current.dwRGBBitCount == 0) {
            return true;
        }
        if (nAlphaBits == 0) {
            return false;
        }
        if (PopCount32(current.dwRGBAlphaBitMask) == 0) {
            return true;
        }
        return nAlphaBits < PopCount32(current.dwRGBAlphaBitMask);
    default:
        return false;
    }
}

HRESULT CALLBACK EnumTextureFormatsCallback_MatchPreset(DDSURFACEDESC *pDesc, void *pContext) {
    TexturePixelFormat *pEntry = static_cast<TexturePixelFormat *>(pContext);
    DDPIXELFORMAT candidate = pDesc->ddpfPixelFormat;

    if (candidate.dwRGBBitCount <= pEntry->wMaxBitCount &&
        IsBetterTextureFormat(*pEntry, candidate)) {
        pEntry->ddpf = candidate;
    }
    return D3DENUMRET_OK;
}

void EnumerateTextureFormats(IDirect3DDevice2 *pD3DDevice) {
    for (TexturePixelFormat &entry : g_aTexturePixelFormats) {
        memset(&entry.ddpf, 0, sizeof(entry.ddpf));
        entry.wMaxBitCount = 8;
        pD3DDevice->EnumTextureFormats(EnumTextureFormatsCallback_MatchPreset, &entry);
        entry.wMaxBitCount = 16;
        pD3DDevice->EnumTextureFormats(EnumTextureFormatsCallback_MatchPreset, &entry);
        if (entry.ddpf.dwRGBBitCount == 0) {
            entry.wMaxBitCount = 32;
            pD3DDevice->EnumTextureFormats(EnumTextureFormatsCallback_MatchPreset, &entry);
        }
    }
}

// A channel of 8 bits is moved by a shift that is negative for a right shift.
static inline unsigned ShiftChannel(unsigned char nChannel, int nShift) {
    return nShift >= 0 ? static_cast<unsigned>(nChannel) << nShift :
                         static_cast<unsigned>(nChannel) >> -nShift;
}

template <bool bColorKey>
static int ConvertRGBAToPixelFormatImpl(
    void *pDest, const void *pSrc, int nSize, int nPixelFormatId, unsigned *pColorKey) {
    const DDPIXELFORMAT *pFormat = LookupPixelFormatById(nPixelFormatId);
    unsigned char *pOut = static_cast<unsigned char *>(pDest);
    const unsigned char *pIn = static_cast<const unsigned char *>(pSrc);
    unsigned nBytesPerPixel = pFormat->dwRGBBitCount >> 3;
    int nRedShift = CountBitWidth(pFormat->dwRBitMask) - 8;
    int nGreenShift = CountBitWidth(pFormat->dwGBitMask) - 8;
    int nBlueShift = CountBitWidth(pFormat->dwBBitMask) - 8;
    int nAlphaShift = CountBitWidth(pFormat->dwRGBAlphaBitMask) - 8;
    unsigned nPixel;

    if constexpr (bColorKey) {
        *pColorKey = 1u << CountTrailingZeroBits32(pFormat->dwGBitMask);
    }
    if (nSize > 0) {
        for (unsigned nCount = (static_cast<unsigned>(nSize) + 3) >> 2; nCount != 0; nCount--) {
            if (bColorKey && pIn[3] < 0x80) {
                nPixel = *pColorKey;
            } else {
                nPixel = (ShiftChannel(pIn[0], nRedShift) & pFormat->dwRBitMask) |
                         (ShiftChannel(pIn[1], nGreenShift) & pFormat->dwGBitMask) |
                         (ShiftChannel(pIn[2], nBlueShift) & pFormat->dwBBitMask) |
                         (ShiftChannel(pIn[3], nAlphaShift) & pFormat->dwRGBAlphaBitMask);
            }
            pIn += 4;
            if (nBytesPerPixel == 1) {
                *pOut = static_cast<unsigned char>(nPixel);
                pOut += 1;
            } else if (nBytesPerPixel == 2) {
                memcpy(pOut, &nPixel, 2);
                pOut += 2;
            } else if (nBytesPerPixel == 4) {
                memcpy(pOut, &nPixel, 4);
                pOut += 4;
            }
        }
    }
    return static_cast<int>(pOut - static_cast<unsigned char *>(pDest));
}

int ConvertRGBAToPixelFormat(void *pDest, const void *pSrc, int nSize, int nPixelFormatId) {
    return ConvertRGBAToPixelFormatImpl<false>(pDest, pSrc, nSize, nPixelFormatId, nullptr);
}

int ConvertRGBAToPixelFormatWithColorKey(
    void *pDest, const void *pSrc, int nSize, int nPixelFormatId, unsigned *pColorKey) {
    return ConvertRGBAToPixelFormatImpl<true>(pDest, pSrc, nSize, nPixelFormatId, pColorKey);
}

void UnpackRgba5551ToRgba8(unsigned char *pDest, const unsigned short *pSrc, int nCount) {
    for (int i = 0; i < nCount; i++) {
        unsigned nPixel = *pSrc++;

        *pDest++ = static_cast<unsigned char>(((nPixel >> 7) & 0xf8) | ((nPixel >> 10) & 7));
        *pDest++ = static_cast<unsigned char>(((nPixel >> 2) & 0xf8) | ((nPixel >> 5) & 7));
        *pDest++ = static_cast<unsigned char>(((nPixel << 3) & 0xf8) | (nPixel & 7));
        *pDest++ = (nPixel & 0x8000) ? 0xff : 0;
    }
}

unsigned ConvertN64Rgba5551ToRgba8(unsigned nColor) {
    unsigned short pixel = std::rotr(std::byteswap(static_cast<unsigned short>(nColor)), 1);
    unsigned color;
    UnpackRgba5551ToRgba8(reinterpret_cast<unsigned char *>(&color), &pixel, 1);
    return color;
}

void *
LoadRgba5551ImageFile(const char *pszPath, unsigned nAdlerExpected, int *pnWidth, int *pnHeight) {
    GameFile *pFile;
    int nWidth;
    int nHeight;
    size_t nSize;
    unsigned short *filePixels = g_TextureDecodeWorkspace.rgba5551 + RGBA5551_IMAGE_PIXEL_COUNT;

    pFile = CHK_FReadOpen(pszPath);
    CHK_FRead(&nWidth, sizeof(nWidth), 1, pFile);
    CHK_FRead(&nHeight, sizeof(nHeight), 1, pFile);
    nSize = static_cast<size_t>(nHeight * nWidth) * sizeof(filePixels[0]);
    // The word read here is overwritten by the clear below.
    CHK_FRead(filePixels, 4, 1, pFile);
    memset(filePixels, 0, RGBA5551_IMAGE_PIXEL_COUNT * sizeof(filePixels[0]));
    CHK_FRead(filePixels, nSize, 1, pFile);
    CHK_FClose(pFile);
    if (nAdlerExpected != 0 && adler32(adler32(0, nullptr, 0),
                                       reinterpret_cast<const Bytef *>(filePixels),
                                       static_cast<uInt>(nSize)) != nAdlerExpected) {
        exit(1);
    }
    UnpackRgba5551ToRgba8(g_TextureDecodeWorkspace.decoded, filePixels, RGBA5551_IMAGE_PIXEL_COUNT);
    ConvertRGBAToPixelFormat(g_TextureDecodeWorkspace.decoded,
                             g_TextureDecodeWorkspace.decoded,
                             RGBA5551_IMAGE_PIXEL_COUNT * 4,
                             RGBA5551_IMAGE_PIXEL_FORMAT);
    *pnWidth = nWidth;
    *pnHeight = nHeight;
    return g_TextureDecodeWorkspace.decoded;
}

D3DTexture **g_ppTextureIdTable;
namespace {
// @ghidraAddress BRD3D: 0x104b2554
int g_dwPrecipTextureWidth;
// @ghidraAddress BRD3D: 0x104b2558
int g_dwPrecipTextureHeight;
} // namespace

void GetTextureEntryDimensions(int *pWidth, int *pHeight, int nEntry) {
    const TextureEntry &entry = g_pTextureEntries[nEntry];
    CalculateTextureDimensions(pWidth, pHeight, entry.data.nLod, entry.data.nAspectRatio);
}

void GetPrecipitationTextureDimensions(int nIndex) {
    GetTextureEntryDimensions(
        &g_dwPrecipTextureWidth, &g_dwPrecipTextureHeight, g_adwPrecipitationTextureIds[nIndex]);
}

void CopyTileColumnScaled(int nTileIndex, int nSrcX, int nSrcY, int nSrcRow) {
    int row = (int)((unsigned)nSrcX * (unsigned)g_dwPrecipTextureHeight) / 64;
    int column = (int)((unsigned)nSrcY * (unsigned)g_dwPrecipTextureWidth) / 64;

    for (int byte = 0; byte < g_dwAlphaRampStripRowCount; ++byte) {
        unsigned destination =
            ((unsigned)row * (unsigned)g_dwPrecipTextureWidth + (unsigned)column) *
                (unsigned)g_dwAlphaRampStripRowCount +
            (unsigned)byte;
        unsigned source = (unsigned)nSrcRow * (unsigned)g_dwAlphaRampStripRowCount + (unsigned)byte;

        g_abPrecipitationTextures[nTileIndex][(int)destination] =
            g_abAlphaRampPixelStrip[(int)source];
    }
}

void InitializeAllTextureSurfaces(void) {
    for (unsigned i = 0; i < g_dwTextureEntryCount; i++) {
        // The result of each load is discarded.
        g_ppTextureIdTable[g_pTextureEntries[i].nTextureId]->LoadSysMemTextureToDevice();
    }
}

IDirectDraw *GetDirectDrawFromD3DDevice(IDirect3DDevice2 *pDevice) {
    IDirectDrawSurface *pTarget;
    IDirectDrawSurface2 *pTarget2;
    IDirectDraw *pDirectDraw;

    if (pDevice == nullptr || pDevice->GetRenderTarget(&pTarget) != DD_OK) {
        return nullptr;
    }
    // The render target is not released when the query fails.
    if (pTarget->QueryInterface(IID_IDirectDrawSurface2, reinterpret_cast<void **>(&pTarget2)) !=
        DD_OK) {
        return nullptr;
    }
    pTarget->Release();
    pTarget2->GetDDInterface(reinterpret_cast<void **>(&pDirectDraw));
    pTarget2->Release();
    pDirectDraw->Release();
    return pDirectDraw;
}

void D3DTexture::ReleaseD3DTextureInterfaces() {
    if (pSysMemSurface != nullptr) {
        pSysMemSurface->Release();
    }
    pSysMemSurface = nullptr;
    if (pDeviceSurface != nullptr) {
        pDeviceSurface->Release();
    }
    pDeviceSurface = nullptr;
    if (pUnknown8 != nullptr) {
        pUnknown8->Release();
    }
    pUnknown8 = nullptr;
    hTexture = 0;
}

bool D3DTexture::CreateD3DTexture(IDirect3DDevice2 *pDevice,
                                  void *pImage,
                                  int nWidth,
                                  int nHeight,
                                  int nPixelFormatId,
                                  DWORD dwColorKey) {
    IDirectDraw *pDirectDraw;
    DDSURFACEDESC desc;
    D3DDEVICEDESC hardwareDesc;
    D3DDEVICEDESC softwareDesc;
    DDCOLORKEY colorKey;
    IDirect3DTexture2 *pTexture;
    bool bHardware;

    pDirectDraw = GetDirectDrawFromD3DDevice(pDevice);
    if (pDirectDraw == nullptr) {
        return false;
    }
    ReleaseD3DTextureInterfaces();
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.ddpfPixelFormat = *LookupPixelFormatById(nPixelFormatId);
    desc.dwFlags |= DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    desc.dwHeight = nHeight;
    desc.dwWidth = nWidth;
    memset(&hardwareDesc, 0, sizeof(hardwareDesc));
    memset(&softwareDesc, 0, sizeof(softwareDesc));
    hardwareDesc.dwSize = sizeof(hardwareDesc);
    softwareDesc.dwSize = sizeof(softwareDesc);
    pDevice->GetCaps(&hardwareDesc, &softwareDesc);
    bHardware = hardwareDesc.dcmColorModel != 0;

    desc.ddsCaps.dwCaps = bHardware ? DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY | DDSCAPS_ALLOCONLOAD :
                                      DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY;
    if (pDirectDraw->CreateSurface(&desc, &pDeviceSurface, nullptr) != DD_OK) {
        ReleaseD3DTextureInterfaces();
        return false;
    }
    if (bHardware) {
        desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY;
        if (pDirectDraw->CreateSurface(&desc, &pSysMemSurface, nullptr) != DD_OK) {
            ReleaseD3DTextureInterfaces();
            return false;
        }
    } else {
        pSysMemSurface = pDeviceSurface;
        pSysMemSurface->AddRef();
    }
    if (dwColorKey != 0) {
        colorKey.dwColorSpaceLowValue = dwColorKey;
        colorKey.dwColorSpaceHighValue = dwColorKey;
        pSysMemSurface->SetColorKey(DDCKEY_SRCBLT, &colorKey);
        pDeviceSurface->SetColorKey(DDCKEY_SRCBLT, &colorKey);
    }
    if (!UploadSurfaceToTexture(pImage, nWidth, nHeight)) {
        ReleaseD3DTextureInterfaces();
        return false;
    }
    // The result of the query is not checked.
    pDeviceSurface->QueryInterface(IID_IDirect3DTexture2, reinterpret_cast<void **>(&pTexture));
    pTexture->GetHandle(pDevice, &hTexture);
    pTexture->Release();
    return true;
}

bool D3DTexture::CreateD3DTextureDefault(
    IDirect3DDevice2 *pDevice, void *pImage, int nWidth, int nHeight, int nPixelFormatId) {
    return CreateD3DTexture(pDevice, pImage, nWidth, nHeight, nPixelFormatId, 0);
}

// The pixels of an image are copied row after row into the rows of a locked surface.
template <std::unsigned_integral Pixel>
    requires(sizeof(Pixel) == 1 || sizeof(Pixel) == 2 || sizeof(Pixel) == 4)
static inline void
CopyImageRows(unsigned char *pRow, const Pixel *pSrc, int nWidth, int nHeight, LONG lPitch) {
    for (int y = 0; y < nHeight; y++) {
        Pixel *pDest = reinterpret_cast<Pixel *>(pRow);

        for (int x = 0; x < nWidth; x++) {
            *pDest++ = *pSrc++;
        }
        pRow += lPitch;
    }
}

bool D3DTexture::UploadSurfaceToTexture(const void *pImage, int nWidth, int nHeight) {
    DDSURFACEDESC desc;
    RECT rect;
    unsigned nBytesPerPixel;
    unsigned char *pRow;

    if (pSysMemSurface == nullptr) {
        return false;
    }
    desc.dwSize = sizeof(desc);
    pSysMemSurface->GetSurfaceDesc(&desc);
    nBytesPerPixel = desc.ddpfPixelFormat.dwRGBBitCount >> 3;
    rect.left = 0;
    rect.top = 0;
    rect.right = nWidth;
    rect.bottom = nHeight;
    if (pSysMemSurface->Lock(&rect, &desc, DDLOCK_WAIT | DDLOCK_WRITEONLY, nullptr) != DD_OK) {
        return false;
    }
    pRow = static_cast<unsigned char *>(desc.lpSurface);
    if (nBytesPerPixel == 1) {
        CopyImageRows(
            pRow, static_cast<const unsigned char *>(pImage), nWidth, nHeight, desc.lPitch);
    } else if (nBytesPerPixel == 2) {
        CopyImageRows(
            pRow, static_cast<const unsigned short *>(pImage), nWidth, nHeight, desc.lPitch);
    } else if (nBytesPerPixel == 4) {
        CopyImageRows(pRow, static_cast<const unsigned *>(pImage), nWidth, nHeight, desc.lPitch);
    }
    if (pSysMemSurface->Unlock(desc.lpSurface) != DD_OK) {
        return false;
    }
    return LoadSysMemTextureToDevice();
}

bool D3DTexture::LoadSysMemTextureToDevice() {
    IDirect3DTexture2 *pDeviceTexture;
    IDirect3DTexture2 *pSysMemTexture;
    HRESULT hResult;

    if (pDeviceSurface == nullptr || pSysMemSurface == nullptr) {
        return false;
    }
    if (pDeviceSurface == pSysMemSurface) {
        return true;
    }
    if (pDeviceSurface->Restore() != DD_OK) {
        return false;
    }
    pDeviceSurface->QueryInterface(IID_IDirect3DTexture2,
                                   reinterpret_cast<void **>(&pDeviceTexture));
    pSysMemSurface->QueryInterface(IID_IDirect3DTexture2,
                                   reinterpret_cast<void **>(&pSysMemTexture));
    hResult = pDeviceTexture->Load(pSysMemTexture);
    pDeviceTexture->Release();
    pSysMemTexture->Release();
    return hResult == D3D_OK;
}
