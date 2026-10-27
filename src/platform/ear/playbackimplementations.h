#ifndef COASTLINE_EAR_PLAYBACKIMPLEMENTATIONS_H
#define COASTLINE_EAR_PLAYBACKIMPLEMENTATIONS_H

#include "earplayback.h"

class [[gnu::packed]] PcmWaveStream : public EarPlayback {
public:
    /**
     * @ghidraAddress EAR: 0x1000af00
     * @stub
     */
    PcmWaveStream();

private:
    [[maybe_unused]] std::uint8_t unknownAc[0x2e]; /* +0xac */
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
