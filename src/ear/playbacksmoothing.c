#include "playbacksmoothing.h"

uint16_t g_wPlaybackSmoothingPassCount = 2;
int g_earPlaybackSmoothingEnabled;

void __stdcall ConfigurePlaybackSmoothingPasses(uint32_t reduced) {
    g_wPlaybackSmoothingPassCount = reduced ? 1 : 2;
}
