#include <common/render.h>

#include "common/system.h"

// The Windows backend clears buffers outside these compatibility entry points.
void ClearDepthBuffer(void) {
}

void ClearZBufferRect(s32 x, s32 y, s32 width, s32 height) {
}

void ClearFramebuffersHighRes(void) {
}

void ClearFramebuffersLowRes(void) {
}
