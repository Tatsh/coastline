#ifndef COASTLINE_SOUNDCHANNEL_H
#define COASTLINE_SOUNDCHANNEL_H

/**
 * @file
 * The start of a sample on a sound effect channel.
 */

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

#if TGR_PC
/**
 * Play a sample on a sound effect channel at its own rate and the full levels.
 *
 * @param channel The sound effect channel.
 * @param sample The sample.
 * @param length The length of the sample, which is not read.
 * @param loopLength The length of the loop at the end of the sample, or 0 to play once.
 * @return Whether the sample plays, and false when it has no buffer for the channel.
 * @ghidraAddress BRD3D: 0x100752d0
 * @ghidraAddress BRGlide: 0x1006e530
 */
bool InitSoundChannel(s16 channel, s32 sample, u32 length, u32 loopLength);

/**
 * Play a sample on a sound effect channel at its own rate and no level, for a later change of the
 * levels to raise.
 *
 * @param channel The sound effect channel.
 * @param sample The sample.
 * @param length The length of the sample, which is not read.
 * @param loopLength The length of the loop at the end of the sample, or 0 to play once.
 * @return Whether the sample plays, and false when it has no buffer for the channel.
 * @ghidraAddress BRD3D: 0x10075300
 * @ghidraAddress BRGlide: 0x1006e560
 */
bool StartSoundChannelMuted(s16 channel, s32 sample, u32 length, u32 loopLength);
#else
/**
 * Play a sample on a sound effect channel at its own rate and the middle levels.
 *
 * @param channel The sound effect channel.
 * @param sample The first byte of the sample.
 * @param length The length of the sample, in bytes.
 * @param loopLength The length of the loop at the end of the sample in bytes, or 0 to play once.
 * @ghidraAddress USA: 0x80257b04
 * @ghidraAddress EUR: 0x802590d4
 * @ghidraAddress JPN: 0x80258a64
 */
void InitSoundChannel(s16 channel, s8 *sample, u32 length, u32 loopLength);

/**
 * Play a sample on a sound effect channel at its own rate and no level, for SetSoundChannelVolume()
 * to raise.
 *
 * @param channel The sound effect channel.
 * @param sample The first byte of the sample.
 * @param length The length of the sample, in bytes.
 * @param loopLength The length of the loop at the end of the sample in bytes, or 0 to play once.
 * @ghidraAddress USA: 0x80257b78
 * @ghidraAddress EUR: 0x80259148
 * @ghidraAddress JPN: 0x80258ad8
 */
void StartSoundChannelMuted(s16 channel, s8 *sample, u32 length, u32 loopLength);
#endif /* TGR_PC */

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_SOUNDCHANNEL_H */
