// The mixer of the buffers of EAR in the SDL game. It plays them as DirectSound plays its
// secondary buffers into a primary buffer of 22,050 Hz, 16-bit stereo.

#include "ear/sdlmixer.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <vector>

#include <SDL3/SDL.h>

#include "ear/timer.h"

namespace {

// The limits of the units of DirectSound.
constexpr LONG DIRECTSOUND_VOLUME_MIN = -10000;
constexpr LONG DIRECTSOUND_VOLUME_MAX = 0;
constexpr LONG DIRECTSOUND_PAN_LEFT = -10000;
constexpr LONG DIRECTSOUND_PAN_RIGHT = 10000;
constexpr std::uint32_t DIRECTSOUND_FREQUENCY_MIN = 100;
constexpr std::uint32_t DIRECTSOUND_FREQUENCY_MAX = 100000;

struct Mixer {
    SDL_Mutex *mutex = nullptr;
    SDL_AudioStream *stream = nullptr;
    std::vector<EarSdlVoice *> voices;
    std::vector<float> mix;
    std::vector<std::int16_t> output;
    bool offline = false;
    std::uint64_t renderedFrames = 0;
};

Mixer g_mixer;

class MixerLock {
public:
    MixerLock() {
        SDL_LockMutex(g_mixer.mutex);
    }
    ~MixerLock() {
        SDL_UnlockMutex(g_mixer.mutex);
    }
    MixerLock(const MixerLock &) = delete;
    MixerLock &operator=(const MixerLock &) = delete;
};

// The amplitude of an attenuation in hundredths of a decibel; the minimum of DirectSound is
// silence.
float Amplitude(LONG attenuation) {
    if (attenuation <= DIRECTSOUND_VOLUME_MIN) {
        return 0;
    }
    return std::pow(10.0f, static_cast<float>(attenuation) / 2000.0f);
}

// Mix every playing buffer, saturating the sum to 16 bits. The caller locks the mixer.
void MixLocked(std::int16_t *samples, int frames) {
    std::size_t count = static_cast<std::size_t>(frames) * EAR_MIX_CHANNELS;
    g_mixer.mix.assign(count, 0.0f);
    for (EarSdlVoice *voice : g_mixer.voices) {
        voice->MixInto(g_mixer.mix.data(), frames);
    }
    for (std::size_t i = 0; i < count; ++i) {
        float value = std::nearbyint(g_mixer.mix[i]);
        samples[i] = static_cast<std::int16_t>(std::clamp(value, -32768.0f, 32767.0f));
    }
}

void SDLCALL StreamCallback(void *user, SDL_AudioStream *stream, int additional, int total) {
    int frames = additional / static_cast<int>(sizeof(std::int16_t) * EAR_MIX_CHANNELS);
    if (frames <= 0) {
        return;
    }
    MixerLock lock;
    g_mixer.output.resize(static_cast<std::size_t>(frames) * EAR_MIX_CHANNELS);
    MixLocked(g_mixer.output.data(), frames);
    SDL_PutAudioStreamData(stream,
                           g_mixer.output.data(),
                           static_cast<int>(g_mixer.output.size() * sizeof(std::int16_t)));
}

} // namespace

EarSdlVoice *EarSdlVoice::Create(const EarWaveFormat &format, std::uint32_t bytes) {
    if (format.wFormatTag != EAR_WAVE_FORMAT_PCM ||
        (format.wBitsPerSample != 8 && format.wBitsPerSample != 16) || format.nChannels == 0 ||
        format.nChannels > 2 ||
        format.nBlockAlign != format.nChannels * format.wBitsPerSample / 8 || bytes == 0) {
        return nullptr;
    }
    auto *voice = new EarSdlVoice;
    voice->format = format;
    voice->samples = static_cast<std::uint8_t *>(std::malloc(bytes));
    std::memset(voice->samples, format.wBitsPerSample == 8 ? 0x80 : 0, bytes);
    voice->bytes = bytes;
    voice->frames = bytes / format.nBlockAlign;
    voice->frequency = format.nSamplesPerSec;
    MixerLock lock;
    g_mixer.voices.push_back(voice);
    return voice;
}

void EarSdlVoice::Release() {
    {
        MixerLock lock;
        std::erase(g_mixer.voices, this);
    }
    std::free(samples);
    delete this;
}

void EarSdlVoice::Write(std::uint32_t offset, const void *data, std::uint32_t count) {
    MixerLock lock;
    const auto *source = static_cast<const std::uint8_t *>(data);
    while (count > 0 && offset < bytes) {
        std::uint32_t part = std::min(count, bytes - offset);
        std::memcpy(samples + offset, source, part);
        source += part;
        count -= part;
        offset = 0;
    }
}

void EarSdlVoice::Play(bool loop) {
    MixerLock lock;
    playing = true;
    looping = loop;
}

void EarSdlVoice::Stop() {
    MixerLock lock;
    playing = false;
}

bool EarSdlVoice::IsPlaying() const {
    MixerLock lock;
    return playing;
}

std::uint32_t EarSdlVoice::GetPlayCursor() const {
    MixerLock lock;
    return static_cast<std::uint32_t>(position >> 32) * format.nBlockAlign;
}

bool EarSdlVoice::SetPlayCursor(std::uint32_t offset) {
    if (offset >= bytes) {
        return false;
    }
    MixerLock lock;
    position = static_cast<std::uint64_t>(offset / format.nBlockAlign) << 32;
    return true;
}

void EarSdlVoice::SetFrequency(std::uint32_t value) {
    if (value < DIRECTSOUND_FREQUENCY_MIN || value > DIRECTSOUND_FREQUENCY_MAX) {
        return;
    }
    MixerLock lock;
    frequency = value;
}

void EarSdlVoice::SetVolume(LONG value) {
    if (value < DIRECTSOUND_VOLUME_MIN || value > DIRECTSOUND_VOLUME_MAX) {
        return;
    }
    MixerLock lock;
    volume = value;
    UpdateGains();
}

void EarSdlVoice::SetPan(LONG value) {
    if (value < DIRECTSOUND_PAN_LEFT || value > DIRECTSOUND_PAN_RIGHT) {
        return;
    }
    MixerLock lock;
    pan = value;
    UpdateGains();
}

void EarSdlVoice::UpdateGains() {
    // DirectSound attenuates one side by the pan and retains the other at the volume.
    leftGain = Amplitude(volume + (pan > 0 ? -pan : 0));
    rightGain = Amplitude(volume + (pan < 0 ? pan : 0));
}

float EarSdlVoice::ReadSample(std::uint32_t frame, int channel) const {
    std::size_t offset = static_cast<std::size_t>(frame) * format.nBlockAlign +
                         static_cast<std::size_t>(channel) * (format.wBitsPerSample / 8);
    if (format.wBitsPerSample == 8) {
        return (static_cast<float>(samples[offset]) - 128.0f) * 256.0f;
    }
    std::int16_t value;
    std::memcpy(&value, samples + offset, sizeof(value));
    return value;
}

void EarSdlVoice::MixInto(float *mix, int count) {
    if (!playing || frames == 0) {
        return;
    }
    // The frames between those of the buffer are interpolated linearly.
    std::uint64_t step = (static_cast<std::uint64_t>(frequency) << 32) / EAR_MIX_RATE;
    std::uint64_t end = static_cast<std::uint64_t>(frames) << 32;
    int lastChannel = format.nChannels - 1;
    for (int i = 0; i < count; ++i) {
        auto frame = static_cast<std::uint32_t>(position >> 32);
        float fraction = static_cast<float>(position & 0xffffffffu) / 4294967296.0f;
        std::uint32_t next = frame + 1 < frames ? frame + 1 : (looping ? 0 : frame);
        for (int channel = 0; channel < EAR_MIX_CHANNELS; ++channel) {
            int source = std::min(channel, lastChannel);
            float first = ReadSample(frame, source);
            float second = ReadSample(next, source);
            mix[i * EAR_MIX_CHANNELS + channel] +=
                (first + (second - first) * fraction) * (channel == 0 ? leftGain : rightGain);
        }
        position += step;
        if (position >= end) {
            if (!looping) {
                // A buffer that plays to its end stops with its cursor at its start.
                playing = false;
                position = 0;
                return;
            }
            position %= end;
        }
    }
}

void EarSetSdlMixerOffline(bool offline) {
    g_mixer.offline = offline;
}

bool EarOpenSdlMixer() {
    g_mixer.mutex = SDL_CreateMutex();
    if (!g_mixer.mutex) {
        return false;
    }
    g_mixer.renderedFrames = 0;
    if (g_mixer.offline) {
        return true;
    }
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO, "EAR cannot start the audio: %s", SDL_GetError());
        SDL_DestroyMutex(g_mixer.mutex);
        g_mixer.mutex = nullptr;
        return false;
    }
    SDL_AudioSpec spec{SDL_AUDIO_S16, EAR_MIX_CHANNELS, EAR_MIX_RATE};
    g_mixer.stream = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, StreamCallback, nullptr);
    if (!g_mixer.stream) {
        SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO, "EAR cannot open the audio device: %s", SDL_GetError());
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        SDL_DestroyMutex(g_mixer.mutex);
        g_mixer.mutex = nullptr;
        return false;
    }
    SDL_ResumeAudioStreamDevice(g_mixer.stream);
    return true;
}

void EarCloseSdlMixer() {
    if (g_mixer.stream) {
        SDL_DestroyAudioStream(g_mixer.stream);
        g_mixer.stream = nullptr;
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }
    for (EarSdlVoice *voice : std::vector<EarSdlVoice *>(g_mixer.voices)) {
        voice->Release();
    }
    SDL_DestroyMutex(g_mixer.mutex);
    g_mixer.mutex = nullptr;
}

void EarRenderSdlMixer(std::int16_t *samples, int frames) {
    MixerLock lock;
    MixLocked(samples, frames);
    g_mixer.renderedFrames += static_cast<std::uint64_t>(frames);
}

extern "C" DWORD GetEarTimeMilliseconds(void) {
    if (g_mixer.offline) {
        return static_cast<DWORD>(g_mixer.renderedFrames * 1000 / EAR_MIX_RATE);
    }
    return static_cast<DWORD>(SDL_GetTicks());
}
