#pragma once

/**
 * @file
 * The helpers of DirectDraw that load bitmaps into surfaces, match colours to the pixels of a
 * surface, and set colour keys.
 */

#include <windows.h>
// The headers below need the types of windows.h.
#include <ddraw.h>

/**
 * Create an off-screen surface and copy a bitmap into it. The bitmap is a resource of the game,
 * or failing that a file.
 *
 * @param pDirectDraw The DirectDraw interface that creates the surface.
 * @param pszBitmap The name of the resource or the path of the file.
 * @param nWidth The width to load the bitmap at, or 0 for its own.
 * @param nHeight The height to load the bitmap at, or 0 for its own.
 * @param pDesc Receives the description the surface was created with. It must not be null.
 * @return The surface, or null when the bitmap does not load or the surface is not created.
 * @ghidraAddress BRD3D: 0x1000b680
 */
IDirectDrawSurface *DDLoadBitmap(
    IDirectDraw2 *pDirectDraw, LPCSTR pszBitmap, int nWidth, int nHeight, DDSURFACEDESC *pDesc);

/**
 * Copy a bitmap into a surface again, as after the surface is restored.
 *
 * @param pSurface The surface.
 * @param pszBitmap The name of the resource or the path of the file.
 * @return The result of DDCopyBitmap(), or E_FAIL when the bitmap does not load.
 * @ghidraAddress BRD3D: 0x1000b790
 */
HRESULT DDReLoadBitmap(IDirectDrawSurface *pSurface, LPCSTR pszBitmap);

/**
 * Restore a surface and stretch a part of a bitmap over all of it.
 *
 * @param pSurface The surface.
 * @param hBitmap The bitmap.
 * @param x The left of the part.
 * @param y The top of the part.
 * @param nWidth The width of the part, or 0 for the width of the bitmap.
 * @param nHeight The height of the part, or 0 for the height of the bitmap.
 * @return The result of GetDC() of the surface, or E_FAIL when either argument is null.
 * @ghidraAddress BRD3D: 0x1000b820
 */
HRESULT
DDCopyBitmap(IDirectDrawSurface *pSurface, HBITMAP hBitmap, int x, int y, int nWidth, int nHeight);

/**
 * Find the pixel value of a surface for a colour, by drawing the colour at the top left of the
 * surface and reading the pixel back.
 *
 * @param pSurface The surface.
 * @param color The colour, or CLR_INVALID to read the pixel as it is.
 * @return The pixel value, or CLR_INVALID when the surface does not lock.
 * @ghidraAddress BRD3D: 0x1000b940
 */
DWORD DDColorMatch(IDirectDrawSurface *pSurface, COLORREF color);

/**
 * Make a colour the source colour key of a surface.
 *
 * @param pSurface The surface.
 * @param color The colour.
 * @return The result of SetColorKey() of the surface.
 * @ghidraAddress BRD3D: 0x1000ba40
 */
HRESULT DDSetColorKey(IDirectDrawSurface *pSurface, COLORREF color);
