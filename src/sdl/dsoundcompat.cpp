#include "dsoundcompat.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <mutex>

#include <SDL3/SDL.h>

const GUID CLSID_DirectSound = {
    0x47d4d946, 0x62e8, 0x11cf, {0x93, 0xbc, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}};
const GUID IID_IDirectSound = {
    0x279afa83, 0x4981, 0x11ce, {0xa5, 0x21, 0x00, 0x20, 0xaf, 0x0b, 0xe5, 0x60}};

namespace {

constexpr int kMixRate = 44100;
constexpr int kMixChannels = 2;

struct Mixer {
    std::mutex mutex;
    std::vector<IDirectSoundBuffer *> buffers;
    SDL_AudioStream *stream = nullptr;
    std::vector<float> scratch;
};

Mixer &GetMixer() {
    static Mixer mixer;
    return mixer;
}

// The gain of an attenuation in hundredths of a decibel.
float Gain(LONG attenuation) {
    return attenuation <= DSBVOLUME_MIN ?
               0.0f :
               std::pow(10.0f, static_cast<float>(attenuation) / 2000.0f);
}

float Sample(const IDirectSoundBuffer &buffer, std::size_t frame, int channel) {
    const WAVEFORMATEX &format = buffer.format;
    std::size_t offset = frame * format.nBlockAlign +
                         static_cast<std::size_t>(channel) * (format.wBitsPerSample / 8);
    if (format.wBitsPerSample == 8) {
        return (static_cast<float>(buffer.bytes[offset]) - 128.0f) / 128.0f;
    }
    std::int16_t value;
    std::memcpy(&value, &buffer.bytes[offset], sizeof(value));
    return static_cast<float>(value) / 32768.0f;
}

// Add a buffer that plays into the mix, at linear interpolation between its frames.
void MixBuffer(IDirectSoundBuffer &buffer, float *out, int frames) {
    std::size_t count = buffer.bytes.size() / buffer.format.nBlockAlign;
    if (count == 0) {
        buffer.playing = false;
        return;
    }
    float volume = Gain(buffer.volume);
    float left = volume * (buffer.pan > 0 ? Gain(-buffer.pan) : 1.0f);
    float right = volume * (buffer.pan < 0 ? Gain(buffer.pan) : 1.0f);
    double step = static_cast<double>(buffer.frequency) / kMixRate;
    int lastChannel = buffer.format.nChannels > 1 ? 1 : 0;
    for (int i = 0; i < frames; ++i) {
        auto frame = static_cast<std::size_t>(buffer.position);
        auto fraction = static_cast<float>(buffer.position - static_cast<double>(frame));
        std::size_t next = frame + 1 < count ? frame + 1 : (buffer.looping ? 0 : frame);
        for (int channel = 0; channel < kMixChannels; ++channel) {
            int source = std::min(channel, lastChannel);
            float a = Sample(buffer, frame, source);
            float b = Sample(buffer, next, source);
            out[i * kMixChannels + channel] +=
                (a + (b - a) * fraction) * (channel == 0 ? left : right);
        }
        buffer.position += step;
        if (buffer.position >= static_cast<double>(count)) {
            if (!buffer.looping) {
                buffer.position = 0.0;
                buffer.playing = false;
                return;
            }
            buffer.position = std::fmod(buffer.position, static_cast<double>(count));
        }
    }
}

void SDLCALL MixAudio(void *userdata, SDL_AudioStream *stream, int additional, int total) {
    Mixer &mixer = *static_cast<Mixer *>(userdata);
    int frames = additional / static_cast<int>(sizeof(float) * kMixChannels);
    if (frames <= 0) {
        return;
    }
    std::lock_guard lock(mixer.mutex);
    mixer.scratch.assign(static_cast<std::size_t>(frames) * kMixChannels, 0.0f);
    for (IDirectSoundBuffer *buffer : mixer.buffers) {
        if (buffer->playing && !buffer->primary) {
            MixBuffer(*buffer, mixer.scratch.data(), frames);
        }
    }
    SDL_PutAudioStreamData(
        stream, mixer.scratch.data(), static_cast<int>(mixer.scratch.size() * sizeof(float)));
}

IDirectSound g_directSound;

} // namespace

HRESULT IDirectSoundBuffer::Lock(DWORD offset,
                                 DWORD count,
                                 void **audio1,
                                 DWORD *bytes1,
                                 void **audio2,
                                 DWORD *bytes2,
                                 DWORD flags) {
    auto size = static_cast<DWORD>(bytes.size());
    if (flags & DSBLOCK_ENTIREBUFFER) {
        offset = 0;
        count = size;
    }
    if (offset >= size || count > size) {
        return DSERR_INVALIDPARAM;
    }
    DWORD first = std::min(count, size - offset);
    *audio1 = bytes.data() + offset;
    *bytes1 = first;
    if (audio2 != nullptr) {
        *audio2 = count > first ? bytes.data() : nullptr;
        *bytes2 = count - first;
    }
    return DS_OK;
}

HRESULT IDirectSoundBuffer::Play(DWORD reserved, DWORD priority, DWORD flags) {
    std::lock_guard lock(GetMixer().mutex);
    playing = true;
    looping = (flags & DSBPLAY_LOOPING) != 0;
    return DS_OK;
}

HRESULT IDirectSoundBuffer::Stop() {
    std::lock_guard lock(GetMixer().mutex);
    playing = false;
    return DS_OK;
}

HRESULT IDirectSoundBuffer::SetVolume(LONG value) {
    std::lock_guard lock(GetMixer().mutex);
    volume = std::clamp(value, LONG{DSBVOLUME_MIN}, LONG{DSBVOLUME_MAX});
    return DS_OK;
}

HRESULT IDirectSoundBuffer::SetPan(LONG value) {
    std::lock_guard lock(GetMixer().mutex);
    pan = std::clamp(value, LONG{DSBPAN_LEFT}, LONG{DSBPAN_RIGHT});
    return DS_OK;
}

HRESULT IDirectSoundBuffer::SetFrequency(DWORD value) {
    std::lock_guard lock(GetMixer().mutex);
    frequency = value != 0 ? value : format.nSamplesPerSec;
    return DS_OK;
}

HRESULT IDirectSoundBuffer::GetStatus(DWORD *status) {
    std::lock_guard lock(GetMixer().mutex);
    *status = (playing ? DSBSTATUS_PLAYING : 0) | (playing && looping ? DSBSTATUS_LOOPING : 0);
    return DS_OK;
}

HRESULT IDirectSoundBuffer::SetCurrentPosition(DWORD value) {
    std::lock_guard lock(GetMixer().mutex);
    position = format.nBlockAlign != 0 ? static_cast<double>(value / format.nBlockAlign) : 0.0;
    return DS_OK;
}

HRESULT IDirectSoundBuffer::GetCurrentPosition(DWORD *play, DWORD *write) {
    std::lock_guard lock(GetMixer().mutex);
    auto byte = static_cast<DWORD>(position) * format.nBlockAlign;
    if (play != nullptr) {
        *play = byte;
    }
    if (write != nullptr) {
        *write = byte;
    }
    return DS_OK;
}

HRESULT IDirectSoundBuffer::GetCaps(DSBCAPS *caps) {
    caps->dwFlags = (primary ? DSBCAPS_PRIMARYBUFFER : DSBCAPS_STATIC) | DSBCAPS_LOCSOFTWARE |
                    DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME;
    caps->dwBufferBytes = static_cast<DWORD>(bytes.size());
    caps->dwUnlockTransferRate = 0;
    caps->dwPlayCpuOverhead = 0;
    return DS_OK;
}

HRESULT IDirectSoundBuffer::GetFormat(WAVEFORMATEX *destination, DWORD size, DWORD *written) {
    DWORD count = std::min<DWORD>(size, sizeof(format));
    if (destination != nullptr) {
        std::memcpy(destination, &format, count);
    }
    if (written != nullptr) {
        *written = destination != nullptr ? count : static_cast<DWORD>(sizeof(format));
    }
    return DS_OK;
}

HRESULT IDirectSoundBuffer::SetFormat(const WAVEFORMATEX *value) {
    format = *value;
    format.cbSize = 0;
    return DS_OK;
}

ULONG IDirectSoundBuffer::Release() {
    Mixer &mixer = GetMixer();
    {
        std::lock_guard lock(mixer.mutex);
        std::erase(mixer.buffers, this);
    }
    delete this;
    return 0;
}

HRESULT IDirectSound::Initialize(const GUID *device) {
    Mixer &mixer = GetMixer();
    if (mixer.stream != nullptr) {
        return DS_OK;
    }
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO, "The sound effects are silent: %s", SDL_GetError());
        return E_FAIL;
    }
    SDL_AudioSpec spec{SDL_AUDIO_F32, kMixChannels, kMixRate};
    mixer.stream =
        SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, MixAudio, &mixer);
    if (mixer.stream == nullptr) {
        SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO, "The sound effects are silent: %s", SDL_GetError());
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return E_FAIL;
    }
    SDL_ResumeAudioStreamDevice(mixer.stream);
    return DS_OK;
}

HRESULT IDirectSound::CreateSoundBuffer(const DSBUFFERDESC *description,
                                        IDirectSoundBuffer **buffer,
                                        void *outer) {
    *buffer = nullptr;
    auto *created = new IDirectSoundBuffer;
    if (description->dwFlags & DSBCAPS_PRIMARYBUFFER) {
        created->primary = true;
    } else {
        const WAVEFORMATEX *format = description->lpwfxFormat;
        if (format == nullptr || description->dwBufferBytes == 0) {
            delete created;
            return DSERR_INVALIDPARAM;
        }
        if (format->wFormatTag != 1 ||
            (format->wBitsPerSample != 8 && format->wBitsPerSample != 16) ||
            format->nChannels == 0 || format->nChannels > 2 ||
            format->nBlockAlign != format->nChannels * format->wBitsPerSample / 8) {
            delete created;
            return DSERR_BADFORMAT;
        }
        created->format = *format;
        created->format.cbSize = 0;
        created->frequency = format->nSamplesPerSec;
        created->bytes.assign(description->dwBufferBytes,
                              format->wBitsPerSample == 8 ? std::uint8_t{0x80} : std::uint8_t{0});
    }
    Mixer &mixer = GetMixer();
    std::lock_guard lock(mixer.mutex);
    mixer.buffers.push_back(created);
    *buffer = created;
    return DS_OK;
}

ULONG IDirectSound::Release() {
    Mixer &mixer = GetMixer();
    if (mixer.stream != nullptr) {
        SDL_DestroyAudioStream(mixer.stream);
        mixer.stream = nullptr;
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }
    return 0;
}

HRESULT CoCreateInstance(
    const GUID &classId, void *outer, DWORD context, const GUID &interfaceId, void **object) {
    *object = &g_directSound;
    return S_OK;
}
