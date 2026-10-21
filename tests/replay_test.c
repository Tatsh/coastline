/* Test of the run-length coding of src/replay.c: interleaved channels survive a round trip. */
#include <assert.h>
#include <stdio.h>

#include "replay.c"

#define CHANNELS 2
#define FRAMES 600

int main(void) {
    static u8 input[CHANNELS * FRAMES + CHANNELS];
    static u8 packed[CHANNELS * FRAMES * 2];
    static u8 output[CHANNELS * FRAMES];
    s32 size;
    s32 i;

    /* Long runs, short runs, and literal stretches shorter than the 128 the coding can count. */
    for (i = 0; i < FRAMES; i++) {
        input[i * CHANNELS] = i < 300 ? 7 : (i / 7) % 2 ? 3 : 9;
        input[i * CHANNELS + 1] = (i / 5) & 1 ? 0x80 : (u8)i;
    }
    size = CompressRleChannels(packed, sizeof(packed), input, CHANNELS * FRAMES, CHANNELS);
    assert(size > 0 && size < (s32)sizeof(packed));
    assert(DecompressInterleavedRle(output, sizeof(output), packed, CHANNELS) == CHANNELS * FRAMES);
    for (i = 0; i < CHANNELS * FRAMES; i++) {
        assert(output[i] == input[i]);
    }
    /* Too small a buffer is refused. */
    assert(CompressRleChannels(packed, 8, input, CHANNELS * FRAMES, CHANNELS) == -1);

    puts("replay ok");
    return 0;
}
