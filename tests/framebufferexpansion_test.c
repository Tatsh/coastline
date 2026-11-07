#include <assert.h>

#include "n64/framebuffer.c"

u32 osMemSize = 0x800000;
u32 g_dwKeepScreenBlanked;

static s32 highResFrames;
static s32 lowResFrames;
static s32 clears;
static s32 flushes;

void StartHighResFrame(void) {
    assert(g_dwKeepScreenBlanked);
    highResFrames++;
}

void StartLowResFrame(void) {
    lowResFrames++;
}

void ClearScreen(s32 red, s32 green, s32 blue) {
    assert(red == 0 && green == 0 && blue == 0);
    clears++;
}

void FlushDisplayList(void) {
    flushes++;
}

int main(void) {
    ClearFramebuffersHighRes();
    assert(highResFrames == 2);
    assert(lowResFrames == 0);
    assert(clears == 2);
    assert(flushes == 2);
    assert(!g_dwKeepScreenBlanked);
    return 0;
}
