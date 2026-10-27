#ifndef COASTLINE_SHADOWBUFFERS_H
#define COASTLINE_SHADOWBUFFERS_H

#include "displaylistops.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include "convertedvertex.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The next command of the shadows, and their first this frame.
 *
 * @ghidraAddress USA: 0x8028c75c
 * @ghidraAddress EUR: 0x8028eebc
 * @ghidraAddress JPN: 0x8028e90c
 * @ghidraAddress BRD3D: 0x10363ff0
 */
extern DisplayListCommand *g_pShadowDisplayListWritePtr;

/**
 * @ghidraAddress USA: 0x8028c760
 * @ghidraAddress EUR: 0x8028eec0
 * @ghidraAddress JPN: 0x8028e910
 * @ghidraAddress BRD3D: 0x102e5ec8
 */
extern DisplayListCommand *g_pShadowDisplayListStart;

/**
 * The next vertex of the shadows, and their first this frame.
 *
 * @ghidraAddress USA: 0x8028c764
 * @ghidraAddress EUR: 0x8028eec4
 * @ghidraAddress JPN: 0x8028e914
 * @ghidraAddress BRD3D: 0x10364304
 */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
extern ConvertedModelVertex *g_pShadowVertexWritePtr;
#else
extern Vtx *g_pShadowVertexWritePtr;
#endif

/**
 * @ghidraAddress USA: 0x8028c768
 * @ghidraAddress EUR: 0x8028eec8
 * @ghidraAddress JPN: 0x8028e918
 * @ghidraAddress BRD3D: 0x103643bc
 */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
extern ConvertedModelVertex *g_pShadowVertexStart;
#else
extern Vtx *g_pShadowVertexStart;
#endif

#ifdef __cplusplus
}
#endif

#endif
