#include "shadowbuffers.h"
#include "util.h"

void ResetShadowBuffers(void) {
    g_pShadowDisplayListWritePtr = g_pShadowDisplayListStart =
        g_aShadowDisplayLists[g_dwFrameBufferIndex];
    g_pShadowVertexWritePtr = g_pShadowVertexStart = g_aShadowVertices[g_dwFrameBufferIndex];
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    g_pTrackQuadVertexWritePtr = g_pTrackQuadVertexStart =
        g_aTrackQuadVertices[g_dwFrameBufferIndex];
#endif
}
