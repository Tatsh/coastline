#include "win32/racesound.h"

#include "common/screen.h"
#include "common/soundchannel.h"
#include "common/soundeffect.h"
#include "common/soundmix.h"
#include "common/soundpan.h"
#include "common/vehicleaccess.h"
#include "common/vehiclecount.h"
#include "common/vehiclesound.h"
#include "vehicle.h"
#include "win32/input.h"
#include "win32/rallyconfig.h"
#include "win32/sound.h"

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
        SetSoundBankEntryIndex(i, VEHICLE_GET(&(g_aVehicles[i]), CarModel));
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

    ComputeStereoPan(
        &VEHICLE_GET(vehicle, Frame).position, listener, &upper, &lower, &volume, FALSE);
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

    strength = VEHICLE_GET(vehicle, Body).vehicleImpactVolume;
    if (strength > 127) {
        levels = VehicleEventLevels(vehicle, listener, 0);
        PlaySoundChannelOneOnce(strength < 171 ? 17 : strength < 213 ? 16 : 1, levels);
        SetForceFeedbackShortDuration();
        StartForceFeedbackJolt();
    }
    VEHICLE_GET(vehicle, Body).vehicleImpactVolume = 0;
    strength = VEHICLE_GET(vehicle, Body).impactVolume;
    if (strength > 127) {
        levels = VehicleEventLevels(vehicle, listener, 0);
        PlaySoundChannelOneOnce(strength < 171 ? 19 : strength < 213 ? 18 : 2, levels);
        StartForceFeedbackJolt();
        SetForceFeedbackShortDuration();
    }
    VEHICLE_GET(vehicle, Body).impactVolume = 0;
    if (VEHICLE_GET(vehicle, Body).landed > 127) {
        levels = VehicleEventLevels(vehicle, listener, 0);
        PlaySoundChannelOneOnce(3, levels);
    }
    VEHICLE_GET(vehicle, Body).landed = 0;

    u8 *events[4] = {&VEHICLE_GET(vehicle, Body).unreconstructed202,
                     &VEHICLE_GET(vehicle, Body).unreconstructed203,
                     &VEHICLE_GET(vehicle, Body).unreconstructed204,
                     &VEHICLE_GET(vehicle, Body).replayEffects[0]};
    for (s32 i = 0; i < 4; i++) {
        if (*events[i] > 127) {
            levels = VehicleEventLevels(vehicle, listener, 1);
            PlaySoundChannelOneOnce(20 + i, levels);
        }
        *events[i] = 0;
    }

    if (VEHICLE_GET(vehicle, Body).replayEffects[1] > 127) {
        levels = VehicleEventLevels(vehicle, listener, 2);
        if (VEHICLE_GET(vehicle, DopplerFactor) >= 0.0f) {
            SetSoundVoiceFrequencyFloat(
                24,
                VEHICLE_GET(vehicle, Index),
                static_cast<float>(static_cast<long double>(VEHICLE_GET(vehicle, DopplerFactor)) *
                                   22050.0f));
            // The original formats the Doppler factor for a discarded diagnostic.
            NullStub();
        }
        if (VEHICLE_GET(vehicle, Body).replayEffects[2] <= 127) {
            PlayLoopedSoundVoiceWithStereo(24, VEHICLE_GET(vehicle, Index), levels);
        } else {
            SetSoundVoiceStereo(24, VEHICLE_GET(vehicle, Index), levels);
        }
    } else if (VEHICLE_GET(vehicle, Body).replayEffects[2] > 127) {
        StopSoundVoice(24, VEHICLE_GET(vehicle, Index));
    }
    VEHICLE_GET(vehicle, Body).replayEffects[2] = VEHICLE_GET(vehicle, Body).replayEffects[1];
    VEHICLE_GET(vehicle, Body).replayEffects[1] = 0;
}
