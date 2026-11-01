#pragma once

#include <filesystem>
#include <memory>

#include <SDL3/SDL.h>

namespace coastline {

/** Converted sound selection and per-sample level matching for one game session. */
class SoundEffects {
public:
    enum class Source { Pc, N64 };

    SoundEffects(const std::filesystem::path &assets, Source source, bool enabled);
    ~SoundEffects();
    SoundEffects(const SoundEffects &) = delete;
    SoundEffects &operator=(const SoundEffects &) = delete;

    /** Read a race sample at its PC playback rate, with N64 levels matched to the PC sample. */
    static bool
    LoadRaceWave(const char *path, int sample, SDL_AudioSpec *spec, Uint8 **data, Uint32 *bytes);

    /** Read menu feedback as mono unsigned PCM at the native front-end rate. */
    static bool LoadMenuWave(const std::filesystem::path &path,
                             SDL_AudioSpec *spec,
                             Uint8 **data,
                             Uint32 *bytes);

private:
    struct State;
    std::unique_ptr<State> m_state;
};

} // namespace coastline
