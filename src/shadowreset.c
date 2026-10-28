#include "shadowbuffers.h"
#include "util.h"

#if TGR_PC
/** @ghidraAddress BRD3D: 0x102e5ec8 */
DisplayListCommand *g_pShadowDisplayListStart;
/** @ghidraAddress BRD3D: 0x10363ff0 */
DisplayListCommand *g_pShadowDisplayListWritePtr;
/** @ghidraAddress BRD3D: 0x103643bc */
ConvertedModelVertex *g_pShadowVertexStart;
/** @ghidraAddress BRD3D: 0x10364304 */
ConvertedModelVertex *g_pShadowVertexWritePtr;
/** @ghidraAddress BRD3D: 0x102e5ec4 */
ConvertedModelVertex *g_pTrackQuadVertexStart;
/** @ghidraAddress BRD3D: 0x10363ff4 */
ConvertedModelVertex *g_pTrackQuadVertexWritePtr;
#endif /* TGR_PC */

void ResetShadowBuffers(void) {
    g_pShadowDisplayListWritePtr = g_pShadowDisplayListStart =
        g_aShadowDisplayLists[g_dwFrameBufferIndex];
    g_pShadowVertexWritePtr = g_pShadowVertexStart = g_aShadowVertices[g_dwFrameBufferIndex];
#if TGR_PC
    g_pTrackQuadVertexWritePtr = g_pTrackQuadVertexStart =
        g_aTrackQuadVertices[g_dwFrameBufferIndex];
#endif
}
