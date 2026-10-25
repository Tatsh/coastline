#include "platform/win/racesound.h"

#include "platform/win/sound.h"
#include "soundchannel.h"
#include "soundeffect.h"
#include "vehiclecount.h"

// The sample MuteVehicleSoundVoices() silences on the voice of each vehicle.
#define VEHICLE_PAUSE_SAMPLE 24
// The channels of the sound effects of a vehicle.
#define CHANNELS_PER_VEHICLE 2

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
