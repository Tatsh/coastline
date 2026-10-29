#ifndef COASTLINE_VEHICLE_SOUND_H
#define COASTLINE_VEHICLE_SOUND_H

#include "common/vehicle.h"

#if TGR_BOSS_RALLY
#include "common/soundsystem.h"
#endif

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

#if TGR_BOSS_RALLY
/** Clear frequency and stereo levels of a requested channel. */
static inline void ClearVehicleSoundChannel(s32 channel) {
    g_aSoundChannelRequested[channel].dwStereoLevels = 0;
    g_aSoundChannelRequested[channel].llFrequency = 0;
}
/** Set frequency and stereo levels of a requested channel. */
static inline void SetVehicleSoundChannel(s32 channel, u64 step, u32 levels) {
    g_aSoundChannelRequested[channel].llFrequency = (long long)step;
    g_aSoundChannelRequested[channel].dwStereoLevels = levels;
}
/** Whether a requested channel has a nonzero frequency. */
static inline s32 IsVehicleSoundChannelPlaying(s32 channel) {
    return g_aSoundChannelRequested[channel].llFrequency != 0;
}
/** Select the alternate looping bank beneath a shelter. */
static inline void SetVehicleSoundEchoOffset(s32 offset) {
    g_bAltLoopingSoundSet = offset;
}
/** Play Windows impact and event sounds for a vehicle. */
void PlayVehicleEventSounds(Vehicle *vehicle, CameraFrame *listener);
#endif /* TGR_BOSS_RALLY */

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_VEHICLE_SOUND_H */
