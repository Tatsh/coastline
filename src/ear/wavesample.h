#ifndef COASTLINE_EAR_WAVESAMPLE_H
#define COASTLINE_EAR_WAVESAMPLE_H

#include <cstdint>

#include <windows.h>
// Multimedia declarations require the core Windows SDK types first.
#include <mmreg.h>

/** The source of a wave sample, as RegisterPreset passes it. */
enum EarWaveSampleSource : std::uint16_t {
    EAR_WAVE_SAMPLE_SOURCE_FILE = 0,   /*!< A path to a wave file. */
    EAR_WAVE_SAMPLE_SOURCE_MEMORY = 2, /*!< A wave file in memory. */
};

/** A wave file loaded whole into memory, which a preset registers and sample playbacks share. */
class [[gnu::packed]] EarWaveSample {
public:
    /** @ghidraAddress EAR: 0x10009520 */
    EarWaveSample();
    /** @ghidraAddress EAR: 0x10009554 */
    ~EarWaveSample();

    /**
     * Load the samples of a wave file.
     *
     * @param source The path of the file, or the file in memory.
     * @param sourceKind How @p source is given.
     * @return Whether the samples were loaded.
     * @ghidraAddress EAR: 0x1000958a
     */
    int LoadWaveSample(const void *source, EarWaveSampleSource sourceKind);

    /**
     * @return The samples.
     * @ghidraAddress EAR: 0x100098b1
     */
    char *GetWaveSampleBuffer() const;

    /**
     * @return The bytes of the samples.
     * @ghidraAddress EAR: 0x100098c1
     */
    DWORD GetWaveSampleByteCount() const;

    /**
     * @return The format of the samples.
     * @ghidraAddress EAR: 0x100098d2
     */
    WAVEFORMATEX CopyWaveSampleFormat() const;

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
    WAVEFORMATEX format;                      /*!< The format, as the file gives it. */
    EarWaveSampleSource sourceKind;           /*!< How the file was given. */
    std::uint32_t unknown1c;                  /*!< Cleared on construction. */
    [[maybe_unused]] std::uint32_t unknown20; /* +0x20 */
};

#endif /* COASTLINE_EAR_WAVESAMPLE_H */
