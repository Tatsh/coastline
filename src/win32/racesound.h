#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * The sound effects of the vehicles of a race of the PC port.
 */

/**
 * Silence sample 24 on the voice of each vehicle of the race, while the race pauses.
 *
 * @ghidraAddress BRD3D: 0x100683d0
 * @ghidraAddress BRGlide: 0x10061440
 */
void MuteVehicleSoundVoices(void);

/**
 * Give the voice of a player an entry of the banks of sounds, load its sound effect again, and
 * start the engine muted on its first channel.
 *
 * @param nPlayer The player, whose voice and effect share its index.
 * @param nIndex The entry of the banks of sounds.
 * @ghidraAddress BRD3D: 0x10068260
 * @ghidraAddress BRGlide: 0x100612d0
 */
void ReinitPlayerEngineSoundChannel(int nPlayer, int nIndex);

#ifdef __cplusplus
}
#endif
