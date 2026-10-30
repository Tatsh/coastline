#include "common/renderbackend.h"

#include <cstring>

#include "common/shadowclip.h"
#include "common/trackskidmarks.h"

void NullStub2(void *pMemory) {
}

void ResetRenderCommandBufferStack(void) {
    ShadowClipVertex *pNext = nullptr;

    for (int i = SHADOW_CLIP_VERTEX_COUNT - 1; i >= 0; i--) {
        g_aShadowClipVertexPool[i].pNext = pNext;
        pNext = &g_aShadowClipVertexPool[i];
    }
    g_pShadowClipVertexFreeList = pNext;
}

void ClearSplitScreenRenderState(void) {
    memset(g_anSkidMarkHead, 0, sizeof(g_anSkidMarkHead));
    memset(g_anSkidMarkTail, 0, sizeof(g_anSkidMarkTail));
}
