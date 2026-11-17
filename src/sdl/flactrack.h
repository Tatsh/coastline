#pragma once

/**
 * @file
 * The decoder of the FLAC tracks the import converts from the CD audio of the disc.
 */

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
    // Release a FLAC decoder with FLAC__stream_decoder_delete().
    struct DecoderDeleter {
        void operator()(FLAC__StreamDecoder *decoder) const;
    };

    // Append a decoded block to the pending samples as interleaved 16-bit stereo. Samples of
    // other widths are shifted to 16 bits, and a mono block fills both channels. The user
    // pointer is the FlacTrack that opened the decoder, and the result is always
    // FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE.
    static FLAC__StreamDecoderWriteStatus Write(const FLAC__StreamDecoder *,
                                                const FLAC__Frame *frame,
                                                const FLAC__int32 *const buffer[],
                                                void *user);

    // Log a decoding error as an audio warning. Decoding continues.
    static void Error(const FLAC__StreamDecoder *, FLAC__StreamDecoderErrorStatus status, void *);

    // The open decoder, or empty after a failed open, an error, or the end of the stream.
    std::unique_ptr<FLAC__StreamDecoder, DecoderDeleter> m_decoder;
    std::vector<std::int16_t> m_samples; // The interleaved samples of the last block.
    std::size_t m_read = 0;              // The samples of m_samples already appended to an output.
};

} // namespace coastline
