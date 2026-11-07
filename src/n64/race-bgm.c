#include "common/assetdma.h"
#include "common/audio.h"
#include "common/race.h"
#include "common/trackselect.h"

void LoadAndPlayTrackBgm(void) {
    ResetMusicPlaybackState();
    DecompressFromRom(g_abMenuMusicModule, g_adwTrackMusicRom[g_dwSelectedTrackId], NULL);
    InitMusicPlayback(g_abMenuMusicModule, g_abMusicModuleWorkBuffer);
}
