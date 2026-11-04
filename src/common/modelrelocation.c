#include "common/model.h"

#if TGR_PLATFORM == TGR_PLATFORM_SDL
#define MODEL_ADDRESS(pointer) ((u32)(uintptr_t)(pointer))
#else
#define MODEL_ADDRESS(pointer) ((u32)(pointer))
#endif

void RemapPointer(u32 *pointer, const void *lo, const void *hi, const void *base) {
    if (*pointer >= MODEL_ADDRESS(lo) && *pointer < MODEL_ADDRESS(hi)) {
        *pointer = *pointer - MODEL_ADDRESS(lo) + MODEL_ADDRESS(base);
    }
}

void FixupDisplayListPointers(Gfx *displayList, const void *lo, const void *hi, const void *base) {
    if (displayList == NULL) {
        return;
    }
    /* The display lists it calls are not followed. */
    for (;; displayList++) {
        switch ((u8)(displayList->words.w0 >> 24)) {
        case G_VTX:
        case G_SETTIMG:
            RemapPointer(&displayList->words.w1, lo, hi, base);
            break;
        case G_ENDDL:
            return;
        }
    }
}
