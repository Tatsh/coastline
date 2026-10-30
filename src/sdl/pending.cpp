// ponytail: the routines of the Windows libraries the race engine calls and the SDL game does not
// implement yet. Each records its first call in the log and does nothing. Every routine here calls
// Windows (DirectInput, DirectSound, the EAR music channel, the renderers, or the application
// window), or loads tracks and models through 32-bit relocation, and later milestones give each an
// SDL implementation.
#include <cstdlib>

#include <SDL3/SDL.h>

#include "common/camera.h"
#include "common/damagetexture.h"
#include "common/fatalerror.h"
#include "common/inputpoll.h"
#include "common/model.h"
#include "common/racerslot.h"
#include "common/racesound.h"
#include "common/relocation.h"
#include "common/renderbackend.h"
#include "common/season.h"
#include "common/session.h"
#include "common/soundchannel.h"
#include "common/soundeffect.h"
#include "common/soundsystem.h"
#include "common/track.h"
#include "common/trackrelocation.h"
#include "common/vehicle.h"
#include "common/vehiclecolor.h"
#include "common/vehiclemodel.h"
#include "common/vehiclesound.h"

#define PENDING()                                                                                  \
    do {                                                                                           \
        static bool s_bLogged;                                                                     \
        if (!s_bLogged) {                                                                          \
            s_bLogged = true;                                                                      \
            SDL_Log("%s is not implemented in the SDL game yet.", __func__);                       \
        }                                                                                          \
    } while (false)

// The application and the game session.
[[noreturn]] void ShowFatalErrorScreen(const char *pszMessage) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", pszMessage);
    std::exit(EXIT_FAILURE);
}

[[noreturn]] void ShutdownApplication(int nExitCode) {
    std::exit(nExitCode);
}

void ShutdownGameSession(void) {
    PENDING();
}

// The input of the players.
void ResetMouseState(void) {
    PENDING();
}

unsigned ReadInputDevicesButtonMask(int *pnSteering, int *pnAcceleration) {
    PENDING();
    return 0;
}

void UpdateForceFeedbackForce(int bReduceForce, int bEnableForce, int nForce) {
    PENDING();
}

void SetForceFeedbackDirection(int nDirection) {
    PENDING();
}

// The textures of the vehicles and the racers.
void ApplyEnemyCarColorPanels(Vehicle *vehicle, RacerSlot *slot) {
    PENDING();
}

void MakeEnemyCarColorPanels(RacerSlot *slot) {
    PENDING();
}

void RefreshVehicleMaterialTextures(VehicleModel *model) {
    PENDING();
}

void ApplyRandomAdjacentDamageTexture(int nTexture, const int *pnEntries, int *pnLevels) {
    PENDING();
}

// The tracks and the models.
void LoadTrackGeometryFromRom(s32 track) {
    PENDING();
}

void LoadTrackTextureMap(int track) {
    PENDING();
}

void LoadVehicleModelFromRom(VehicleModel *model, s32 car) {
    PENDING();
}

Model *DecompressModel(void *dst, const char *source) {
    PENDING();
    return nullptr;
}

void ByteSwap3DWords(Vec3f *pData) {
    PENDING();
}

void ByteSwapTerrainVertex(TrackPoint *point) {
    PENDING();
}

void RelocatePointerValue(unsigned *pdwPointer) {
    PENDING();
}

void ResetModelDataPool(void) {
    PENDING();
}

// The renderer.
void CopyTileColumnScaled(int nTileIndex, int nSrcX, int nSrcY, int nSrcRow) {
    PENDING();
}

void CreateGlobalEffectTextures(void) {
    PENDING();
}

void GetPrecipitationTextureDimensions(int nIndex) {
    PENDING();
}

// The sound and the music.
void Vehicle::UpdateVehicleSounds(void) {
    PENDING();
}

void ApplyAudioVolumeSettings(void) {
    PENDING();
}

bool DispatchMusicVolumeChange(unsigned char bVolume) {
    PENDING();
    return false;
}

bool InitSoundChannel(s16 channel, s32 sample, u32 length, u32 loopLength) {
    PENDING();
    return false;
}

bool StartSoundChannelMuted(s16 channel, s32 sample, u32 length, u32 loopLength) {
    PENDING();
    return false;
}

void LoadAllSoundEffects(void) {
    PENDING();
}

bool MuteAllActiveSoundChannels(void) {
    PENDING();
    return false;
}

void MuteVehicleSoundVoices(void) {
    PENDING();
}

bool PauseMusicPlayback(void) {
    PENDING();
    return false;
}

bool UnpauseMusicPlayback(void) {
    PENDING();
    return false;
}

void PlayVehicleEventSounds(Vehicle *vehicle, CameraFrame *listener) {
    PENDING();
}

int RandomizeMusicTrack(void) {
    PENDING();
    return 0;
}

bool ResetAudioSubsystem(void) {
    PENDING();
    return false;
}

bool ResumeLoopingSoundChannels(void) {
    PENDING();
    return false;
}

bool SetCurrentMusicTrack(int nModeIndex) {
    PENDING();
    return false;
}

bool StopAudioChannel(int nChannel) {
    PENDING();
    return false;
}
