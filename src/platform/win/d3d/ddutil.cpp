#include "platform/win/d3d/ddutil.h"

#include <cstring>

// Load a bitmap as a section of the game, or failing that from a file.
static inline HBITMAP LoadBitmapImage(LPCSTR pszBitmap, int nWidth, int nHeight) {
    HBITMAP hBitmap = static_cast<HBITMAP>(LoadImageA(
        GetModuleHandleA(nullptr), pszBitmap, IMAGE_BITMAP, nWidth, nHeight, LR_CREATEDIBSECTION));

    if (hBitmap == nullptr) {
        hBitmap = static_cast<HBITMAP>(LoadImageA(nullptr,
                                                  pszBitmap,
                                                  IMAGE_BITMAP,
                                                  nWidth,
                                                  nHeight,
                                                  LR_LOADFROMFILE | LR_CREATEDIBSECTION));
    }
    return hBitmap;
}

IDirectDrawSurface *DDLoadBitmap(
    IDirectDraw2 *pDirectDraw, LPCSTR pszBitmap, int nWidth, int nHeight, DDSURFACEDESC *pDesc) {
    HBITMAP hBitmap;
    BITMAP bm;
    DDSURFACEDESC desc;
    IDirectDrawSurface *pSurface;

    hBitmap = LoadBitmapImage(pszBitmap, nWidth, nHeight);
    if (hBitmap == nullptr) {
        return nullptr;
    }
    GetObjectA(hBitmap, sizeof(bm), &bm);
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    desc.dwWidth = bm.bmWidth;
    desc.dwHeight = bm.bmHeight;
    if (pDirectDraw->CreateSurface(&desc, &pSurface, nullptr) != DD_OK) {
        // The bitmap is not deleted here, as in the original.
        return nullptr;
    }
    *pDesc = desc;
    DDCopyBitmap(pSurface, hBitmap, 0, 0, 0, 0);
    DeleteObject(hBitmap);
    return pSurface;
}

HRESULT DDReLoadBitmap(IDirectDrawSurface *pSurface, LPCSTR pszBitmap) {
    HBITMAP hBitmap;
    HRESULT hr;

    hBitmap = LoadBitmapImage(pszBitmap, 0, 0);
    if (hBitmap == nullptr) {
        OutputDebugStringA("handle is null\n");
        return E_FAIL;
    }
    hr = DDCopyBitmap(pSurface, hBitmap, 0, 0, 0, 0);
    if (hr != DD_OK) {
        OutputDebugStringA("ddcopybitmap failed\n");
    }
    DeleteObject(hBitmap);
    return hr;
}

HRESULT
DDCopyBitmap(IDirectDrawSurface *pSurface, HBITMAP hBitmap, int x, int y, int nWidth, int nHeight) {
    HDC hdcImage;
    HDC hdc;
    BITMAP bm;
    DDSURFACEDESC desc;
    HRESULT hr;

    if (hBitmap == nullptr || pSurface == nullptr) {
        return E_FAIL;
    }
    pSurface->Restore();
    hdcImage = CreateCompatibleDC(nullptr);
    // The bitmap is selected into the context even when it is not created.
    if (hdcImage == nullptr) {
        OutputDebugStringA("createcompatible dc failed\n");
    }
    SelectObject(hdcImage, hBitmap);
    GetObjectA(hBitmap, sizeof(bm), &bm);
    if (nWidth == 0) {
        nWidth = bm.bmWidth;
    }
    if (nHeight == 0) {
        nHeight = bm.bmHeight;
    }
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_HEIGHT | DDSD_WIDTH;
    pSurface->GetSurfaceDesc(&desc);
    hr = pSurface->GetDC(&hdc);
    if (hr == DD_OK) {
        StretchBlt(
            hdc, 0, 0, desc.dwWidth, desc.dwHeight, hdcImage, x, y, nWidth, nHeight, SRCCOPY);
        pSurface->ReleaseDC(hdc);
    }
    DeleteDC(hdcImage);
    return hr;
}

DWORD DDColorMatch(IDirectDrawSurface *pSurface, COLORREF color) {
    COLORREF savedColor;
    HDC hdc;
    DWORD dwPixel = CLR_INVALID;
    DDSURFACEDESC desc;
    HRESULT hr;

    if (color != CLR_INVALID && pSurface->GetDC(&hdc) == DD_OK) {
        savedColor = GetPixel(hdc, 0, 0);
        SetPixel(hdc, 0, 0, color);
        pSurface->ReleaseDC(hdc);
    }
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    while ((hr = pSurface->Lock(nullptr, &desc, 0, nullptr)) == DDERR_WASSTILLDRAWING) {
    }
    if (hr == DD_OK) {
        dwPixel = *static_cast<DWORD *>(desc.lpSurface);
        if (desc.ddpfPixelFormat.dwRGBBitCount < 32) {
            dwPixel &= (1u << desc.ddpfPixelFormat.dwRGBBitCount) - 1;
        }
        pSurface->Unlock(nullptr);
    }
    // The saved colour is read uninitialised when the first GetDC() fails, as in the original.
    if (color != CLR_INVALID && pSurface->GetDC(&hdc) == DD_OK) {
        SetPixel(hdc, 0, 0, savedColor);
        pSurface->ReleaseDC(hdc);
    }
    return dwPixel;
}

HRESULT DDSetColorKey(IDirectDrawSurface *pSurface, COLORREF color) {
    DDCOLORKEY key;

    key.dwColorSpaceLowValue = DDColorMatch(pSurface, color);
    key.dwColorSpaceHighValue = key.dwColorSpaceLowValue;
    return pSurface->SetColorKey(DDCKEY_SRCBLT, &key);
}
