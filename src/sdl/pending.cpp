// ponytail: the routines of the Windows libraries the race engine calls and the SDL game does not
// implement yet. Each records its first call in the log and does nothing. Later milestones replace
// them with real implementations, and move the game logic among them into src/common.
#include "sdl/pending.h"

#include <cstdlib>

#include <PR/os.h>
#include <SDL3/SDL.h>

#include "common/buildinfo.h"
#include "common/camera.h"
#include "common/combine.h"
#include "common/damagetexture.h"
#include "common/debugmessage.h"
#include "common/driving.h"
#include "common/fatalerror.h"
#include "common/fileio.h"
#include "common/frameclock.h"
#include "common/ghost.h"
#include "common/hud.h"
#include "common/inputpoll.h"
#include "common/model.h"
#include "common/packedvalue.h"
#include "common/playerinput.h"
#include "common/racermotion.h"
#include "common/racers.h"
#include "common/racerslot.h"
#include "common/racesound.h"
#include "common/rallyconfig.h"
#include "common/relocation.h"
#include "common/renderbackend.h"
#include "common/screen.h"
#include "common/season.h"
#include "common/session.h"
#include "common/soundchannel.h"
#include "common/soundeffect.h"
#include "common/soundsystem.h"
#include "common/strings.h"
#include "common/track.h"
#include "common/trackrelocation.h"
#include "common/vehicle.h"
#include "common/vehiclecolor.h"
#include "common/vehiclemodel.h"
#include "common/vehiclephysics.h"
#include "common/vehiclesound.h"

#define PENDING()                                                                                  \
    do {                                                                                           \
        static bool s_bLogged;                                                                     \
        if (!s_bLogged) {                                                                          \
            s_bLogged = true;                                                                      \
            SDL_Log("%s is not implemented in the SDL game yet.", __func__);                       \
        }                                                                                          \
    } while (false)

// The timing of the frames.
u64 osClockRate = 2165625000ULL;

u64 osGetCount(void) {
    PENDING();
    return 0;
}

u32 GetHighResolutionTime(void) {
    PENDING();
    return 0;
}

void AdvanceFrameTickCounter(void) {
    PENDING();
}

void ResetFrameTimer(void) {
    PENDING();
}

void ResetFrameTimerState(void) {
    PENDING();
}

void UpdateRaceFrameStatistics(void) {
    PENDING();
}

void PaceRaceFrames(void) {
    PENDING();
}

// The files and the messages of the game.
char *GetStringResource(unsigned nResourceId) {
    PENDING();
    return nullptr;
}

void SetDebugMessage(const char *message) {
    PENDING();
}

[[noreturn]] void ShowFatalErrorScreen(const char *pszMessage) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", pszMessage);
    std::exit(EXIT_FAILURE);
}

[[noreturn]] void ShutdownApplication(int nExitCode) {
    std::exit(nExitCode);
}

// The constructor of g_GlobalRallyConfig runs before main(), and does not log.
RallyConfig::RallyConfig() {
}

bool RallyConfig::SaveConfigToFile(const char *pszPath) {
    PENDING();
    return false;
}

// The front end, the seasons, and the session.
s32 ReturnZero(void) {
    return 0;
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

// The vehicles and the racers.
void Vehicle::UpdateVehicleSounds(void) {
    PENDING();
}

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

// The renderer.
void ClearSplitScreenRenderState(void) {
    PENDING();
}

void CopyTileColumnScaled(int nTileIndex, int nSrcX, int nSrcY, int nSrcRow) {
    PENDING();
}

void CreateGlobalEffectTextures(void) {
    PENDING();
}

void GetPrecipitationTextureDimensions(int nIndex) {
    PENDING();
}

void NullStub2(void *pMemory) {
    PENDING();
}

void PackCombineMode(GbiCommand *pCommand,
                     int nColorA0,
                     int nColorB0,
                     int nColorC0,
                     int nColorD0,
                     int nAlphaA0,
                     int nAlphaB0,
                     int nAlphaC0,
                     int nAlphaD0,
                     int nColorA1,
                     int nColorB1,
                     int nColorC1,
                     int nColorD1,
                     int nAlphaA1,
                     int nAlphaB1,
                     int nAlphaC1,
                     int nAlphaD1) {
    PENDING();
}

void ResetModelDataPool(void) {
    PENDING();
}

void ResetRenderCommandBufferStack(void) {
    PENDING();
}

void DrawFpsCounterOverlay(void) {
    PENDING();
}

// The sound and the music.
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
