#include "model.h"

void RemapPointer(u32 *pointer, u32 lo, u32 hi, u32 base) {
    if (*pointer >= lo && *pointer < hi) {
        *pointer = *pointer - lo + base;
    }
}

void FixupDisplayListPointers(Gfx *displayList, u32 lo, u32 hi, u32 base) {
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
