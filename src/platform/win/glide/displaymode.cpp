#include "platform/win/glide/displaymode.h"

#include <windows.h>

#include "platform/win/boot/rallyconfig.h"
#include "platform/win/frontend.h"
#include "platform/win/glide/renderer.h"

// The bits of a pixel of every mode.
#define GLIDE_MODE_BIT_DEPTH 16

const GlideResolution g_aGlideResolutions[GLIDE_RESOLUTION_COUNT] = {
    {512, 384}, {640, 400}, {640, 480}, {800, 600}, {960, 720}, {856, 480}, {1024, 768}};
GlideDisplayModeNode *g_pGlideDisplayModeList;
int g_dwGlideDisplayModeCount;

GlideDisplayModeNode::GlideDisplayModeNode()
    : nWidth(0), nHeight(0), nBitDepth(0), nRefreshRate(0), pNext(nullptr) {
}

void GlideDisplayModeNode::DeleteChain() {
    if (pNext != nullptr) {
        pNext->DeleteChain();
        delete pNext;
        pNext = nullptr;
    }
}

bool GlideDisplayModeNode::IsLessOrEqual(const GlideDisplayModeNode *pMode) const {
    return pMode != nullptr && pNext != nullptr && pMode->nBitDepth <= pNext->nBitDepth &&
           pMode->nWidth <= pNext->nWidth && pMode->nHeight <= pNext->nHeight;
}

void GlideDisplayModeNode::InsertSorted(GlideDisplayModeNode *pMode) {
    GlideDisplayModeNode *pAt = this;

    while (!pAt->IsLessOrEqual(pMode)) {
        if (pAt->pNext == nullptr) {
            pAt->pNext = pMode;
            return;
        }
        pAt = pAt->pNext;
    }
    pMode->pNext = pAt->pNext;
    pAt->pNext = pMode;
}

void BuildGlideDisplayModeList(void) {
    if (g_pGlideDisplayModeList != nullptr) {
        g_pGlideDisplayModeList->DeleteChain();
        delete g_pGlideDisplayModeList;
        g_pGlideDisplayModeList = nullptr;
        g_dwGlideDisplayModeCount = 0;
    }
    for (const GlideResolution &resolution : g_aGlideResolutions) {
        // Two colour buffers and a depth buffer of two bytes a pixel.
        if (resolution.nHeight * resolution.nWidth * 6 > g_dwGlideFbRamMB << 20) {
            continue;
        }
        // The new of the original returns NULL when memory runs out, and the mode is written
        // through it.
        GlideDisplayModeNode *pMode = new GlideDisplayModeNode;
        pMode->nWidth = resolution.nWidth;
        pMode->nHeight = resolution.nHeight;
        pMode->nBitDepth = GLIDE_MODE_BIT_DEPTH;
        pMode->nRefreshRate = 0;
        if (g_pGlideDisplayModeList == nullptr) {
            g_pGlideDisplayModeList = pMode;
        } else {
            g_pGlideDisplayModeList->InsertSorted(pMode);
        }
        g_dwGlideDisplayModeCount++;
    }
}

int PopulateDisplayModeList(void) {
    BuildGlideDisplayModeList();
    if (g_dwGlideDisplayModeCount == 0) {
        return 0;
    }
    int index = 0;
    bool selected = false;
    for (GlideDisplayModeNode *pMode = g_pGlideDisplayModeList; pMode != nullptr;
         pMode = pMode->pNext, index++) {
        int width = pMode->nWidth;
        int height = pMode->nHeight;
        int depth = pMode->nBitDepth;
        int refresh = pMode->nRefreshRate;
        char text[80];

        if (refresh) {
            wsprintfA(text, "%4d x %4d x %2d (%4d Hz)", width, height, depth, refresh);
        } else {
            wsprintfA(text, "%4d x %4d x %2d", width, height, depth);
        }
        if (g_GlobalRallyConfig.dwDisplayModeWidth == width &&
            g_GlobalRallyConfig.dwDisplayModeHeight == height &&
            g_GlobalRallyConfig.dwDisplayModeBitDepth == depth &&
            (!refresh || g_GlobalRallyConfig.dwDisplayModeExtra == refresh)) {
            selected = true;
            g_dwActiveDisplayModeIndex = index;
            g_dwSelectedDisplayModeIndex = index;
        }
        if (!selected && width == 640 && height == 480 && depth == GLIDE_MODE_BIT_DEPTH) {
            g_dwActiveDisplayModeIndex = index;
            g_dwSelectedDisplayModeIndex = index;
        }
        if (g_pDisplayModeSelectMenu != nullptr) {
            g_pDisplayModeSelectMenu->GetItemList().AddItem(
                text, 0, 1, &g_rcMenuDeviceListBox, true);
            g_pDisplayModeSelectMenu->GetItemList().SetItemData(pMode, sizeof(*pMode), index);
        }
    }
    return 1;
}

int DisplayModeSelectUI::OnSetMode(MenuWidget *pWidget) {
    const GlideDisplayModeNode *pMode = static_cast<const GlideDisplayModeNode *>(
        g_pDisplayModeSelectMenu->GetItemList().GetItemData(g_dwSelectedDisplayModeIndex));

    g_dwActiveDisplayModeIndex = g_dwSelectedDisplayModeIndex;
    g_GlobalRallyConfig.dwDisplayModeWidth = pMode->nWidth;
    g_GlobalRallyConfig.dwDisplayModeHeight = pMode->nHeight;
    g_GlobalRallyConfig.dwDisplayModeBitDepth = pMode->nBitDepth;
    g_GlobalRallyConfig.dwDisplayModeExtra = pMode->nRefreshRate;
    ReturnToVideoOptionsFromResolution(pWidget);
    return 0;
}
