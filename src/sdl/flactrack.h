#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

#include <FLAC/stream_decoder.h>

namespace coastline {

/** Decode imported FLAC tracks into stereo PCM without discarding block remainders. */
class FlacTrack {
public:
    bool Open(const std::filesystem::path &path);

    /** Append at most the requested stereo frames; return false at EOF or on failure. */
    bool Decode(std::vector<std::int16_t> &output, std::size_t frames);

private:
    struct DecoderDeleter {
        void operator()(FLAC__StreamDecoder *decoder) const;
    };
    static FLAC__StreamDecoderWriteStatus Write(const FLAC__StreamDecoder *,
                                                const FLAC__Frame *frame,
                                                const FLAC__int32 *const buffer[],
                                                void *user);
    static void Error(const FLAC__StreamDecoder *, FLAC__StreamDecoderErrorStatus status, void *);

    std::unique_ptr<FLAC__StreamDecoder, DecoderDeleter> m_decoder;
    std::vector<std::int16_t> m_samples;
    std::size_t m_read = 0;
};

} // namespace coastline
