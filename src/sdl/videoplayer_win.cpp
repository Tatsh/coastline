#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#include <SDL3/SDL.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <wrl/client.h>

#include "paths.h"
#include "scenerenderer.h"
#include "videoplayer.h"

namespace coastline {

using Microsoft::WRL::ComPtr;

struct VideoPlayer::State {
    std::unique_ptr<RenderTexture> texture;
    ComPtr<IMFSourceReader> reader;
    ComPtr<IMFSample> pending;
    SDL_AudioStream *audio = nullptr;
    std::vector<std::uint8_t> rgba;
    LONGLONG pendingTime = 0;
    double duration = 0;
    int stride = 0;
    DWORD videoIndex = MF_SOURCE_READER_INVALID_STREAM_INDEX;
    DWORD audioIndex = MF_SOURCE_READER_INVALID_STREAM_INDEX;
    bool paused = false;
    bool comInitialized = false;
    bool mediaInitialized = false;
    bool ended = false;

    ~State() {
        SDL_DestroyAudioStream(audio);
        reader.Reset();
        if (mediaInitialized) {
            MFShutdown();
        }
        if (comInitialized) {
            CoUninitialize();
        }
    }

    void UploadFrame() {
        if (!paused) {
            texture->Update(rgba);
            return;
        }
        auto grey = rgba;
        for (std::size_t index = 0; index < grey.size(); index += 4) {
            const auto shade = static_cast<std::uint8_t>(
                (77 * grey[index] + 150 * grey[index + 1] + 29 * grey[index + 2]) / 512);
            grey[index] = grey[index + 1] = grey[index + 2] = shade;
        }
        texture->Update(grey);
    }

    bool Video(IMFSample *sample) {
        ComPtr<IMFMediaBuffer> buffer;
        if (FAILED(sample->ConvertToContiguousBuffer(&buffer))) {
            return false;
        }
        BYTE *data = nullptr;
        DWORD length = 0;
        if (FAILED(buffer->Lock(&data, nullptr, &length))) {
            return false;
        }
        const int width = texture->Width();
        const int height = texture->Height();
        const auto pitch = static_cast<std::uint64_t>(std::abs(stride));
        const bool valid = pitch >= static_cast<std::uint64_t>(width) * 4 &&
                           static_cast<std::uint64_t>(length) >= pitch * height;
        if (valid) {
            for (int y = 0; y < height; ++y) {
                const auto *row = data + (stride < 0 ? height - 1 - y : y) * pitch;
                for (int x = 0; x < width; ++x) {
                    auto *pixel = rgba.data() + (y * width + x) * 4;
                    pixel[0] = row[x * 4 + 2];
                    pixel[1] = row[x * 4 + 1];
                    pixel[2] = row[x * 4];
                    pixel[3] = 255;
                }
            }
        }
        buffer->Unlock();
        if (valid) {
            UploadFrame();
        }
        return valid;
    }

    void Audio(IMFSample *sample, float volume) {
        if (!audio || volume <= 0) {
            return;
        }
        ComPtr<IMFMediaBuffer> buffer;
        if (FAILED(sample->ConvertToContiguousBuffer(&buffer))) {
            return;
        }
        BYTE *data = nullptr;
        DWORD length = 0;
        if (SUCCEEDED(buffer->Lock(&data, nullptr, &length))) {
            SDL_PutAudioStreamData(audio, data, static_cast<int>(length));
            buffer->Unlock();
        }
    }
};

VideoPlayer::VideoPlayer(std::unique_ptr<State> state) : m_state(std::move(state)) {
}

VideoPlayer::~VideoPlayer() = default;

std::unique_ptr<VideoPlayer> VideoPlayer::Open(SceneRenderer &renderer,
                                               const std::filesystem::path &path) {
    auto state = std::make_unique<State>();
    const HRESULT com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(com) && com != RPC_E_CHANGED_MODE) {
        return nullptr;
    }
    state->comInitialized = SUCCEEDED(com);
    if (FAILED(MFStartup(MF_VERSION))) {
        return nullptr;
    }
    state->mediaInitialized = true;
    ComPtr<IMFAttributes> attributes;
    if (FAILED(MFCreateAttributes(&attributes, 1)) ||
        FAILED(attributes->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE)) ||
        FAILED(MFCreateSourceReaderFromURL(path.c_str(), attributes.Get(), &state->reader))) {
        return nullptr;
    }
    for (DWORD index = 0; index < 64; ++index) {
        ComPtr<IMFMediaType> native;
        if (FAILED(state->reader->GetNativeMediaType(index, 0, &native))) {
            break;
        }
        GUID major;
        if (SUCCEEDED(native->GetGUID(MF_MT_MAJOR_TYPE, &major))) {
            if (major == MFMediaType_Video &&
                state->videoIndex == MF_SOURCE_READER_INVALID_STREAM_INDEX) {
                state->videoIndex = index;
            } else if (major == MFMediaType_Audio &&
                       state->audioIndex == MF_SOURCE_READER_INVALID_STREAM_INDEX) {
                state->audioIndex = index;
            }
        }
    }
    if (state->videoIndex == MF_SOURCE_READER_INVALID_STREAM_INDEX) {
        return nullptr;
    }
    ComPtr<IMFMediaType> type;
    if (FAILED(MFCreateMediaType(&type)) ||
        FAILED(type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video)) ||
        FAILED(type->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32)) ||
        FAILED(state->reader->SetCurrentMediaType(
            MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, type.Get())) ||
        FAILED(state->reader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, &type))) {
        return nullptr;
    }
    UINT32 width = 0;
    UINT32 height = 0;
    if (FAILED(MFGetAttributeSize(type.Get(), MF_MT_FRAME_SIZE, &width, &height)) || width == 0 ||
        height == 0 || width > 4096 || height > 4096) {
        return nullptr;
    }
    UINT32 rawStride = 0;
    if (FAILED(type->GetUINT32(MF_MT_DEFAULT_STRIDE, &rawStride))) {
        LONG fallback = 0;
        if (FAILED(MFGetStrideForBitmapInfoHeader(MFVideoFormat_RGB32.Data1, width, &fallback))) {
            return nullptr;
        }
        state->stride = fallback;
    } else {
        state->stride = static_cast<std::int32_t>(rawStride);
    }
    state->rgba.resize(static_cast<std::size_t>(width) * height * 4);
    state->texture = RenderTexture::FromTexels(renderer, width, height, state->rgba, true, false);
    if (!state->texture) {
        return nullptr;
    }
    UINT64 duration = 0;
    PROPVARIANT property;
    PropVariantInit(&property);
    if (SUCCEEDED(state->reader->GetPresentationAttribute(
            MF_SOURCE_READER_MEDIASOURCE, MF_PD_DURATION, &property)) &&
        property.vt == VT_UI8) {
        duration = property.uhVal.QuadPart;
    }
    PropVariantClear(&property);
    state->duration = static_cast<double>(duration) / 10000000.0;
    if (SDL_WasInit(SDL_INIT_AUDIO)) {
        type.Reset();
        if (SUCCEEDED(MFCreateMediaType(&type)) &&
            SUCCEEDED(type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio)) &&
            SUCCEEDED(type->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM)) &&
            SUCCEEDED(type->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, 2)) &&
            SUCCEEDED(type->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 48000)) &&
            SUCCEEDED(type->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16)) &&
            SUCCEEDED(type->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 4)) &&
            SUCCEEDED(type->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 192000)) &&
            SUCCEEDED(state->reader->SetCurrentMediaType(
                MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, type.Get()))) {
            const SDL_AudioSpec spec{SDL_AUDIO_S16, 2, 48000};
            state->audio = SDL_OpenAudioDeviceStream(
                SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
            if (state->audio) {
                SDL_ResumeAudioStreamDevice(state->audio);
            }
        }
    }
    SDL_Log("Playing intro movie %s.", Utf8FromPath(path.filename()).c_str());
    return std::unique_ptr<VideoPlayer>(new VideoPlayer(std::move(state)));
}

bool VideoPlayer::Update(double seconds, float volume) {
    State &state = *m_state;
    if (!std::isfinite(seconds) || seconds < 0) {
        return true;
    }
    if (state.audio) {
        SDL_SetAudioStreamGain(state.audio, std::clamp(volume, 0.0f, 1.0f));
        if (volume <= 0) {
            SDL_ClearAudioStream(state.audio);
        }
    }
    for (int work = 0; work < 256; ++work) {
        if (state.pending) {
            if (static_cast<double>(state.pendingTime) / 10000000.0 > seconds) {
                return true;
            }
            if (!state.Video(state.pending.Get())) {
                return false;
            }
            state.pending.Reset();
        }
        DWORD stream = 0;
        DWORD flags = 0;
        LONGLONG time = 0;
        ComPtr<IMFSample> sample;
        if (FAILED(state.reader->ReadSample(
                MF_SOURCE_READER_ANY_STREAM, 0, &stream, &flags, &time, &sample))) {
            return false;
        }
        if ((flags & MF_SOURCE_READERF_ENDOFSTREAM) && stream == state.videoIndex) {
            state.ended = true;
        }
        if (!sample) {
            if (state.ended) {
                return state.audio && SDL_GetAudioStreamQueued(state.audio) > 0;
            }
            continue;
        }
        if (stream == state.videoIndex) {
            state.pending = sample;
            state.pendingTime = time;
        } else if (stream == state.audioIndex) {
            state.Audio(sample.Get(), volume);
        }
    }
    return true;
}

std::optional<double> VideoPlayer::Seek(double seconds) {
    if (!std::isfinite(seconds)) {
        return std::nullopt;
    }
    State &state = *m_state;
    seconds =
        std::clamp(seconds,
                   0.0,
                   state.duration > 0 ? std::clamp(state.duration - 0.05, 0.0, 86400.0) : 86400.0);
    PROPVARIANT position;
    PropVariantInit(&position);
    position.vt = VT_I8;
    position.hVal.QuadPart = static_cast<LONGLONG>(seconds * 10000000.0);
    if (FAILED(state.reader->SetCurrentPosition(GUID_NULL, position))) {
        return std::nullopt;
    }
    state.pending.Reset();
    state.ended = false;
    if (state.audio) {
        SDL_ClearAudioStream(state.audio);
    }
    return seconds;
}

void VideoPlayer::SetPaused(bool paused) {
    State &state = *m_state;
    state.paused = paused;
    state.UploadFrame();
    if (state.audio) {
        if (paused) {
            SDL_PauseAudioStreamDevice(state.audio);
        } else {
            SDL_ResumeAudioStreamDevice(state.audio);
        }
    }
}

double VideoPlayer::Duration() const {
    return m_state->duration;
}

const RenderTexture *VideoPlayer::Texture() const {
    return m_state->texture.get();
}

} // namespace coastline
