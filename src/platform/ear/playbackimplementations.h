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
    /** @ghidraAddress EAR: 0x10012ba7 */
    std::uint16_t AllocateEarEventId() override;
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
    /**
     * @ghidraAddress EAR: 0x1000b2ec
     * @stub
     */
    int PreparePcmWaveStream(const void *source);
    /** @ghidraAddress EAR: 0x10012e1b */
    int CheckWavePlaybackDeviceAccess();

    HMMIO file;
    DWORD readOffset;
    LONG dataStartOffset;
    LONG originalFileOffset;
    char *sampleBuffer;
    DWORD dataBytes;
    WAVEFORMATEX format;
    DWORD writeOffset;
};

class [[gnu::packed]] MemoryWaveStream : public EarPlayback {
public:
    /**
     * @ghidraAddress EAR: 0x1000a2a0
     * @stub
     */
    MemoryWaveStream();

private:
    [[maybe_unused]] std::uint8_t unknownAc[0x2e]; /* +0xac */
};

class [[gnu::packed]] SampleWavePlayback : public EarPlayback {
public:
    /**
     * @ghidraAddress EAR: 0x10009910
     * @stub
     */
    SampleWavePlayback();

private:
    [[maybe_unused]] std::uint8_t unknownAc[0x2e]; /* +0xac */
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

#endif
