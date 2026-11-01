#ifndef COASTLINE_EAR_PLAYBACKIMPLEMENTATIONS_H
#define COASTLINE_EAR_PLAYBACKIMPLEMENTATIONS_H

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include <windows.h>
// Multimedia declarations require the core Windows SDK types first.
#include <mmreg.h>
#include <mmsystem.h>
#endif

#include "ear/earplayback.h"
#include "ear/wavechunks.h"

/**
 * PCM wave playback with optional half-buffer streaming and explicit resource release.
 * File-backed instances allocate sample storage; memory and preset subclasses borrow samples.
 */
class [[gnu::packed]] PcmWaveStream : public EarPlayback {
public:
    /**
     * Initialise null file and sample references with zero read and write counters.
     * Media preparation must assign format and file offsets before use.
     * @ghidraAddress EAR: 0x1000af00
     */
    PcmWaveStream();

private:
    /** @ghidraAddress EAR: 0x1000af60 */
    void ReleaseEarPlaybackResources() override;
    /** @ghidraAddress EAR: 0x1000b8fa */
    int InitializeEarPlayback(const void *source, int streaming, int looping) override;

protected:
    /**
     * Create and fill the device buffer, apply initial frequency, and seek when requested.
     * Sample storage, format, buffer sizes, and base frequency must already be prepared.
     * @return The next 16-bit event identifier on success, wrapping without skipping zero;
     * zero also indicates a device-buffer, copy, or seek failure.
     * @ghidraAddress EAR: 0x10012ba7
     */
    std::uint16_t AllocateEarEventId() override;

private:
protected:
    /**
     * Release registered playback, close or restore the source file, and free sample storage.
     * Subclasses with borrowed sample storage replace this closure operation.
     * @ghidraAddress EAR: 0x1000b1c6
     */
    void CloseRegisteredEarPlayback() override;

private:
    /** @ghidraAddress EAR: 0x1000af76 */
    void RefillEarPlaybackStream() override;

protected:
    /**
     * Copy the sample half opposite the refill index into the corresponding device-buffer half.
     * A prepared sample buffer and device buffer are required. Device-copy failures may mark
     * playback complete and record an EAR error.
     * @ghidraAddress EAR: 0x100129c9
     */
    void CopyEarPlaybackSamplesToOutput() override;

private:
    /** @ghidraAddress EAR: 0x1000b260 */
    void CompleteEarPlayback() override;

protected:
    /**
     * Drain a completed stream by copying its first trailing half and stopping on later checks.
     * @return Nonzero when the device buffer has stopped; otherwise zero.
     * @ghidraAddress EAR: 0x10012b34
     */
    int CheckEarPlaybackLoopCompletion() override;

private:
    /** @ghidraAddress EAR: 0x10009ac0 */
    std::uint32_t GetEarPlaybackMediaLength() override;

    /** @ghidraAddress EAR: 0x1000b2a3 */
    int OpenWaveFileStream(const char *path);
    /** @ghidraAddress EAR: 0x1000b2ec */
    int PreparePcmWaveStream(const void *source);

    void CloseWaveFileStream();
    int FailPcmWavePreparation(std::uint32_t error);

protected:
    /**
     * @return One; this implementation does not query device access.
     * @ghidraAddress EAR: 0x10012e1b
     */
    int CheckWavePlaybackDeviceAccess();

    EarWaveFile file;     /*!< Source file handle; memory subclasses may retain a closed handle. */
    DWORD readOffset;     /*!< PCM or encoded data bytes read so far. */
    LONG dataStartOffset; /*!< Byte offset of source data within the file. */
    LONG originalFileOffset; /*!< Borrowed file's byte position, restored on close. */
    char *sampleBuffer;      /*!< Decoded PCM storage, borrowed by memory and preset subclasses. */
    DWORD dataBytes;         /*!< Total decoded PCM data size in bytes. */
    EarWaveFormat format;    /*!< Decoded PCM sample format. */
    DWORD writeOffset; /*!< Next refill destination offset in bytes of the current source buffer. */
};

/** PCM wave playback borrowing sample bytes from a caller-supplied RIFF image until release. */
class [[gnu::packed]] MemoryWaveStream : public PcmWaveStream {
public:
    /**
     * Initialise inherited stream state without allocating or attaching a memory image.
     * @ghidraAddress EAR: 0x1000a2a0
     */
    MemoryWaveStream();

private:
    /** @ghidraAddress EAR: 0x1000a2bf */
    void ReleaseEarPlaybackResources() override;
    /** @ghidraAddress EAR: 0x1000a5b9 */
    int InitializeEarPlayback(const void *source, int streaming, int looping) override;
    /** @ghidraAddress EAR: 0x1000a2d5 */
    void CloseRegisteredEarPlayback() override;

    /**
     * Read the format of a wave file in memory and point the samples at its data.
     *
     * @param source The wave file.
     * @return Whether the file was read.
     * @ghidraAddress EAR: 0x1000a2e8
     */
    int OpenMemoryWaveStream(const void *source);
};

/** PCM playback borrowing a preset's sample storage; the preset must outlast playback release. */
class [[gnu::packed]] SampleWavePlayback : public PcmWaveStream {
public:
    /**
     * Initialise inherited stream state without attaching a preset.
     * @ghidraAddress EAR: 0x10009910
     */
    SampleWavePlayback();

private:
    /** @ghidraAddress EAR: 0x1000992f */
    void ReleaseEarPlaybackResources() override;
    /** @ghidraAddress EAR: 0x10009958 */
    int InitializeEarPlayback(const void *source, int streaming, int looping) override;
    /** @ghidraAddress EAR: 0x10009945 */
    void CloseRegisteredEarPlayback() override;
    /** @ghidraAddress EAR: 0x10009a0c */
    void ApplyPreset(EarWaveSample *preset) override;
};

#if TGR_PLATFORM == TGR_PLATFORM_WIN
/**
 * MIDI playback through the shared MCI sequencer and Windows MIDI mixer.
 * Music devices and mixer controls must be initialised before playback operations.
 */
class [[gnu::packed]] MidiPlayback : public EarPlayback {
public:
    /**
     * Initialise empty media state; a gain adjustment must assign pendingGain before playback.
     * @ghidraAddress EAR: 0x10008d20
     */
    MidiPlayback();

    /**
     * Unregister playback and delete any temporary MIDI file and allocated temporary path.
     * Caller-supplied source paths remain borrowed.
     * @ghidraAddress EAR: 0x10008d8d
     */
    void ReleaseEarPlaybackResources() override;
    /**
     * Set shared MIDI mixer volume when active, or cache gain for the next start when inactive.
     * @param direction The unused directional row index.
     * @param gain The linear multiplier of the MIDI mixer's maximum volume, without clamping.
     * @param matrix The unused directional matrix handle.
     * @ghidraAddress EAR: 0x100093d5
     */
    void SetEarPlaybackDirectionalGain(std::uint16_t direction,
                                       double gain,
                                       std::uint16_t matrix) override;
    /**
     * Open source media in the shared sequencer and query sequence length.
     * Source flag 0x2000 writes reservedEventControl bytes to a temporary MIDI file first.
     * @param source A borrowed terminated path, or readable MIDI bytes for memory-source flags.
     * A borrowed path must remain valid until resource release.
     * @param streaming The ignored streaming request; MIDI streaming is disabled.
     * @param looping The looping marker, with nonzero enabling loop handling at completion.
     * @return One when MCI opens the sequence; zero when opening fails.
     * @ghidraAddress EAR: 0x1000914d
     */
    int InitializeEarPlayback(const void *source, int streaming, int looping) override;
    /**
     * Seek to a specified fractional start, post an enabled notification, and play the sequence.
     * @return One on successful playback; zero on failure, after closing the sequencer.
     * @ghidraAddress EAR: 0x10009051
     */
    int StartEarPlayback() override;
    /**
     * Set paused status and pause the sequencer when active.
     * @return The paused marker, one unless pausing an active sequencer fails.
     * @ghidraAddress EAR: 0x10009009
     */
    int PauseEarPlayback() override;
    /**
     * Attempt to resume an active paused sequencer and clear paused status on success.
     * @return Nonzero when unpaused after the attempt, including when already unpaused.
     * @ghidraAddress EAR: 0x1000948b
     */
    int ResumeEarPlayback() override;
    /**
     * Store mute state and write the shared Windows MIDI mixer's mute control.
     * @param muted The mute marker, with nonzero indicating mute.
     * @ghidraAddress EAR: 0x10008f83
     */
    void SetEarPlaybackMute(int muted) override;
    /**
     * Query MCI stop state unless inactive or a shared completion query is already in progress.
     * A completed loop seeks to the start, clears active state, and posts an end notification.
     * Loop completion posts the notification regardless of notification flags.
     * @return Nonzero on requested or detected non-looping completion; otherwise zero.
     * @ghidraAddress EAR: 0x10008e5b
     */
    int IsBufferedEarPlaybackComplete() override;

protected:
    /**
     * Request zero gain, record shared sequencer use, post an enabled end message, and unregister.
     * The sequencer remains open; an absent playback handle suppresses closure work.
     * @ghidraAddress EAR: 0x10008df4
     */
    void CloseRegisteredEarPlayback() override;

private:
    // The native slots share the empty folded body of CObject::AssertValid.
    void UpdateDirectSoundPlaybackGainAndPan() override;
    void UpdateDirectSoundPlaybackFrequency() override;

    MCI_PLAY_PARMS playParameters;   /*!< The play command from the start. */
    MCI_PLAY_PARMS resumeParameters; /*!< The play command a resume sends. */
    DWORD lengthMilliseconds;        /*!< The length of the sequence. */
    char *path;                      /*!< The file the sequencer plays. */
    int ownsTemporaryFile;           /*!< Whether `path` is a copy to delete on release. */
    double pendingGain;              /*!< The gain set while inactive, applied at the start. */
};

/**
 * CD track playback through the shared MCI CD device and Windows CD mixer.
 * Device, track positions, and mixer controls must be initialised before playback operations.
 */
class [[gnu::packed]] CdAudioPlayback : public EarPlayback {
public:
    /**
     * Initialise single-track mode with zero duration; media initialisation sets command ranges.
     * @ghidraAddress EAR: 0x10009ae0
     */
    CdAudioPlayback();

    /**
     * Release playback through the registered-closure operation without closing the shared device.
     * @ghidraAddress EAR: 0x10009b19
     */
    void ReleaseEarPlaybackResources() override;
    /**
     * Write the shared CD mixer volume control immediately.
     * @param direction The unused directional row index.
     * @param gain The linear multiplier of the CD mixer's maximum volume, without clamping.
     * @param matrix The unused directional matrix handle.
     * @ghidraAddress EAR: 0x1000a187
     */
    void SetEarPlaybackDirectionalGain(std::uint16_t direction,
                                       double gain,
                                       std::uint16_t matrix) override;
    /**
     * Configure a track range from the loaded CD positions without starting playback.
     * @param source A one-based track number encoded in pointer bits, or null for all audio tracks.
     * @param streaming The streaming marker to store; CD playback does not use PCM refill halves.
     * @param looping The looping marker, with nonzero enabling restart at completion.
     * @return One on success; zero with EAR error 0x0d when the track exceeds the last audio track.
     * @ghidraAddress EAR: 0x1000a051
     */
    int InitializeEarPlayback(const void *source, int streaming, int looping) override;
    /**
     * Apply specified start and end overrides, post an enabled start message, and play the range.
     * Whole-disc overrides encode track numbers divided by 10000; single-track overrides are
     * fractions of track duration. Successful single-track playback clears applied overrides.
     * @return One on MCI success, with active state set and pause status cleared; zero on failure.
     * @ghidraAddress EAR: 0x10009df1
     */
    int StartEarPlayback() override;
    /**
     * Pause CD playback and cache its position for a later resume when pausing succeeds.
     * @return One when MCI accepts the pause command; zero on failure.
     * @ghidraAddress EAR: 0x10009d39
     */
    int PauseEarPlayback() override;
    /**
     * Resume an active paused range from the cached position and clear pause status on success.
     * @return Nonzero when unpaused after the attempt, including when already unpaused.
     * @ghidraAddress EAR: 0x1000a21a
     */
    int ResumeEarPlayback() override;
    /**
     * @return The configured playback range duration in milliseconds.
     * @ghidraAddress EAR: 0x1000a280
     */
    std::uint32_t GetEarPlaybackMediaLength() override;
    /**
     * Store mute state and write the shared Windows CD mixer's mute control.
     * @param muted The mute marker, with nonzero indicating mute.
     * @ghidraAddress EAR: 0x10009cb3
     */
    void SetEarPlaybackMute(int muted) override;
    /**
     * Query MCI stop state while active and restart a completed looping range.
     * @return Nonzero on requested completion, detected non-looping completion, or loop restart
     * failure; zero while incomplete or after a successful loop restart.
     * @ghidraAddress EAR: 0x10009bba
     */
    int IsBufferedEarPlaybackComplete() override;

protected:
    /**
     * Stop CD playback, post an enabled end message, clear mute, restore full gain, and unregister.
     * An absent playback handle suppresses closure work.
     * @ghidraAddress EAR: 0x10009b2f
     */
    void CloseRegisteredEarPlayback() override;

private:
    // The native slots share the empty folded body of CObject::AssertValid.
    void UpdateDirectSoundPlaybackGainAndPan() override;
    void UpdateDirectSoundPlaybackFrequency() override;

    MCI_PLAY_PARMS playParameters; /*!< The play command, from and to positions in milliseconds. */
    MCI_PLAY_PARMS resumeParameters; /*!< The play command a resume sends. */
    int wholeDisc;                   /*!< Whether every audio track plays, positions then tracks. */
    DWORD durationMilliseconds;      /*!< The length of the tracks. */
};
#endif

/** The state of the decoder of one channel of IMA ADPCM, from the header of each block. */
struct ImaAdpcmChannelState {
    std::int16_t predictor; /*!< The last sample decoded. */
    std::int16_t stepIndex; /*!< The entry of the table of step sizes. */
};

/** A wave file of IMA ADPCM, decoded into 16-bit samples a half of the buffer at a time. */
class [[gnu::packed]] ImaAdpcmWaveStream : public PcmWaveStream {
public:
    /**
     * Initialise inherited stream state and empty ADPCM decoder and block-buffer state.
     * Media preparation must assign encoded format fields before use.
     * @ghidraAddress EAR: 0x10001000
     */
    ImaAdpcmWaveStream();

private:
    /** @ghidraAddress EAR: 0x100010aa */
    void ReleaseEarPlaybackResources() override;
    /** @ghidraAddress EAR: 0x10001a16 */
    int InitializeEarPlayback(const void *source, int streaming, int looping) override;
    /** @ghidraAddress EAR: 0x10001bd7 */
    void CloseRegisteredEarPlayback() override;
    /** @ghidraAddress EAR: 0x10001c21 */
    void RefillEarPlaybackStream() override;
    /** @ghidraAddress EAR: 0x10001ea5 */
    void CompleteEarPlayback() override;
    /**
     * Get the bytes of the encoded data.
     *
     * @return The bytes of the data chunk.
     * @ghidraAddress EAR: 0x10003ae0
     */
    std::uint32_t GetEarPlaybackMediaLength() override;

    /**
     * Open a wave file of IMA ADPCM, from a path or from memory, read its format, and read the
     * first blocks.
     *
     * @param source The path, or the wave file in memory.
     * @return Whether the file was opened.
     * @ghidraAddress EAR: 0x100010f3
     */
    int OpenImaAdpcmWaveStream(const void *source);

    /**
     * Decode the blocks of the half of the encoded buffer the stream fills next into the same half
     * of the samples.
     *
     * @ghidraAddress EAR: 0x10001ee5
     */
    void DecodeImaAdpcmStreamBlocks();

    EarImaAdpcmWaveFormat adpcmFormat; /*!< The format of the file. */
    std::uint8_t *blockBuffer;  /*!< The encoded blocks, or the part of them being streamed. */
    DWORD adpcmDataBytes;       /*!< The bytes of the data chunk. */
    DWORD blockBufferBytes;     /*!< The bytes of `blockBuffer`. */
    DWORD blockHalfBytes;       /*!< The encoded bytes each half of the buffer decodes. */
    int decodedSample;          /*!< The sample the decoder produced last. */
    int decodedDifference;      /*!< The difference the decoder added last. */
    std::uint8_t decodedNibble; /*!< The code the decoder read last. */
    ImaAdpcmChannelState *channelStates; /*!< The state of the decoder of each channel. */
    std::uint16_t blocksPerHalf;         /*!< The blocks each half of the buffer decodes. */
    DWORD dataWordsPerBlock;             /*!< The words of codes of all the channels in a block. */
    int memorySource;                    /*!< Whether the file is in memory, read in place. */
};

/**
 * Convert a value to an integer as the x87 instruction of the original does, retaining the low 32
 * bits.
 *
 * @param value The value to convert.
 * @return The low 32 bits of the integer, or 0 where the conversion is invalid.
 */
inline std::uint32_t TruncatePlaybackInteger(long double value) {
    // Invalid x87 integer conversions produce zero in the low 32 bits.
    return value >= -0x1p63L && value < 0x1p63L ?
               static_cast<std::uint32_t>(static_cast<std::int64_t>(value)) :
               0;
}

#endif /* COASTLINE_EAR_PLAYBACKIMPLEMENTATIONS_H */
