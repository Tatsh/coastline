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

struct Stream {
    AVCodecContext *decoder = nullptr;
    AVCodecContext *encoder = nullptr;
    AVStream *input = nullptr;
    AVStream *output = nullptr;
    AVFrame *decoded = av_frame_alloc();
    AVFrame *converted = av_frame_alloc();
    SwsContext *scale = nullptr;
    SwrContext *resample = nullptr;
    AVAudioFifo *fifo = nullptr;
    std::int64_t audioSamples = 0;

    ~Stream() {
        av_audio_fifo_free(fifo);
        swr_free(&resample);
        sws_freeContext(scale);
        av_frame_free(&converted);
        av_frame_free(&decoded);
        avcodec_free_context(&encoder);
        avcodec_free_context(&decoder);
    }
};

struct Context {
    AVFormatContext *input = nullptr;
    AVFormatContext *output = nullptr;
    AVPacket *packet = av_packet_alloc();
    Stream video;
    Stream audio;

    ~Context() {
        av_packet_free(&packet);
        if (output) {
            if (output->pb) {
                avio_closep(&output->pb);
            }
            avformat_free_context(output);
        }
        avformat_close_input(&input);
    }
};

bool OpenDecoder(AVFormatContext *input, AVMediaType type, Stream &stream) {
    const int index = av_find_best_stream(input, type, -1, -1, nullptr, 0);
    if (index < 0) {
        return false;
    }
    stream.input = input->streams[index];
    const auto *codec = avcodec_find_decoder(stream.input->codecpar->codec_id);
    stream.decoder = codec ? avcodec_alloc_context3(codec) : nullptr;
    return stream.decoder &&
           avcodec_parameters_to_context(stream.decoder, stream.input->codecpar) >= 0 &&
           avcodec_open2(stream.decoder, codec, nullptr) >= 0;
}

bool OpenEncoder(AVFormatContext *output, Stream &stream, AVMediaType type) {
    const auto *codec = type == AVMEDIA_TYPE_VIDEO ? avcodec_find_encoder_by_name("libopenh264") :
                                                     avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (type == AVMEDIA_TYPE_VIDEO && !codec) {
        codec = avcodec_find_encoder_by_name("libx264");
    }
    stream.encoder = codec ? avcodec_alloc_context3(codec) : nullptr;
    stream.output = avformat_new_stream(output, nullptr);
    if (!stream.encoder || !stream.output) {
        return false;
    }
    if (type == AVMEDIA_TYPE_VIDEO) {
        const AVRational rate =
            stream.input->avg_frame_rate.num > 0 ? stream.input->avg_frame_rate : AVRational{15, 1};
        stream.encoder->width = stream.decoder->width;
        stream.encoder->height = stream.decoder->height;
        stream.encoder->pix_fmt = AV_PIX_FMT_YUV420P;
        stream.encoder->time_base = av_inv_q(rate);
        stream.encoder->framerate = rate;
        stream.encoder->bit_rate = 2000000;
        stream.encoder->gop_size = 30;
    } else {
        stream.encoder->sample_rate = 48000;
        av_channel_layout_default(&stream.encoder->ch_layout, 2);
        stream.encoder->sample_fmt = AV_SAMPLE_FMT_FLTP;
        stream.encoder->time_base = AVRational{1, stream.encoder->sample_rate};
        stream.encoder->bit_rate = 192000;
    }
    if (output->oformat->flags & AVFMT_GLOBALHEADER) {
        stream.encoder->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    if (avcodec_open2(stream.encoder, codec, nullptr) < 0 ||
        avcodec_parameters_from_context(stream.output->codecpar, stream.encoder) < 0) {
        return false;
    }
    stream.output->time_base = stream.encoder->time_base;
    return true;
}

bool WritePackets(Context &context, Stream &stream, AVFrame *frame) {
    if (avcodec_send_frame(stream.encoder, frame) < 0) {
        return false;
    }
    AVPacket *packet = av_packet_alloc();
    if (!packet) {
        return false;
    }
    bool okay = true;
    for (;;) {
        const int result = avcodec_receive_packet(stream.encoder, packet);
        if (result == AVERROR(EAGAIN) || result == AVERROR_EOF) {
            break;
        }
        if (result < 0) {
            okay = false;
            break;
        }
        av_packet_rescale_ts(packet, stream.encoder->time_base, stream.output->time_base);
        packet->stream_index = stream.output->index;
        if (av_interleaved_write_frame(context.output, packet) < 0) {
            okay = false;
            break;
        }
        av_packet_unref(packet);
    }
    av_packet_free(&packet);
    return okay;
}

bool ConvertVideo(Context &context, Stream &stream) {
    auto *frame = stream.converted;
    av_frame_unref(frame);
    frame->format = stream.encoder->pix_fmt;
    frame->width = stream.encoder->width;
    frame->height = stream.encoder->height;
    if (av_frame_get_buffer(frame, 32) < 0) {
        return false;
    }
    stream.scale = sws_getCachedContext(stream.scale,
                                        stream.decoder->width,
                                        stream.decoder->height,
                                        static_cast<AVPixelFormat>(stream.decoded->format),
                                        frame->width,
                                        frame->height,
                                        stream.encoder->pix_fmt,
                                        SWS_BILINEAR,
                                        nullptr,
                                        nullptr,
                                        nullptr);
    if (!stream.scale || sws_scale(stream.scale,
                                   stream.decoded->data,
                                   stream.decoded->linesize,
                                   0,
                                   stream.decoded->height,
                                   frame->data,
                                   frame->linesize) <= 0) {
        return false;
    }
    frame->pts = stream.decoded->best_effort_timestamp == AV_NOPTS_VALUE ?
                     0 :
                     av_rescale_q(stream.decoded->best_effort_timestamp,
                                  stream.input->time_base,
                                  stream.encoder->time_base);
    return WritePackets(context, stream, frame);
}

bool WriteAudio(Context &context, Stream &stream, bool final) {
    const int frameSize = stream.encoder->frame_size;
    while (av_audio_fifo_size(stream.fifo) >= frameSize ||
           (final && av_audio_fifo_size(stream.fifo) > 0)) {
        auto *frame = stream.converted;
        av_frame_unref(frame);
        frame->format = stream.encoder->sample_fmt;
        frame->sample_rate = stream.encoder->sample_rate;
        frame->nb_samples = frameSize;
        if (av_channel_layout_copy(&frame->ch_layout, &stream.encoder->ch_layout) < 0 ||
            av_frame_get_buffer(frame, 0) < 0) {
            return false;
        }
        const int available = std::min(frameSize, av_audio_fifo_size(stream.fifo));
        if (av_audio_fifo_read(stream.fifo, reinterpret_cast<void **>(frame->data), available) !=
            available) {
            return false;
        }
        if (available < frameSize) {
            av_samples_set_silence(
                frame->data, available, frameSize - available, 2, stream.encoder->sample_fmt);
        }
        frame->pts = stream.audioSamples;
        stream.audioSamples += frameSize;
        if (!WritePackets(context, stream, frame)) {
            return false;
        }
    }
    return true;
}

bool ConvertAudio(Context &context, Stream &stream) {
    const int capacity = swr_get_out_samples(stream.resample, stream.decoded->nb_samples);
    if (capacity <= 0 || capacity > 48000 * 4) {
        return false;
    }
    AVFrame *frame = av_frame_alloc();
    if (!frame) {
        return false;
    }
    frame->format = stream.encoder->sample_fmt;
    frame->sample_rate = stream.encoder->sample_rate;
    frame->nb_samples = capacity;
    bool okay = av_channel_layout_copy(&frame->ch_layout, &stream.encoder->ch_layout) >= 0 &&
                av_frame_get_buffer(frame, 0) >= 0;
    if (okay) {
        const int count =
            swr_convert(stream.resample,
                        frame->data,
                        capacity,
                        const_cast<const std::uint8_t **>(stream.decoded->extended_data),
                        stream.decoded->nb_samples);
        okay = count >= 0 &&
               av_audio_fifo_realloc(stream.fifo, av_audio_fifo_size(stream.fifo) + count) >= 0 &&
               av_audio_fifo_write(stream.fifo, reinterpret_cast<void **>(frame->data), count) ==
                   count &&
               WriteAudio(context, stream, false);
    }
    av_frame_free(&frame);
    return okay;
}

bool Decode(Context &context, Stream &stream, AVPacket *packet, bool video) {
    if (avcodec_send_packet(stream.decoder, packet) < 0) {
        return false;
    }
    for (;;) {
        const int result = avcodec_receive_frame(stream.decoder, stream.decoded);
        if (result == AVERROR(EAGAIN) || result == AVERROR_EOF) {
            return true;
        }
        if (result < 0) {
            return false;
        }
        const bool okay = video ? ConvertVideo(context, stream) : ConvertAudio(context, stream);
        av_frame_unref(stream.decoded);
        if (!okay) {
            return false;
        }
    }
}

} // namespace

std::optional<std::string> ConvertIntroMovieWithLibav(const std::filesystem::path &source,
                                                      const std::filesystem::path &output,
                                                      const ImportProgress &progress) {
    Context context;
    if (!context.packet ||
        avformat_open_input(&context.input, Utf8FromPath(source).c_str(), nullptr, nullptr) < 0 ||
        avformat_find_stream_info(context.input, nullptr) < 0 ||
        !OpenDecoder(context.input, AVMEDIA_TYPE_VIDEO, context.video) ||
        avformat_alloc_output_context2(
            &context.output, nullptr, "mp4", Utf8FromPath(output).c_str()) < 0 ||
        !context.output || !OpenEncoder(context.output, context.video, AVMEDIA_TYPE_VIDEO)) {
        return "Cannot prepare libav intro movie conversion.";
    }
    const bool hasAudio = OpenDecoder(context.input, AVMEDIA_TYPE_AUDIO, context.audio);
    if (hasAudio) {
        auto &audio = context.audio;
        if (!OpenEncoder(context.output, audio, AVMEDIA_TYPE_AUDIO) ||
            swr_alloc_set_opts2(&audio.resample,
                                &audio.encoder->ch_layout,
                                audio.encoder->sample_fmt,
                                audio.encoder->sample_rate,
                                &audio.decoder->ch_layout,
                                audio.decoder->sample_fmt,
                                audio.decoder->sample_rate,
                                0,
                                nullptr) < 0 ||
            swr_init(audio.resample) < 0 ||
            !(audio.fifo = av_audio_fifo_alloc(audio.encoder->sample_fmt, 2, 1))) {
            return "Cannot prepare libav intro movie audio.";
        }
    }
    if (avio_open(&context.output->pb, Utf8FromPath(output).c_str(), AVIO_FLAG_WRITE) < 0 ||
        avformat_write_header(context.output, nullptr) < 0) {
        return "Cannot open converted intro movie.";
    }
    while (av_read_frame(context.input, context.packet) >= 0) {
        Stream *stream = context.packet->stream_index == context.video.input->index ?
                             &context.video :
                         hasAudio && context.packet->stream_index == context.audio.input->index ?
                             &context.audio :
                             nullptr;
        if (stream && !Decode(context, *stream, context.packet, stream == &context.video)) {
            return "Cannot decode or encode intro movie.";
        }
        if (progress && context.input->duration > 0 && context.packet->pts != AV_NOPTS_VALUE &&
            stream &&
            !progress("Converting the intro movie...",
                      std::clamp(context.packet->pts * av_q2d(stream->input->time_base) *
                                     1000000.0 / context.input->duration,
                                 0.0,
                                 1.0))) {
            return "Import cancelled.";
        }
        av_packet_unref(context.packet);
    }
    if (!Decode(context, context.video, nullptr, true) ||
        (hasAudio && (!Decode(context, context.audio, nullptr, false) ||
                      !WriteAudio(context, context.audio, true))) ||
        !WritePackets(context, context.video, nullptr) ||
        (hasAudio && !WritePackets(context, context.audio, nullptr)) ||
        av_write_trailer(context.output) < 0) {
        return "Cannot finish converted intro movie.";
    }
    return std::nullopt;
}

} // namespace coastline
