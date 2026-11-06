#include <SDL3/SDL.h>

#include "common/soundsampleinput.h"
#include "sdl/soundeffects.h"

int LoadSoundBufferSample(const char *path, SoundBufferNode *node, int sample) {
    SDL_AudioSpec spec;
    Uint8 *data = nullptr;
    Uint32 bytes = 0;
    if (!coastline::SoundEffects::LoadRaceWave(path, sample, &spec, &data, &bytes)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO, "%s cannot be read: %s", path, SDL_GetError());
        return 1;
    }
    if (spec.format != SDL_AUDIO_U8 && spec.format != SDL_AUDIO_S16LE) {
        SDL_AudioSpec target = {SDL_AUDIO_S16LE, spec.channels, spec.freq};
        Uint8 *converted = nullptr;
        int convertedBytes = 0;
        bool success = SDL_ConvertAudioSamples(
            &spec, data, static_cast<int>(bytes), &target, &converted, &convertedBytes);
        SDL_free(data);
        if (!success) {
            SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO, "%s cannot be played: %s", path, SDL_GetError());
            return 1;
        }
        spec = target;
        data = converted;
        bytes = static_cast<Uint32>(convertedBytes);
    }
    auto *format = static_cast<EarWaveFormat *>(SDL_calloc(1, sizeof(EarWaveFormat)));
    if (format == nullptr) {
        SDL_free(data);
        return 1;
    }
    format->wFormatTag = 1;
    format->nChannels = static_cast<WORD>(spec.channels);
    format->nSamplesPerSec = static_cast<DWORD>(spec.freq);
    format->wBitsPerSample = static_cast<WORD>(SDL_AUDIO_BITSIZE(spec.format));
    format->nBlockAlign = static_cast<WORD>(spec.channels * format->wBitsPerSample / 8);
    format->nAvgBytesPerSec = format->nSamplesPerSec * format->nBlockAlign;
    node->pWaveFormat = format;
    node->pSampleData = data;
    node->dwSampleSize = bytes;
    return 0;
}
