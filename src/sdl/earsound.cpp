#include "earsound.h"

#include <bit>
#include <cstdint>
#include <string>
#include <vector>

#include <SDL3/SDL.h>

#include "ear/channel.h"
#include "ear/configuration.h"
#include "ear/directionalmatrix.h"
#include "ear/error.h"
#include "ear/event.h"
#include "ear/initialization.h"
#include "ear/runtime.h"
#include "ear/sdlmixer.h"
#include "ear/shutdown.h"
#include "ear/window.h"
#include "music.h"

namespace coastline {

namespace {

// The key the PC release validates EAR with.
constexpr std::uint32_t EAR_VALIDATION_KEY = 0x9be9c9;
// The flag of EAR_DLL_InitializeEar() that does not start the timer.
constexpr std::uint32_t EAR_INITIALIZE_WITHOUT_TIMER = 0x80;
// The channel, the matrix of one direction, and the environment the sound plays with.
constexpr std::uint16_t SOUND_CHANNEL = 1;
constexpr std::uint16_t SOUND_MATRIX = 1;
constexpr std::uint16_t DEFAULT_ENVIRONMENT = 1;
// The source type of a wave file read from a path, and the size of the parameters of an event.
constexpr std::uint16_t SOURCE_WAVE_FILE = 0;
constexpr std::uint16_t EVENT_PARAMS_SIZE = 0x80;
// The distance unit that is already centimetres, and the gain of a whole sound.
constexpr std::uint32_t CENTIMETERS = 4;
constexpr std::uint16_t FULL_GAIN = 10000;
// The notification EAR posts when an event stops.
constexpr Sint32 EVENT_STOPPED = 2;
// The period of the timer of EAR, in milliseconds, at which a rendered mix updates EAR.
constexpr int UPDATE_MILLISECONDS = 50;
// The bit of EAR_DLL_GetEventStatus() of an event that is playing.
constexpr std::uint32_t EVENT_STATUS_ACTIVE = 4;
// The time the timer of EAR has to start an event, in milliseconds.
constexpr Uint64 START_MILLISECONDS = 1000;
// The longest mix rendered, in milliseconds, against a sound that never stops.
constexpr std::uint64_t LONGEST_RENDER_MILLISECONDS = 10 * 60 * 1000;

bool LogEarFailure(const char *step) {
    SDL_LogError(
        SDL_LOG_CATEGORY_AUDIO, "EAR failed to %s, error %u.", step, EAR_DLL_GetLastError());
    return false;
}

bool IsStopOfEvent(const SDL_Event &event, std::uint32_t handle) {
    return event.type == g_uEarNotifyMessage && event.user.code == EVENT_STOPPED &&
           std::bit_cast<std::uintptr_t>(event.user.data1) == handle;
}

// Take the events SDL queued, and report whether one of them is the stop of the event.
bool EventStopped(std::uint32_t handle) {
    SDL_Event event;
    bool stopped = false;
    while (SDL_PollEvent(&event)) {
        stopped = stopped || IsStopOfEvent(event, handle);
    }
    return stopped;
}

} // namespace

bool PlayEarSound(const std::filesystem::path &path, const EarSoundOptions &options) {
    EarSetSdlMixerOffline(options.output.has_value());
    EAR_DLL_AAA_Validate(EAR_VALIDATION_KEY);
    if (!EAR_DLL_InitializeEar(options.output ? EAR_INITIALIZE_WITHOUT_TIMER : 0)) {
        return LogEarFailure("start");
    }
    std::string source = path.string();
    EarEventParams params = {};
    params.size = EVENT_PARAMS_SIZE;
    params.sourceType = SOURCE_WAVE_FILE;
    params.source = source.c_str();
    params.channelHandle = SOUND_CHANNEL;
    params.matrixHandle = SOUND_MATRIX;
    params.minimumGain = FULL_GAIN;
    params.maximumGain = FULL_GAIN;
    params.flags = EAR_EVENT_NOTIFY_STOP | (options.whole ? EAR_EVENT_NOT_STREAMING : 0);
    params.direction = static_cast<std::uint16_t>(options.direction);
    params.environmentHandle = DEFAULT_ENVIRONMENT;
    params.distanceUnit = CENTIMETERS;
    std::uint32_t handle = 0;
    if (!EAR_DLL_RegisterChannel(SOUND_CHANNEL, 0, 100, 0)) {
        LogEarFailure("register the channel");
    } else if (!EAR_DLL_RegisterMatrix(SOUND_MATRIX)) {
        LogEarFailure("register the matrix");
    } else if (!(handle = EAR_DLL_MixEvent(&params))) {
        LogEarFailure("mix the sound");
    }
    if (!handle) {
        EAR_DLL_ShutDownEar();
        return false;
    }
    bool stopped = false;
    std::vector<std::int16_t> samples;
    if (options.output) {
        // Each update of EAR follows the mix of the time of a period of its timer.
        std::uint64_t frames = 0;
        for (std::uint64_t period = 1; !stopped; ++period) {
            EAR_DLL_UpdateEar();
            std::uint32_t error = EAR_DLL_GetLastError();
            stopped = EventStopped(handle);
            if (stopped || period * UPDATE_MILLISECONDS > LONGEST_RENDER_MILLISECONDS) {
                break;
            }
            // An event that cannot start is retired without a notification.
            if (!(EAR_DLL_GetEventStatus(SOUND_CHANNEL, handle) & EVENT_STATUS_ACTIVE)) {
                SDL_LogError(
                    SDL_LOG_CATEGORY_AUDIO, "EAR failed to play the sound, error %u.", error);
                break;
            }
            std::uint64_t end = period * UPDATE_MILLISECONDS * EAR_MIX_RATE / 1000;
            samples.resize(static_cast<std::size_t>(end) * EAR_MIX_CHANNELS);
            EarRenderSdlMixer(samples.data() + frames * EAR_MIX_CHANNELS,
                              static_cast<int>(end - frames));
            frames = end;
        }
    } else {
        bool started = false;
        Uint64 mixed = SDL_GetTicks();
        while (!stopped) {
            SDL_Event event;
            if (SDL_WaitEventTimeout(&event, UPDATE_MILLISECONDS)) {
                stopped = IsStopOfEvent(event, handle);
                continue;
            }
            // The timer starts the event at its first update, and an event that cannot start is
            // retired without a notification.
            bool active = EAR_DLL_GetEventStatus(SOUND_CHANNEL, handle) & EVENT_STATUS_ACTIVE;
            started = started || active;
            if (!active && (started || SDL_GetTicks() - mixed > START_MILLISECONDS)) {
                SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "EAR failed to play the sound.");
                break;
            }
        }
        // The device plays what the mixer gave it last.
        SDL_Delay(200);
    }
    EAR_DLL_ShutDownEar();
    if (!stopped) {
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "The sound did not stop.");
        return false;
    }
    if (options.output) {
        if (!WriteWav(*options.output, samples, EAR_MIX_RATE)) {
            SDL_LogError(
                SDL_LOG_CATEGORY_AUDIO, "%s cannot be written.", options.output->string().c_str());
            return false;
        }
        SDL_Log("Wrote %zu frames to %s.",
                samples.size() / EAR_MIX_CHANNELS,
                options.output->string().c_str());
    }
    return true;
}

} // namespace coastline
