#ifndef COASTLINE_AUDIOMIX_H
#define COASTLINE_AUDIOMIX_H

/**
 * @file
 * The hand-written routines of audiomix.s: copies of 64-bit words and the sample mixers.
 */

#include <PR/ultratypes.h>

/**
 * The state of the sample mixers. It begins with the write position in the stereo ring buffer, and
 * the channel records follow 8 bytes in.
 *
 * @ghidraAddress USA: 0x802a4790
 */
extern u16 g_wAudioBufferWritePos;

/**
 * The state of the reverb mixer. It begins with the mask that clears the low bit of each 16-bit
 * sample, and the channel records follow 8 bytes in.
 *
 * @ghidraAddress USA: 0x802a4918
 */
extern u64 g_qwReverbSampleMask;

/**
 * Copy 16 bytes between 8-byte aligned addresses.
 *
 * @ghidraAddress USA: 0x80256270
 */
void CopyVec4(void *dst, const void *src);

/**
 * Fill a byte count rounded up to 64 with a 16-bit value. The address must be 8-byte aligned.
 *
 * @ghidraAddress USA: 0x80256290
 */
void MemFill64Aligned(void *dst, u32 size, u16 value);

/**
 * Mix six channels into the ring buffer, replacing what it held.
 *
 * @ghidraAddress USA: 0x802562f0
 */
void MixAudioSamples(s16 *ring, u32 bytes);

/**
 * Add a pair of channels to the ring buffer.
 *
 * @param pairOffset Offset of the pair's records from those of the first extra pair.
 * @ghidraAddress USA: 0x8025649c
 */
void MixAudioStereoSamples(s16 *ring, u32 bytes, u32 pairOffset);

/**
 * Add the reverb channels to the ring buffer from an offset.
 *
 * @ghidraAddress USA: 0x8025658c
 */
void MixAudioReverbChannels(s16 *ring, u32 bytes, u32 offset);

#endif
