#ifndef COASTLINE_VEHICLE_SOUND_H
#define COASTLINE_VEHICLE_SOUND_H

#include "common/vehicle.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Play the engine, the impacts, and the rumble of a vehicle for a frame.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x8022bcb4
 * @ghidraAddress EUR: 0x8022d1c4
 * @ghidraAddress JPN: 0x8022c97c
 */
void UpdateVehicleSounds(Vehicle *vehicle);

#if TGR_PC
/** Clear frequency and stereo levels of a requested channel. */
void ClearVehicleSoundChannel(s32 channel);
/** Set frequency and stereo levels of a requested channel. */
void SetVehicleSoundChannel(s32 channel, u64 step, u32 levels);
/** Whether a requested channel has a nonzero frequency. */
s32 IsVehicleSoundChannelPlaying(s32 channel);
/** Select the alternate looping bank beneath a shelter. */
void SetVehicleSoundEchoOffset(s32 offset);
/** Play Windows impact and event sounds for a vehicle. */
void PlayVehicleEventSounds(Vehicle *vehicle, CameraFrame *listener);
#endif /* TGR_PC */

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_VEHICLE_SOUND_H */
