#include "enginecheck.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <system_error>
#include <vector>

#include <SDL3/SDL.h>

#include "common/assetnames.h"
#include "common/fileio.h"
#include "common/ghost.h"
#include "common/inputpoll.h"
#include "common/model.h"
#include "common/podarchive.h"
#include "common/racephases.h"
#include "common/rallyconfig.h"
#include "common/renderbackend.h"
#include "common/settings.h"
#include "common/soundbuffer.h"
#include "common/track.h"
#include "common/trackselect.h"
#include "common/vehiclemodel.h"
#include "ear/sdlmixer.h"
#include "input.h"
#include "racedemo.h"
#include "racemusic.h"
#include "trackload.h"

namespace coastline {

namespace {

// The track and the car the check loads, and the sound effect it plays.
constexpr int CHECK_TRACK = 0;
constexpr int CHECK_CAR = 0;
constexpr int CHECK_SAMPLE = 3;
constexpr DWORD FULL_LEVELS = 32u << 16 | 32u;
constexpr int RENDER_FRAMES = 22050;

// The model of the start lights the race loads from BossRally.pod, in static data the 32-bit
// pointers of the model can address.
alignas(16) unsigned char g_abCheckModelBuffer[0x10000];

bool CheckInput() {
    g_GlobalRallyConfig.InitRallyConfigDefaults();
    g_GlobalRallyConfig.LoadDeviceDefaultBindings(0);
    g_GlobalRallyConfig.SelectActiveInputBindings();
    g_bRaceStartReached = 1;
    int steering = 0;
    int acceleration = 0;
    SetScriptedKeys({});
    ReadInputDevicesButtonMask(&steering, &acceleration);
    SetScriptedKeys({SDL_SCANCODE_LEFT, SDL_SCANCODE_UP});
    unsigned mask = ReadInputDevicesButtonMask(&steering, &acceleration);
    SetScriptedKeys({});
    SDL_Log("Input: left and up give the mask 0x%x, steering %d, and acceleration %d.",
            mask,
            steering,
            acceleration);
    // The keys of steering left and acceleration stay down, at full throttle.
    return mask == 0x5 && steering == 0 && acceleration == 80;
}

bool CheckSound() {
    EarSetSdlMixerOffline(true);
    if (!InitializeDirectSound()) {
        return false;
    }
    ResetSoundBankTables();
    SetSoundBankEntryIndex(0, CHECK_CAR);
    LoadSoundSet(SOUND_SET_RACE);
    int buffers = 0;
    for (SoundBufferNode *node = g_SoundBufferListHead.pNext; node != nullptr; node = node->pNext) {
        ++buffers;
    }
    PlaySoundChannelOneOnce(CHECK_SAMPLE, FULL_LEVELS);
    std::vector<std::int16_t> samples(static_cast<std::size_t>(RENDER_FRAMES) * EAR_MIX_CHANNELS);
    EarRenderSdlMixer(samples.data(), RENDER_FRAMES);
    auto audible = std::ranges::count_if(samples, [](std::int16_t sample) { return sample != 0; });
    SDL_Log("Sound: %d buffers loaded, and sample %d mixes %td audible samples of %zu.",
            buffers,
            CHECK_SAMPLE,
            audible,
            samples.size());
    ShutdownDirectSound();
    return buffers > 0 && audible > 0;
}

bool CheckMusic(RaceMusicSource source, const char *name) {
    StartRaceMusic("music", source);
    int track = RandomizeMusicTrack();
    SetCurrentMusicTrack(track);
    std::size_t audible = RenderRaceMusic(RENDER_FRAMES);
    SDL_Log("Music (%s): track %d renders %zu audible samples.", name, track, audible);
    StopRaceMusic();
    return audible > 0;
}

bool CheckTrack() {
    char path[1024];
    SDL_snprintf(path, sizeof(path), "%s%s", g_szTracks, g_apszTrackFilenames[CHECK_TRACK]);
    if (!CHK_FileExists(path)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "%s is missing. Import the disc again for the images of the tracks.",
                     path);
        return false;
    }
    ResetModelDataPool();
    LoadTrackGeometryFromRom(CHECK_TRACK);
    const TrackHeader &track = g_LoadedTrack;
    SDL_Log("Track %s: %d vertices, %d surfaces, %d textures, %d instances, %d sections, %d "
            "camera points, %d gates, start (%.1f, %.1f, %.1f), %u converted vertices, %d entries "
            "of the texture map.",
            path,
            track.vertexCount,
            track.surfaceCount,
            track.textureCount,
            track.instanceCount,
            track.sectionCount,
            track.cameraPointCount,
            track.gateCount,
            static_cast<double>(track.startPosition.x),
            static_cast<double>(track.startPosition.y),
            static_cast<double>(track.startPosition.z),
            ConvertedVertexCount(),
            LoadedTextureMapEntries());
    return track.vertexCount > 0 && track.instanceCount > 0 && ConvertedVertexCount() > 0 &&
           track.sections != nullptr;
}

bool CheckCar() {
    char path[1024];
    SDL_snprintf(
        path, sizeof(path), "%s%s.rca", g_szCarsDirPath, g_apszCarModelPrefixes[CHECK_CAR]);
    if (!CHK_FileExists(path)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "%s is missing. Import the disc again for the images of the cars.",
                     path);
        return false;
    }
    unsigned before = ConvertedVertexCount();
    LoadVehicleModelFromRom(&g_aVehicleModels[0], CHECK_CAR);
    const VehicleModel &model = g_aVehicleModels[0];
    SDL_Log("Car %s: %u triangles, %d textures, wheel 0 at (%.2f, %.2f, %.2f), %u converted "
            "vertices.",
            path,
            model.triangleCount,
            model.textureCount,
            static_cast<double>(model.wheels[0].x),
            static_cast<double>(model.wheels[0].y),
            static_cast<double>(model.wheels[0].z),
            ConvertedVertexCount() - before);
    return model.triangleCount > 0 && model.textureCount > 0 && ConvertedVertexCount() > before;
}

bool CheckPodModel() {
    g_BossRallyPodArchive.SetFilePath("BossRally.pod");
    g_BossRallyPodArchive.OpenPodFile();
    Model *model = DecompressModel(g_abCheckModelBuffer, "misc\\modelLights.blob");
    SDL_Log("BossRally.pod: misc\\modelLights.blob has %u parts.",
            static_cast<unsigned>(model->partCount));
    g_BossRallyPodArchive.ClosePodFile();
    return model->partCount > 0;
}

// The demo runs a few seconds of its race, through the countdown and past the start.
bool CheckAttractDemo() {
    constexpr int DEMO_FRAMES = 300;
    OpenRaceData();
    StartAttractDemo();
    int frames = 0;
    Vec3f start{};
    while (frames < DEMO_FRAMES && StepAttractDemo(false)) {
        ++frames;
        if (frames == 1) {
            start = g_aVehicles[0].GetFrame().position;
        }
    }
    const Vec3f &position = g_aVehicles[0].GetFrame().position;
    SDL_Log("Attract demo: track %d, car %d, %d frames, the car from (%.1f, %.1f, %.1f) to (%.1f, "
            "%.1f, %.1f).",
            static_cast<int>(g_dwSelectedTrackId),
            static_cast<int>(g_GhostRecordingInfo.car),
            frames,
            static_cast<double>(start.x),
            static_cast<double>(start.y),
            static_cast<double>(start.z),
            static_cast<double>(position.x),
            static_cast<double>(position.y),
            static_cast<double>(position.z));
    return frames == DEMO_FRAMES && (start.x != position.x || start.y != position.y);
}

} // namespace

int CheckEngine(const std::filesystem::path &defaults) {
    std::error_code error;
    std::filesystem::current_path(defaults, error);
    if (error) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "%s cannot be entered: %s",
                     defaults.string().c_str(),
                     error.message().c_str());
        return EXIT_FAILURE;
    }
    if (!SDL_Init(SDL_INIT_EVENTS)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL did not start: %s", SDL_GetError());
        return EXIT_FAILURE;
    }
    struct Check {
        const char *name;
        bool passed;
    };
    const Check checks[] = {
        {"input", CheckInput()},
        {"sound", CheckSound()},
        {"music of the modules", CheckMusic(RaceMusicSource::Modules, "modules")},
        {"music of the CD", CheckMusic(RaceMusicSource::CdAudio, "CD")},
        {"track", CheckTrack()},
        {"car", CheckCar()},
        {"model of the pod", CheckPodModel()},
        {"attract demo", CheckAttractDemo()},
    };
    int failures = 0;
    for (const Check &check : checks) {
        SDL_Log("%s: %s", check.name, check.passed ? "passed" : "FAILED");
        failures += check.passed ? 0 : 1;
    }
    SDL_Quit();
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

} // namespace coastline
