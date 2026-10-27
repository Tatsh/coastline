#include "heap.h"

#if TGR_PLATFORM == TGR_PLATFORM_N64
#include <PR/os.h>
#include <PR/os_libc.h>

#include "render.h"
#include "system.h"
#else
u8 *g_pHeapCurrent;
#endif

/* The interface heap lies below the program. */
#define HEAP_START ((u8 *)0x80096400)
#define HEAP_END ((u8 *)0x800D4000)

void ResetHeap(void) {
    g_pHeapCurrent = HEAP_START;
}

#if TGR_PLATFORM == TGR_PLATFORM_N64
u8 *GetHeapCurrentAddress(void) {
    return g_pHeapCurrent;
}

void SetHeapCurrentAddress(u8 *address) {
    g_pHeapCurrent = address;
}

void *AllocHeapAligned(u32 size) {
    u8 *p = (u8 *)(((u32)g_pHeapCurrent + 7) & ~7);
    char message[120];

    g_pHeapCurrent = size + p;
    if (g_pHeapCurrent > HEAP_END) {
        sprintf(message, "Insufficient interface memory (need %d, have %d)", size, HEAP_END - p);
        SetDebugMessage(message);
    }
    return p;
}

/* Both routines make three passes with the display blanked. The first two clear the frame and the
 * third only flushes. */
void ClearFramebuffersForWideMode(void) {
    g_dwKeepScreenBlanked = TRUE;
    BLANK_SCREEN();
    StartHighResFrame();
    ClearScreen(0, 0, 0);
    FlushDisplayList();
    BLANK_SCREEN();
    StartHighResFrame();
    ClearScreen(0, 0, 0);
    FlushDisplayList();
    BLANK_SCREEN();
    StartHighResFrame();
    FlushDisplayList();
    g_dwKeepScreenBlanked = FALSE;
}

void ClearFramebuffersForNormalMode(void) {
    g_dwKeepScreenBlanked = TRUE;
    BLANK_SCREEN();
    StartLowResFrame();
    ClearScreen(0, 0, 0);
    FlushDisplayList();
    BLANK_SCREEN();
    StartLowResFrame();
    ClearScreen(0, 0, 0);
    FlushDisplayList();
    BLANK_SCREEN();
    StartLowResFrame();
    FlushDisplayList();
    g_dwKeepScreenBlanked = FALSE;
}
#endif
