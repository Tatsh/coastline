#include "videoplayer.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#include <SDL3/SDL.h>

#include "paths.h"
#include "scenerenderer.h"

#if COASTLINE_VIDEO
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}
#endif

namespace coastline {

struct VideoPlayer::State {
    std::unique_ptr<RenderTexture> texture;
#if COASTLINE_VIDEO
    AVFormatContext *format = nullptr;
    AVCodecContext *video = nullptr;
    AVCodecContext *audio = nullptr;
    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    SwsContext *scale = nullptr;
    SwrContext *resample = nullptr;
    SDL_AudioStream *stream = nullptr;
    int videoIndex = -1;
    int audioIndex = -1;
    bool pending = false;
    bool eof = false;
    bool drained = false;
    double frameTime = 0;
    std::vector<std::uint8_t> rgba;
    std::vector<float> samples;

    ~State() {
        SDL_DestroyAudioStream(stream);
        swr_free(&resample);
        sws_freeContext(scale);
        av_frame_free(&frame);
        av_packet_free(&packet);
        avcodec_free_context(&audio);
        avcodec_free_context(&video);
        avformat_close_input(&format);
    }

    AVCodecContext *Decoder(int index) {
        const auto *parameters = format->streams[index]->codecpar;
        const auto *codec = avcodec_find_decoder(parameters->codec_id);
        auto *context = codec ? avcodec_alloc_context3(codec) : nullptr;
        if (!context) {
            return nullptr;
        }
        context->thread_count = 2;
        if (avcodec_parameters_to_context(context, parameters) < 0 ||
            avcodec_open2(context, codec, nullptr) < 0) {
            avcodec_free_context(&context);
        }
        return context;
    }

    void Audio(float volume, bool finish = false) {
        if (!audio || !stream || volume <= 0) {
            return;
        }
        if (avcodec_send_packet(audio, finish ? nullptr : packet) < 0) {
            return;
        }
        auto *decoded = av_frame_alloc();
        if (!decoded) {
            return;
        }
        while (avcodec_receive_frame(audio, decoded) == 0) {
            const int capacity = swr_get_out_samples(resample, decoded->nb_samples);
            if (capacity <= 0 || capacity > 48000 * 4) {
                break;
            }
            samples.resize(static_cast<std::size_t>(capacity) * 2);
            auto *destination = reinterpret_cast<std::uint8_t *>(samples.data());
            const int count = swr_convert(resample,
                                          &destination,
                                          capacity,
                                          const_cast<const std::uint8_t **>(decoded->extended_data),
                                          decoded->nb_samples);
            if (count > 0) {
                SDL_PutAudioStreamData(stream, samples.data(), count * 2 * sizeof(float));
            }
            av_frame_unref(decoded);
        }
        av_frame_free(&decoded);
    }
#endif
};

VideoPlayer::VideoPlayer(std::unique_ptr<State> state) : m_state(std::move(state)) {
}

VideoPlayer::~VideoPlayer() = default;

std::unique_ptr<VideoPlayer> VideoPlayer::Open(SceneRenderer &renderer,
                                               const std::filesystem::path &path) {
#if COASTLINE_VIDEO
    auto state = std::make_unique<State>();
    if (!state->packet || !state->frame ||
        avformat_open_input(&state->format, Utf8FromPath(path).c_str(), nullptr, nullptr) < 0 ||
        avformat_find_stream_info(state->format, nullptr) < 0) {
        return nullptr;
    }
    state->videoIndex = av_find_best_stream(state->format, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (state->videoIndex < 0 || !(state->video = state->Decoder(state->videoIndex))) {
        return nullptr;
    }
    const int width = state->video->width;
    const int height = state->video->height;
    if (width <= 0 || height <= 0 || width > 4096 || height > 4096) {
        return nullptr;
    }
    state->rgba.resize(static_cast<std::size_t>(width) * height * 4);
    state->texture = RenderTexture::FromTexels(renderer, width, height, state->rgba, true, false);
    if (!state->texture) {
        return nullptr;
    }
    state->audioIndex = av_find_best_stream(state->format, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (state->audioIndex >= 0 && SDL_WasInit(SDL_INIT_AUDIO) &&
        (state->audio = state->Decoder(state->audioIndex))) {
        AVChannelLayout stereo = AV_CHANNEL_LAYOUT_STEREO;
        if (swr_alloc_set_opts2(&state->resample,
                                &stereo,
                                AV_SAMPLE_FMT_FLT,
                                48000,
                                &state->audio->ch_layout,
                                state->audio->sample_fmt,
                                state->audio->sample_rate,
                                0,
                                nullptr) >= 0 &&
            swr_init(state->resample) >= 0) {
            const SDL_AudioSpec spec{SDL_AUDIO_F32, 2, 48000};
            state->stream = SDL_OpenAudioDeviceStream(
                SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
            if (state->stream) {
                SDL_ResumeAudioStreamDevice(state->stream);
            }
        }
    }
    SDL_Log("Playing intro movie %s.", Utf8FromPath(path.filename()).c_str());
    return std::unique_ptr<VideoPlayer>(new VideoPlayer(std::move(state)));
#else
    (void)renderer;
    (void)path;
    return nullptr;
#endif
}

bool VideoPlayer::Update(double seconds, float volume) {
#if COASTLINE_VIDEO
    State &s = *m_state;
    if (!std::isfinite(seconds) || seconds < 0) {
        return true;
    }
    if (s.stream) {
        SDL_SetAudioStreamGain(s.stream, std::clamp(volume, 0.0f, 1.0f));
        if (volume <= 0) {
            SDL_ClearAudioStream(s.stream);
        }
    }
    // A pending frame bounds decoding to the playback clock.
    for (int work = 0; work < 512; ++work) {
        if (s.pending) {
            if (s.frameTime > seconds) {
                return true;
            }
            s.scale = sws_getCachedContext(s.scale,
                                           s.frame->width,
                                           s.frame->height,
                                           static_cast<AVPixelFormat>(s.frame->format),
                                           s.texture->Width(),
                                           s.texture->Height(),
                                           AV_PIX_FMT_RGBA,
                                           SWS_BILINEAR,
                                           nullptr,
                                           nullptr,
                                           nullptr);
            if (!s.scale) {
                return false;
            }
            std::uint8_t *destination[] = {s.rgba.data()};
            int stride[] = {s.texture->Width() * 4};
            sws_scale(
                s.scale, s.frame->data, s.frame->linesize, 0, s.frame->height, destination, stride);
            s.texture->Update(s.rgba);
            av_frame_unref(s.frame);
            s.pending = false;
        }
        const int received = avcodec_receive_frame(s.video, s.frame);
        if (received == 0) {
            const auto *track = s.format->streams[s.videoIndex];
            const auto timestamp = s.frame->best_effort_timestamp;
            const auto start = track->start_time == AV_NOPTS_VALUE ? 0 : track->start_time;
            s.frameTime = timestamp == AV_NOPTS_VALUE ?
                              seconds :
                              (timestamp - start) * av_q2d(track->time_base);
            s.pending = true;
            continue;
        }
        if (received != AVERROR(EAGAIN) && received != AVERROR_EOF) {
            return false;
        }
        if (s.eof) {
            if (!s.drained) {
                s.Audio(volume, true);
                if (s.stream) {
                    SDL_FlushAudioStream(s.stream);
                }
                avcodec_send_packet(s.video, nullptr);
                s.drained = true;
                continue;
            }
            return s.stream && SDL_GetAudioStreamQueued(s.stream) > 0;
        }
        if (av_read_frame(s.format, s.packet) < 0) {
            s.eof = true;
            continue;
        }
        if (s.packet->stream_index == s.videoIndex) {
            avcodec_send_packet(s.video, s.packet);
        } else if (s.packet->stream_index == s.audioIndex) {
            s.Audio(volume);
        }
        av_packet_unref(s.packet);
    }
    return true;
#else
    (void)seconds;
    (void)volume;
    return false;
#endif
}

const RenderTexture *VideoPlayer::Texture() const {
    return m_state->texture.get();
}

} // namespace coastline
