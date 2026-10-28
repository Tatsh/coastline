#include "heap.h"

#if !TGR_PC
#include <PR/os.h>
#include <PR/os_libc.h>

#include "render.h"
#include "system.h"
#else
u8 *g_pHeapCurrent;
#endif

#if TGR_PLATFORM == TGR_PLATFORM_SDL && !TGR_PC
/* The interface heap of the front end of the SDL game, of the size the console gives it. */
alignas(8) static u8 s_abInterfaceHeap[0x800D4000 - 0x80096400];
#define HEAP_START (&s_abInterfaceHeap[0])
#define HEAP_END (&s_abInterfaceHeap[sizeof(s_abInterfaceHeap)])
#else
/* The interface heap lies below the program. */
#define HEAP_START ((u8 *)0x80096400)
#define HEAP_END ((u8 *)0x800D4000)
#endif

void ResetHeap(void) {
    g_pHeapCurrent = HEAP_START;
}

#if !TGR_PC
u8 *GetHeapCurrentAddress(void) {
    return g_pHeapCurrent;
}

void SetHeapCurrentAddress(u8 *address) {
    g_pHeapCurrent = address;
}

void *AllocHeapAligned(u32 size) {
#if TGR_PLATFORM == TGR_PLATFORM_SDL
    u8 *p = (u8 *)(((uintptr_t)g_pHeapCurrent + 7) & ~(uintptr_t)7);
#else
    u8 *p = (u8 *)(((u32)g_pHeapCurrent + 7) & ~7);
#endif
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
#endif /* !TGR_PC */
