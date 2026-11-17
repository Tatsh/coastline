#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/audio_fifo.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

#include "importer.h"
#include "paths.h"

namespace coastline {

namespace {

class Stream {
public:
    Stream() = default;
    Stream(const Stream &) = delete;
    Stream &operator=(const Stream &) = delete;

    ~Stream() {
        av_audio_fifo_free(m_fifo);
        swr_free(&m_resample);
        sws_freeContext(m_scale);
        av_frame_free(&m_converted);
        av_frame_free(&m_decoded);
        avcodec_free_context(&m_encoder);
        avcodec_free_context(&m_decoder);
    }

    bool OpenDecoder(AVFormatContext *input, AVMediaType type) {
        const int index = av_find_best_stream(input, type, -1, -1, nullptr, 0);
        if (index < 0) {
            return false;
        }
        m_input = input->streams[index];
        const auto *codec = avcodec_find_decoder(m_input->codecpar->codec_id);
        m_decoder = codec ? avcodec_alloc_context3(codec) : nullptr;
        return m_decoder && avcodec_parameters_to_context(m_decoder, m_input->codecpar) >= 0 &&
               avcodec_open2(m_decoder, codec, nullptr) >= 0;
    }

    bool OpenEncoder(AVFormatContext *output, AVMediaType type) {
        const auto *codec = type == AVMEDIA_TYPE_VIDEO ?
                                avcodec_find_encoder_by_name("libopenh264") :
                                avcodec_find_encoder(AV_CODEC_ID_AAC);
        if (type == AVMEDIA_TYPE_VIDEO && !codec) {
            codec = avcodec_find_encoder_by_name("libx264");
        }
        m_encoder = codec ? avcodec_alloc_context3(codec) : nullptr;
        m_output = avformat_new_stream(output, nullptr);
        if (!m_encoder || !m_output) {
            return false;
        }
        if (type == AVMEDIA_TYPE_VIDEO) {
            const AVRational rate =
                m_input->avg_frame_rate.num > 0 ? m_input->avg_frame_rate : AVRational{15, 1};
            m_encoder->width = m_decoder->width;
            m_encoder->height = m_decoder->height;
            m_encoder->pix_fmt = AV_PIX_FMT_YUV420P;
            m_encoder->time_base = av_inv_q(rate);
            m_encoder->framerate = rate;
            m_encoder->bit_rate = 2000000;
            m_encoder->gop_size = 30;
        } else {
            m_encoder->sample_rate = 48000;
            av_channel_layout_default(&m_encoder->ch_layout, 2);
            m_encoder->sample_fmt = AV_SAMPLE_FMT_FLTP;
            m_encoder->time_base = AVRational{1, m_encoder->sample_rate};
            m_encoder->bit_rate = 192000;
        }
        if (output->oformat->flags & AVFMT_GLOBALHEADER) {
            m_encoder->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }
        if (avcodec_open2(m_encoder, codec, nullptr) < 0 ||
            avcodec_parameters_from_context(m_output->codecpar, m_encoder) < 0) {
            return false;
        }
        m_output->time_base = m_encoder->time_base;
        return true;
    }

    bool OpenResampler() {
        if (swr_alloc_set_opts2(&m_resample,
                                &m_encoder->ch_layout,
                                m_encoder->sample_fmt,
                                m_encoder->sample_rate,
                                &m_decoder->ch_layout,
                                m_decoder->sample_fmt,
                                m_decoder->sample_rate,
                                0,
                                nullptr) < 0 ||
            swr_init(m_resample) < 0) {
            return false;
        }
        m_fifo = av_audio_fifo_alloc(m_encoder->sample_fmt, 2, 1);
        return m_fifo != nullptr;
    }

    bool Decode(AVFormatContext *output, AVPacket *packet, bool video) {
        if (avcodec_send_packet(m_decoder, packet) < 0) {
            return false;
        }
        for (;;) {
            const int result = avcodec_receive_frame(m_decoder, m_decoded);
            if (result == AVERROR(EAGAIN) || result == AVERROR_EOF) {
                return true;
            }
            if (result < 0) {
                return false;
            }
            const bool okay = video ? ConvertVideo(output) : ConvertAudio(output);
            av_frame_unref(m_decoded);
            if (!okay) {
                return false;
            }
        }
    }

    bool WriteAudio(AVFormatContext *output, bool final) {
        const int frameSize = m_encoder->frame_size;
        while (av_audio_fifo_size(m_fifo) >= frameSize ||
               (final && av_audio_fifo_size(m_fifo) > 0)) {
            auto *frame = m_converted;
            av_frame_unref(frame);
            frame->format = m_encoder->sample_fmt;
            frame->sample_rate = m_encoder->sample_rate;
            frame->nb_samples = frameSize;
            if (av_channel_layout_copy(&frame->ch_layout, &m_encoder->ch_layout) < 0 ||
                av_frame_get_buffer(frame, 0) < 0) {
                return false;
            }
            const int available = std::min(frameSize, av_audio_fifo_size(m_fifo));
            if (av_audio_fifo_read(m_fifo, reinterpret_cast<void **>(frame->data), available) !=
                available) {
                return false;
            }
            if (available < frameSize) {
                av_samples_set_silence(
                    frame->data, available, frameSize - available, 2, m_encoder->sample_fmt);
            }
            frame->pts = m_audioSamples;
            m_audioSamples += frameSize;
            if (!WritePackets(output, frame)) {
                return false;
            }
        }
        return true;
    }

    bool Flush(AVFormatContext *output) {
        return WritePackets(output, nullptr);
    }

    int Index() const {
        return m_input->index;
    }

    AVRational TimeBase() const {
        return m_input->time_base;
    }

private:
    bool WritePackets(AVFormatContext *output, AVFrame *frame) {
        if (avcodec_send_frame(m_encoder, frame) < 0) {
            return false;
        }
        AVPacket *packet = av_packet_alloc();
        if (!packet) {
            return false;
        }
        bool okay = true;
        for (;;) {
            const int result = avcodec_receive_packet(m_encoder, packet);
            if (result == AVERROR(EAGAIN) || result == AVERROR_EOF) {
                break;
            }
            if (result < 0) {
                okay = false;
                break;
            }
            av_packet_rescale_ts(packet, m_encoder->time_base, m_output->time_base);
            packet->stream_index = m_output->index;
            if (av_interleaved_write_frame(output, packet) < 0) {
                okay = false;
                break;
            }
            av_packet_unref(packet);
        }
        av_packet_free(&packet);
        return okay;
    }

    bool ConvertVideo(AVFormatContext *output) {
        auto *frame = m_converted;
        av_frame_unref(frame);
        frame->format = m_encoder->pix_fmt;
        frame->width = m_encoder->width;
        frame->height = m_encoder->height;
        if (av_frame_get_buffer(frame, 32) < 0) {
            return false;
        }
        m_scale = sws_getCachedContext(m_scale,
                                       m_decoder->width,
                                       m_decoder->height,
                                       static_cast<AVPixelFormat>(m_decoded->format),
                                       frame->width,
                                       frame->height,
                                       m_encoder->pix_fmt,
                                       SWS_BILINEAR,
                                       nullptr,
                                       nullptr,
                                       nullptr);
        if (!m_scale || sws_scale(m_scale,
                                  m_decoded->data,
                                  m_decoded->linesize,
                                  0,
                                  m_decoded->height,
                                  frame->data,
                                  frame->linesize) <= 0) {
            return false;
        }
        frame->pts = m_decoded->best_effort_timestamp == AV_NOPTS_VALUE ?
                         0 :
                         av_rescale_q(m_decoded->best_effort_timestamp,
                                      m_input->time_base,
                                      m_encoder->time_base);
        return WritePackets(output, frame);
    }

    bool ConvertAudio(AVFormatContext *output) {
        const int capacity = swr_get_out_samples(m_resample, m_decoded->nb_samples);
        if (capacity <= 0 || capacity > 48000 * 4) {
            return false;
        }
        AVFrame *frame = av_frame_alloc();
        if (!frame) {
            return false;
        }
        frame->format = m_encoder->sample_fmt;
        frame->sample_rate = m_encoder->sample_rate;
        frame->nb_samples = capacity;
        bool okay = av_channel_layout_copy(&frame->ch_layout, &m_encoder->ch_layout) >= 0 &&
                    av_frame_get_buffer(frame, 0) >= 0;
        if (okay) {
            const int count =
                swr_convert(m_resample,
                            frame->data,
                            capacity,
                            const_cast<const std::uint8_t **>(m_decoded->extended_data),
                            m_decoded->nb_samples);
            okay = count >= 0 &&
                   av_audio_fifo_realloc(m_fifo, av_audio_fifo_size(m_fifo) + count) >= 0 &&
                   av_audio_fifo_write(m_fifo, reinterpret_cast<void **>(frame->data), count) ==
                       count &&
                   WriteAudio(output, false);
        }
        av_frame_free(&frame);
        return okay;
    }

    AVCodecContext *m_decoder = nullptr;
    AVCodecContext *m_encoder = nullptr;
    AVStream *m_input = nullptr;
    AVStream *m_output = nullptr;
    AVFrame *m_decoded = av_frame_alloc();
    AVFrame *m_converted = av_frame_alloc();
    SwsContext *m_scale = nullptr;
    SwrContext *m_resample = nullptr;
    AVAudioFifo *m_fifo = nullptr;
    std::int64_t m_audioSamples = 0;
};

class Context {
public:
    Context() = default;
    Context(const Context &) = delete;
    Context &operator=(const Context &) = delete;

    ~Context() {
        av_packet_free(&m_packet);
        if (m_output) {
            if (m_output->pb) {
                avio_closep(&m_output->pb);
            }
            avformat_free_context(m_output);
        }
        avformat_close_input(&m_input);
    }

    std::optional<std::string> Convert(const std::filesystem::path &source,
                                       const std::filesystem::path &output,
                                       const ImportProgress &progress) {
        if (!m_packet ||
            avformat_open_input(&m_input, Utf8FromPath(source).c_str(), nullptr, nullptr) < 0 ||
            avformat_find_stream_info(m_input, nullptr) < 0 ||
            !m_video.OpenDecoder(m_input, AVMEDIA_TYPE_VIDEO) ||
            avformat_alloc_output_context2(
                &m_output, nullptr, "mp4", Utf8FromPath(output).c_str()) < 0 ||
            !m_output || !m_video.OpenEncoder(m_output, AVMEDIA_TYPE_VIDEO)) {
            return "Cannot prepare libav intro movie conversion.";
        }
        const bool hasAudio = m_audio.OpenDecoder(m_input, AVMEDIA_TYPE_AUDIO);
        if (hasAudio &&
            (!m_audio.OpenEncoder(m_output, AVMEDIA_TYPE_AUDIO) || !m_audio.OpenResampler())) {
            return "Cannot prepare libav intro movie audio.";
        }
        if (avio_open(&m_output->pb, Utf8FromPath(output).c_str(), AVIO_FLAG_WRITE) < 0 ||
            avformat_write_header(m_output, nullptr) < 0) {
            return "Cannot open converted intro movie.";
        }
        while (av_read_frame(m_input, m_packet) >= 0) {
            Stream *stream = m_packet->stream_index == m_video.Index()             ? &m_video :
                             hasAudio && m_packet->stream_index == m_audio.Index() ? &m_audio :
                                                                                     nullptr;
            if (stream && !stream->Decode(m_output, m_packet, stream == &m_video)) {
                return "Cannot decode or encode intro movie.";
            }
            if (progress && m_input->duration > 0 && m_packet->pts != AV_NOPTS_VALUE && stream &&
                !progress("Converting the intro movie...",
                          std::clamp(m_packet->pts * av_q2d(stream->TimeBase()) * 1000000.0 /
                                         m_input->duration,
                                     0.0,
                                     1.0))) {
                return "Import cancelled.";
            }
            av_packet_unref(m_packet);
        }
        if (!m_video.Decode(m_output, nullptr, true) ||
            (hasAudio &&
             (!m_audio.Decode(m_output, nullptr, false) || !m_audio.WriteAudio(m_output, true))) ||
            !m_video.Flush(m_output) || (hasAudio && !m_audio.Flush(m_output)) ||
            av_write_trailer(m_output) < 0) {
            return "Cannot finish converted intro movie.";
        }
        return std::nullopt;
    }

private:
    AVFormatContext *m_input = nullptr;
    AVFormatContext *m_output = nullptr;
    AVPacket *m_packet = av_packet_alloc();
    Stream m_video;
    Stream m_audio;
};

} // namespace

std::optional<std::string> ConvertIntroMovieWithLibav(const std::filesystem::path &source,
                                                      const std::filesystem::path &output,
                                                      const ImportProgress &progress) {
    Context context;
    return context.Convert(source, output, progress);
}

} // namespace coastline
