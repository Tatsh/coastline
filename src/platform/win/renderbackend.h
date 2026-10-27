#ifndef COASTLINE_WIN_RENDERBACKEND_H
#define COASTLINE_WIN_RENDERBACKEND_H

struct GbiCommand;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ghidraAddress BRD3D: 0x10b501d0
 */
extern void (*g_pfnRenderBackendSubmitDisplayList)(struct GbiCommand *pDisplayList);

#ifdef __cplusplus
}
#endif

#endif
