#include "platform/win/racesound.h"

#include "platform/win/sound.h"
#include "vehiclecount.h"

// The sample MuteVehicleSoundVoices() silences on the voice of each vehicle.
#define VEHICLE_PAUSE_SAMPLE 24

void MuteVehicleSoundVoices(void) {
    for (int i = 0; i < g_dwNumActiveVehicles; i++) {
        // The result of the change is discarded.
        SetSoundVoiceStereo(VEHICLE_PAUSE_SAMPLE, i, 0);
    }
}
