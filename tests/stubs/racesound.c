#include "audio.h"
#include "race.h"
#include "rumble.h"
#include "track.h"
MixerChannel g_aSoundEffectChannels[SOUND_EFFECT_CHANNELS];
s32 g_dwSoundEffectEchoOffset;
Vehicle g_aVehicles[MAX_VEHICLES];
ViewportRect g_aViewportRects[2];
s32 g_dwNumViewports;
u32 g_dwRacePauseState, g_dwNumLocalPlayers, g_dwCurrentWeather;
s32 g_dwNumActiveVehicles;
u32 g_dwSelectedTrackId;
TrackHeader g_LoadedTrack;
u8 g_aInflateHeapBuffer[16];
s16 g_awRumbleCounter[RUMBLE_CHANNELS], g_awRumbleMotorOn[RUMBLE_CHANNELS],
    g_awRumbleOnFrames[RUMBLE_CHANNELS], g_awRumbleOffFrames[RUMBLE_CHANNELS],
    g_awRumbleFramesLeft[RUMBLE_CHANNELS], g_awRumbleActive[RUMBLE_CHANNELS];
void InitSoundChannel(s16 c, s8 *s, u32 l, u32 ll) {
}
void StartSoundChannelMuted(s16 c, s8 *s, u32 l, u32 ll) {
}
u32 DmaReadAssetSize(u32 r) {
    return 0;
}
u32 DecompressFromRom(void *d, u32 r, DecompressJob *j) {
    return 0;
}
void osSyncPrintf(const char *f, ...) {
}
