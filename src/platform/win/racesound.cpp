#include "platform/win/racesound.h"

#include "platform/win/boot/rallyconfig.h"
#include "platform/win/input.h"
#include "platform/win/sound.h"
#include "soundchannel.h"
#include "soundeffect.h"
#include "vehicle.h"
#include "vehiclecount.h"

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
