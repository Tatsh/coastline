#ifndef COASTLINE_EAR_PLAYBACKIMPLEMENTATIONS_H
#define COASTLINE_EAR_PLAYBACKIMPLEMENTATIONS_H

#include <windows.h>
// Multimedia declarations require the core Windows SDK types first.
#include <mmreg.h>
#include <mmsystem.h>

#include "earplayback.h"

class [[gnu::packed]] PcmWaveStream : public EarPlayback {
public:
    /** @ghidraAddress EAR: 0x1000af00 */
    PcmWaveStream();

private:
    /** @ghidraAddress EAR: 0x1000af60 */
    void ReleaseEarPlaybackResources() override;
    /** @ghidraAddress EAR: 0x1000b8fa */
    int InitializeEarPlayback(const void *source, int streaming, int looping) override;

protected:
    /** @ghidraAddress EAR: 0x10012ba7 */
    std::uint16_t AllocateEarEventId() override;

private:
protected:
    /** @ghidraAddress EAR: 0x1000b1c6 */
    void CloseRegisteredEarPlayback() override;

private:
    /** @ghidraAddress EAR: 0x1000af76 */
    void RefillEarPlaybackStream() override;

protected:
    /** @ghidraAddress EAR: 0x100129c9 */
    void CopyEarPlaybackSamplesToOutput() override;

private:
    /** @ghidraAddress EAR: 0x1000b260 */
    void CompleteEarPlayback() override;

protected:
    /** @ghidraAddress EAR: 0x10012b34 */
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
    /** @ghidraAddress EAR: 0x10012e1b */
    int CheckWavePlaybackDeviceAccess();

    HMMIO file;              /*!< The file the samples stream from. */
    DWORD readOffset;        /*!< The bytes of the data read so far. */
    LONG dataStartOffset;    /*!< The offset of the data in the file. */
    LONG originalFileOffset; /*!< The offset of a file the caller opened, restored on close. */
    char *sampleBuffer;      /*!< The samples, or the part of them being streamed. */
    DWORD dataBytes;         /*!< The bytes of the data. */
    WAVEFORMATEX format;     /*!< The format of the samples. */
    DWORD writeOffset;       /*!< The offset in the buffer the next samples are written at. */
};

/** A wave file in memory, played from where it lies. */
class [[gnu::packed]] MemoryWaveStream : public PcmWaveStream {
public:
    /** @ghidraAddress EAR: 0x1000a2a0 */
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

/** A playback of the samples of a preset. */
class [[gnu::packed]] SampleWavePlayback : public PcmWaveStream {
public:
    /** @ghidraAddress EAR: 0x10009910 */
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

/** A playback of a MIDI file through the MCI sequencer, its volume set through the mixer. */
class [[gnu::packed]] MidiPlayback : public EarPlayback {
public:
    /** @ghidraAddress EAR: 0x10008d20 */
    MidiPlayback();

    /** @ghidraAddress EAR: 0x10008d8d */
    void ReleaseEarPlaybackResources() override;
    /** @ghidraAddress EAR: 0x100093d5 */
    void SetEarPlaybackDirectionalGain(std::uint16_t direction,
                                       double gain,
                                       std::uint16_t matrix) override;
    /** @ghidraAddress EAR: 0x1000914d */
    int InitializeEarPlayback(const void *source, int streaming, int looping) override;
    /** @ghidraAddress EAR: 0x10009051 */
    int StartEarPlayback() override;
    /** @ghidraAddress EAR: 0x10009009 */
    int PauseEarPlayback() override;
    /** @ghidraAddress EAR: 0x1000948b */
    int ResumeEarPlayback() override;
    /** @ghidraAddress EAR: 0x10008f83 */
    void SetEarPlaybackMute(int muted) override;
    /** @ghidraAddress EAR: 0x10008e5b */
    int IsBufferedEarPlaybackComplete() override;

protected:
    /** @ghidraAddress EAR: 0x10008df4 */
    void CloseRegisteredEarPlayback() override;

private:
    // The native slots share the empty folded body of CObject::AssertValid.
    void UpdateDirectSoundPlaybackGainAndPan() override;
    void UpdateDirectSoundPlaybackFrequency() override;

    MCI_PLAY_PARMS playParameters;   /*!< The play command from the start. */
    MCI_PLAY_PARMS resumeParameters; /*!< The play command a resume sends. */
    DWORD lengthMilliseconds;        /*!< The length of the sequence. */
    char *path;                      /*!< The file the sequencer plays. */
    int ownsTemporaryFile;           /*!< Whether @ref path is a copy to delete on release. */
    double pendingGain;              /*!< The gain set while inactive, applied at the start. */
};

/** A playback of the tracks of the CD through the MCI CD audio device. */
class [[gnu::packed]] CdAudioPlayback : public EarPlayback {
public:
    /** @ghidraAddress EAR: 0x10009ae0 */
    CdAudioPlayback();

    /** @ghidraAddress EAR: 0x10009b19 */
    void ReleaseEarPlaybackResources() override;
    /** @ghidraAddress EAR: 0x1000a187 */
    void SetEarPlaybackDirectionalGain(std::uint16_t direction,
                                       double gain,
                                       std::uint16_t matrix) override;
    /** @ghidraAddress EAR: 0x1000a051 */
    int InitializeEarPlayback(const void *source, int streaming, int looping) override;
    /** @ghidraAddress EAR: 0x10009df1 */
    int StartEarPlayback() override;
    /** @ghidraAddress EAR: 0x10009d39 */
    int PauseEarPlayback() override;
    /** @ghidraAddress EAR: 0x1000a21a */
    int ResumeEarPlayback() override;
    /** @ghidraAddress EAR: 0x1000a280 */
    std::uint32_t GetEarPlaybackMediaLength() override;
    /** @ghidraAddress EAR: 0x10009cb3 */
    void SetEarPlaybackMute(int muted) override;
    /** @ghidraAddress EAR: 0x10009bba */
    int IsBufferedEarPlaybackComplete() override;

protected:
    /** @ghidraAddress EAR: 0x10009b2f */
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

/** The state of the decoder of one channel of IMA ADPCM, from the header of each block. */
struct ImaAdpcmChannelState {
    std::int16_t predictor; /*!< The last sample decoded. */
    std::int16_t stepIndex; /*!< The entry of the table of step sizes. */
};

/** A wave file of IMA ADPCM, decoded into 16-bit samples a half of the buffer at a time. */
class [[gnu::packed]] ImaAdpcmWaveStream : public PcmWaveStream {
public:
    /** @ghidraAddress EAR: 0x10001000 */
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

    IMAADPCMWAVEFORMAT adpcmFormat; /*!< The format of the file. */
    std::uint8_t *blockBuffer;      /*!< The encoded blocks, or the part of them being streamed. */
    DWORD adpcmDataBytes;           /*!< The bytes of the data chunk. */
    DWORD blockBufferBytes;         /*!< The bytes of @ref blockBuffer. */
    DWORD blockHalfBytes;           /*!< The encoded bytes each half of the buffer decodes. */
    int decodedSample;              /*!< The sample the decoder produced last. */
    int decodedDifference;          /*!< The difference the decoder added last. */
    std::uint8_t decodedNibble;     /*!< The code the decoder read last. */
    ImaAdpcmChannelState *channelStates; /*!< The state of the decoder of each channel. */
    std::uint16_t blocksPerHalf;         /*!< The blocks each half of the buffer decodes. */
    DWORD dataWordsPerBlock;             /*!< The words of codes of all the channels in a block. */
    int memorySource;                    /*!< Whether the file is in memory, read in place. */
};

#endif /* COASTLINE_EAR_PLAYBACKIMPLEMENTATIONS_H */
