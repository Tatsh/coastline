#include "flactrack.h"

#include <algorithm>

#include <SDL3/SDL.h>

namespace coastline {

void FlacTrack::DecoderDeleter::operator()(FLAC__StreamDecoder *decoder) const {
    FLAC__stream_decoder_delete(decoder);
}

bool FlacTrack::Open(const std::filesystem::path &path) {
    m_samples.clear();
    m_read = 0;
    m_decoder.reset(FLAC__stream_decoder_new());
    if (!m_decoder) {
        return false;
    }
    const auto file = path.u8string();
    const std::string utf8(file.begin(), file.end());
    const auto status =
        FLAC__stream_decoder_init_file(m_decoder.get(), utf8.c_str(), Write, nullptr, Error, this);
    if (status != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
        SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO,
                    "Cannot play %s: %s",
                    utf8.c_str(),
                    FLAC__StreamDecoderInitStatusString[status]);
        m_decoder.reset();
        return false;
    }
    return true;
}

bool FlacTrack::Decode(std::vector<std::int16_t> &output, std::size_t frames) {
    const std::size_t target = output.size() + frames * 2;
    while (output.size() < target) {
        if (m_read < m_samples.size()) {
            const auto count = std::min(target - output.size(), m_samples.size() - m_read);
            output.insert(
                output.end(), m_samples.begin() + m_read, m_samples.begin() + m_read + count);
            m_read += count;
            continue;
        }
        m_samples.clear();
        m_read = 0;
        if (!m_decoder) {
            return false;
        }
        if (!FLAC__stream_decoder_process_single(m_decoder.get()) ||
            FLAC__stream_decoder_get_state(m_decoder.get()) == FLAC__STREAM_DECODER_END_OF_STREAM) {
            m_decoder.reset();
        }
    }
    return m_decoder != nullptr;
}

FLAC__StreamDecoderWriteStatus FlacTrack::Write(const FLAC__StreamDecoder *,
                                                const FLAC__Frame *frame,
                                                const FLAC__int32 *const buffer[],
                                                void *user) {
    auto *track = static_cast<FlacTrack *>(user);
    const unsigned bits = frame->header.bits_per_sample;
    for (unsigned index = 0; index < frame->header.blocksize; ++index) {
        for (unsigned channel = 0; channel < 2; ++channel) {
            const FLAC__int32 sample =
                buffer[channel < frame->header.channels ? channel : 0][index];
            track->m_samples.push_back(static_cast<std::int16_t>(
                bits > 16 ? sample >> (bits - 16) : sample << (16 - bits)));
        }
    }
    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void FlacTrack::Error(const FLAC__StreamDecoder *, FLAC__StreamDecoderErrorStatus status, void *) {
    SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO,
                "Invalid FLAC audio: %s",
                FLAC__StreamDecoderErrorStatusString[status]);
}

} // namespace coastline
