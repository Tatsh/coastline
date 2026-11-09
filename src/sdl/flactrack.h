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
    /**
     * Open an imported track, replacing any previous decoder and pending samples.
     * Mono samples duplicate into both output channels; multichannel input uses its first two.
     *
     * @param path The FLAC file to decode.
     * @return Whether decoder initialisation succeeded.
     */
    bool Open(const std::filesystem::path &path);

    /**
     * Append at most the requested stereo frames without discarding unused block samples.
     *
     * @param output Receives interleaved signed 16-bit left and right samples.
     * @param frames The maximum number of stereo frames to append.
     * @return Whether the decoder remains active. Output may grow even when false is returned.
     */
    bool Decode(std::vector<std::int16_t> &output, std::size_t frames);

private:
    /** Release a FLAC decoder through its library API. */
    struct DecoderDeleter {
        /** Release @p decoder. */
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
