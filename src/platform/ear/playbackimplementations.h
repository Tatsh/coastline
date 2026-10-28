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
    /** @ghidraAddress EAR: 0x1000b1c6 */
    void CloseRegisteredEarPlayback() override;
    /** @ghidraAddress EAR: 0x1000af76 */
    void RefillEarPlaybackStream() override;
    /** @ghidraAddress EAR: 0x100129c9 */
    void CopyEarPlaybackSamplesToOutput() override;
    /** @ghidraAddress EAR: 0x1000b260 */
    void CompleteEarPlayback() override;
    /** @ghidraAddress EAR: 0x10012b34 */
    int CheckEarPlaybackLoopCompletion() override;
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

class [[gnu::packed]] MidiPlayback : public EarPlayback {
public:
    /**
     * @ghidraAddress EAR: 0x10008d20
     * @stub
     */
    MidiPlayback();

private:
    [[maybe_unused]] std::uint8_t unknownAc[0x2c]; /* +0xac */
};

class [[gnu::packed]] CdAudioPlayback : public EarPlayback {
public:
    /**
     * @ghidraAddress EAR: 0x10009ae0
     * @stub
     */
    CdAudioPlayback();

private:
    [[maybe_unused]] std::uint8_t unknownAc[0x20]; /* +0xac */
};

class [[gnu::packed]] ImaAdpcmWaveStream : public EarPlayback {
public:
    /**
     * @ghidraAddress EAR: 0x10001000
     * @stub
     */
    ImaAdpcmWaveStream();

private:
    [[maybe_unused]] std::uint8_t unknownAc[0x69]; /* +0xac */
};

#endif /* COASTLINE_EAR_PLAYBACKIMPLEMENTATIONS_H */
