#pragma once

#include <filesystem>
#include <memory>

#include <SDL3/SDL.h>

namespace coastline {

/** Converted sound selection and per-sample level matching for one game session. */
class SoundEffects {
public:
    /** The preferred converted race sound library; menu feedback retains front-end samples. */
    enum class Source {
        Pc, /*!< Use the PC race samples at their native playback rates. */
        /** Substitute console samples at matched PC rates and levels; silence unmapped taunts. */
        N64
    };

    /**
     * Register the exclusive sound selection context. Another instance must not be active.
     *
     * @param assets The prepared asset directory used to locate console substitutions.
     * @param source The preferred race sample library.
     * @param enabled Whether loaded samples remain audible; false produces silent samples.
     */
    SoundEffects(const std::filesystem::path &assets, Source source, bool enabled);
    /** Unregister sample selection and release cached converted samples. */
    ~SoundEffects();
    /** Prevent copying the registered sound selection context. */
    SoundEffects(const SoundEffects &) = delete;
    /** Prevent assignment of the registered sound selection context. */
    SoundEffects &operator=(const SoundEffects &) = delete;

    /**
     * Read a race sample, matching available N64 substitutions to the PC sample's rate and level.
     *
     * @param path The UTF-8 PC reference WAV path, used directly without a matching substitution.
     * @param sample The race sound index; N64 mode silences taunts absent from the console.
     * @param spec Receives the format of the decoded or converted PCM; must not be null.
     * @param data Non-null output pointer for allocated PCM; free with SDL_free() after success.
     * @param bytes Receives the PCM byte count; must not be null.
     * @return Whether decoding and allocation succeed. Outputs are unspecified on failure.
     */
    static bool
    LoadRaceWave(const char *path, int sample, SDL_AudioSpec *spec, Uint8 **data, Uint32 *bytes);

    /**
     * Read menu feedback as mono unsigned 8-bit PCM at 10999 frames per second.
     *
     * @param path The converted front-end WAV file.
     * @param spec Receives the PCM format; must not be null.
     * @param data Non-null output pointer for allocated PCM; free with SDL_free() after success.
     * @param bytes Receives the PCM byte count; must not be null.
     * @return Whether decoding, conversion, and allocation succeed.
     * Outputs are unspecified on failure.
     */
    static bool LoadMenuWave(const std::filesystem::path &path,
                             SDL_AudioSpec *spec,
                             Uint8 **data,
                             Uint32 *bytes);

private:
    struct State;
    std::unique_ptr<State> m_state;
};

} // namespace coastline
