#include "platform/win/racesound.h"

#include "platform/win/boot/rallyconfig.h"
#include "platform/win/input.h"
#include "platform/win/sound.h"
#include "screen.h"
#include "soundchannel.h"
#include "soundeffect.h"
#include "soundmix.h"
#include "soundpan.h"
#include "vehicle.h"
#include "vehiclecount.h"
#include "vehiclesound.h"

// The sample MuteVehicleSoundVoices() silences on the voice of each vehicle.
#define VEHICLE_PAUSE_SAMPLE 24
// The channels of the sound effects of a vehicle.
#define CHANNELS_PER_VEHICLE 2

void LoadAllSoundEffects(void) {
    if (g_GlobalRallyConfig.dwInputDeviceType == 1 || g_GlobalRallyConfig.dwInputDeviceType == 2) {
        g_GlobalRallyConfig.dwInputDeviceType = InitializeJoystickDevice();
        g_GlobalRallyConfig.SelectActiveInputBindings();
    }
    for (int i = 0; i < SOUND_EFFECTS; ++i) {
        g_aSoundEffects[i].sample = i;
    }
    ResetSoundBankTables();
    for (int i = 0; i < g_dwNumActiveVehicles; ++i) {
        SetSoundBankEntryIndex(i, g_aVehicles[i].carModel);
    }
    LoadSoundSet(1);
    StartSoundChannelMuted(0,
                           g_aSoundEffects[0].sample,
                           g_aSoundEffects[0].length,
                           static_cast<u32>(g_aSoundEffects[0].loopLength));
    if (g_dwNumActiveVehicles > 1) {
        StartSoundChannelMuted(2,
                               g_aSoundEffects[0].sample,
                               g_aSoundEffects[0].length,
                               static_cast<u32>(g_aSoundEffects[0].loopLength));
    }
    if (g_dwNumActiveVehicles > 2) {
        StartSoundChannelMuted(4,
                               g_aSoundEffects[0].sample,
                               g_aSoundEffects[0].length,
                               static_cast<u32>(g_aSoundEffects[0].loopLength));
    }
}

void ReinitPlayerEngineSoundChannel(int nPlayer, int nIndex) {
    const SoundEffect &engine = g_aSoundEffects[SOUND_EFFECT_ENGINE];

    SetSoundBankEntryIndex(nPlayer, nIndex);
    // The result of the load is discarded.
    LoadSfxWaveVariants(nPlayer);
    StartSoundChannelMuted(static_cast<s16>(nPlayer * CHANNELS_PER_VEHICLE),
                           engine.sample,
                           engine.length,
                           static_cast<u32>(engine.loopLength));
}

void MuteVehicleSoundVoices(void) {
    for (int i = 0; i < g_dwNumActiveVehicles; i++) {
        // The result of the change is discarded.
        SetSoundVoiceStereo(VEHICLE_PAUSE_SAMPLE, i, 0);
    }
}

void Vehicle::UpdateVehicleSounds(void) {
    ::UpdateVehicleSounds(this);
}

void ClearVehicleSoundChannel(s32 channel) {
    g_aSoundChannelRequested[channel].dwStereoLevels = 0;
    g_aSoundChannelRequested[channel].llFrequency = 0;
}

void SetVehicleSoundChannel(s32 channel, u64 step, u32 levels) {
    g_aSoundChannelRequested[channel].llFrequency = static_cast<long long>(step);
    g_aSoundChannelRequested[channel].dwStereoLevels = levels;
}

s32 IsVehicleSoundChannelPlaying(s32 channel) {
    return g_aSoundChannelRequested[channel].llFrequency != 0;
}

void SetVehicleSoundEchoOffset(s32 offset) {
    g_bAltLoopingSoundSet = offset;
}

static u32 VehicleEventLevels(Vehicle *vehicle, CameraFrame *listener, s32 mono) {
    f32 upper;
    f32 lower;
    s32 volume;

    ComputeStereoPan(&vehicle->frame.position, listener, &upper, &lower, &volume, FALSE);
    if (mono) {
        if (mono == 1 && volume > 2) {
            volume = 32;
        }
        return (static_cast<u32>(volume) << 16) + static_cast<u32>(volume);
    }
    return PackVolume(volume, upper, lower);
}

void PlayVehicleEventSounds(Vehicle *vehicle, CameraFrame *listener) {
    u32 levels;
    u8 strength;

    strength = vehicle->body.vehicleImpactVolume;
    if (strength > 127) {
        levels = VehicleEventLevels(vehicle, listener, 0);
        PlaySoundChannelOneOnce(strength < 171 ? 17 : strength < 213 ? 16 : 1, levels);
        SetForceFeedbackShortDuration();
        StartForceFeedbackJolt();
    }
    vehicle->body.vehicleImpactVolume = 0;
    strength = vehicle->body.impactVolume;
    if (strength > 127) {
        levels = VehicleEventLevels(vehicle, listener, 0);
        PlaySoundChannelOneOnce(strength < 171 ? 19 : strength < 213 ? 18 : 2, levels);
        StartForceFeedbackJolt();
        SetForceFeedbackShortDuration();
    }
    vehicle->body.impactVolume = 0;
    if (vehicle->body.landed > 127) {
        levels = VehicleEventLevels(vehicle, listener, 0);
        PlaySoundChannelOneOnce(3, levels);
    }
    vehicle->body.landed = 0;

    u8 *events[4] = {&vehicle->body.unreconstructed202,
                     &vehicle->body.unreconstructed203,
                     &vehicle->body.unreconstructed204,
                     &vehicle->body.replayEffects[0]};
    for (s32 i = 0; i < 4; i++) {
        if (*events[i] > 127) {
            levels = VehicleEventLevels(vehicle, listener, 1);
            PlaySoundChannelOneOnce(20 + i, levels);
        }
        *events[i] = 0;
    }

    if (vehicle->body.replayEffects[1] > 127) {
        levels = VehicleEventLevels(vehicle, listener, 2);
        if (vehicle->dopplerFactor >= 0.0f) {
            SetSoundVoiceFrequencyFloat(
                24,
                vehicle->index,
                static_cast<float>(static_cast<long double>(vehicle->dopplerFactor) * 22050.0f));
            // The original formats the Doppler factor for a discarded diagnostic.
            NullStub();
        }
        if (vehicle->body.replayEffects[2] <= 127) {
            PlayLoopedSoundVoiceWithStereo(24, vehicle->index, levels);
        } else {
            SetSoundVoiceStereo(24, vehicle->index, levels);
        }
    } else if (vehicle->body.replayEffects[2] > 127) {
        StopSoundVoice(24, vehicle->index);
    }
    vehicle->body.replayEffects[2] = vehicle->body.replayEffects[1];
    vehicle->body.replayEffects[1] = 0;
}
