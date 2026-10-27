#include "frameclear.h"

void ClearPerFrameVariables(void) {
    s32 i;
    u8 *byte;
    u8 *end;

    for (i = 0; g_aPerFrameVariables[i].start != NULL; i++) {
        byte = g_aPerFrameVariables[i].start;
        end = byte + g_aPerFrameVariables[i].size;
        while (byte < end) {
            *byte++ = 0;
        }
    }
}
