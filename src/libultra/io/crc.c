#include "controller.h"

u8 __osContAddressCrc(u16 addr) {
    u8 temp = 0;
    u8 temp2;
    int i;

    /* The generator polynomial is x^5 + x^4 + x^2 + 1, fed with the 11 address bits. */
    for (i = 0; i < 16; i++) {
        temp2 = (temp & 0x10) ? 0x15 : 0;
        temp <<= 1;
        temp |= (addr & 0x400) ? 1 : 0;
        addr <<= 1;
        temp ^= temp2;
    }
    return temp & 0x1F;
}

u8 __osContDataCrc(u8 *data) {
    u8 temp = 0;
    u8 temp2;
    int i, j;

    /* The generator polynomial is x^8 + x^7 + x^2 + 1. The pass after the last byte shifts in
     * zeroes. */
    for (i = 0; i <= BLOCKSIZE; i++, data++) {
        for (j = 7; j >= 0; j--) {
            temp2 = (temp & 0x80) ? 0x85 : 0;
            temp <<= 1;
            if (i == BLOCKSIZE) {
                /* The original masks the shifted value to a byte, which it already is. */
                temp &= 0xFF;
            } else {
                temp |= (*data & (1 << j)) ? 1 : 0;
            }
            temp ^= temp2;
        }
    }
    return temp;
}
