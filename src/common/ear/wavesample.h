#ifndef COASTLINE_EAR_WAVESAMPLE_H
#define COASTLINE_EAR_WAVESAMPLE_H

#include <cstdint>

#include "ear/wavechunks.h"

/** The source of a wave sample, as RegisterPreset passes it. */
enum EarWaveSampleSource : std::uint16_t {
    EAR_WAVE_SAMPLE_SOURCE_FILE = 0,   /*!< A path to a wave file. */
    EAR_WAVE_SAMPLE_SOURCE_MEMORY = 2, /*!< A wave file in memory. */
};

/** A wave file loaded whole into memory, which a preset registers and sample playbacks share. */
class [[gnu::packed]] EarWaveSample {
public:
    /**
     * Initialise empty sample storage and file-source defaults without assigning a wave format.
     * @ghidraAddress EAR: 0x10009520
     */
    EarWaveSample();
    /**
     * Free allocated sample storage and subtract its tracked byte count from the sample budget.
     * Borrowing sample playbacks must be released before destruction.
     * @ghidraAddress EAR: 0x10009554
     */
    ~EarWaveSample();

    /**
     * Load wave samples into allocated storage with sample-memory budget accounting.
     * Use an empty sample object; loading does not release a previous sample allocation first.
     * Failed loads retain any budget reservation until destruction.
     *
     * @param source A non-null terminated path or complete readable RIFF image, borrowed during
     * load.
     * @param sourceKind How @p source is given.
     * @return One on success; zero with an EAR error for format, I/O, budget, or allocation
     * failure.
     * @ghidraAddress EAR: 0x1000958a
     */
    int LoadWaveSample(const void *source, EarWaveSampleSource sourceKind);

    /**
     * @return Borrowed sample storage, valid until destruction or replacement, or null when empty.
     * @ghidraAddress EAR: 0x100098b1
     */
    char *GetWaveSampleBuffer() const;

    /**
     * @return The bytes of the samples.
     * @ghidraAddress EAR: 0x100098c1
     */
    DWORD GetWaveSampleByteCount() const;

    /**
     * @return A copy of the loaded format; a successful load must precede this query.
     * @ghidraAddress EAR: 0x100098d2
     */
    EarWaveFormat CopyWaveSampleFormat() const;

private:
    /**
     * Read the format and the samples of a wave file.
     *
     * @param source The path of the file, or the file in memory.
     * @return Whether the file was read.
     * @ghidraAddress EAR: 0x100095bb
     */
    int ReadWaveSampleData(const void *source);

    /**
     * Decide whether to load the samples of a file whose size is known.
     *
     * @return Whether to load them, which is always.
     * @ghidraAddress EAR: 0x1001255f
     */
    int AcceptWaveSampleData();

    char *buffer;                             /*!< The samples, allocated with malloc(). */
    DWORD byteCount;                          /*!< The bytes of the samples. */
    EarWaveFormat format;                     /*!< The format, as the file gives it. */
    EarWaveSampleSource sourceKind;           /*!< How the file was given. */
    std::uint32_t unknown1c;                  /*!< Cleared on construction. */
    [[maybe_unused]] std::uint32_t unknown20; /* +0x20 */
};

#endif /* COASTLINE_EAR_WAVESAMPLE_H */
