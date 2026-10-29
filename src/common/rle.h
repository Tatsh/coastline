#ifndef COASTLINE_RLE_H
#define COASTLINE_RLE_H

#include "common/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Compress interleaved data with run lengths, each channel on its own.
 *
 * @param dst The buffer to write.
 * @param dstSize The size of dst, in bytes.
 * @param src The interleaved data.
 * @param srcSize The size of src, in bytes.
 * @param channels The number of interleaved channels.
 * @return The size of the compressed data, or -1 when it does not fit in dst.
 * @ghidraAddress USA: 0x8021c878
 * @ghidraAddress EUR: 0x8021da78
 * @ghidraAddress JPN: 0x8021d348
 * @ghidraAddress BRD3D: 0x10034cc5
 * @ghidraAddress BRGlide: 0x1002e376
 */
s32 CompressRleChannels(u8 *dst, s32 dstSize, u8 *src, s32 srcSize, s32 channels);

/**
 * Expand the data CompressRleChannels() wrote, interleaving its channels again.
 *
 * @param dst The buffer to write.
 * @param dstSize The size of dst, which is not checked.
 * @param src The compressed data.
 * @param channels The number of interleaved channels.
 * @return The final channel's expanded byte count multiplied by the number of channels.
 * @ghidraAddress USA: 0x8021cb4c
 * @ghidraAddress EUR: 0x8021dd4c
 * @ghidraAddress JPN: 0x8021d61c
 * @ghidraAddress BRD3D: 0x10034f08
 * @ghidraAddress BRGlide: 0x1002e5b9
 */
s32 DecompressInterleavedRle(u8 *dst, s32 dstSize, u8 *src, s32 channels);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_RLE_H */
