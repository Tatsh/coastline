#include "platform/win/ghost.h"

void SetGhostFrameCountFromSize(unsigned dwSize) {
    g_adwGhostFrameCount[1] = dwSize / GHOST_FRAME_SIZE;
}
