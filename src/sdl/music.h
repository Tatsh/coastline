#pragma once

#include <cstdint>
#include <expected>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include <SDL3/SDL.h>

namespace coastline {

/** The rate the music plays at, as the audio interface of the console sets it for 21,998 Hz. */
inline constexpr int N64_AUDIO_RATE = 21998;

/**
 * The music player and the mixers of the Nintendo 64 release, fed by an emulated audio interface.
 * Each tick runs what the mixing thread of the console runs each 10 milliseconds, and the bytes
 * the interface would have played in that time go to the output.
 */
class N64Audio {
public:
    N64Audio();
    ~N64Audio();
    N64Audio(const N64Audio &) = delete;
    N64Audio &operator=(const N64Audio &) = delete;

    /**
     * Open the default playback device.
     *
     * @return Whether the device opened. The player still renders without it.
     */
    bool OpenDevice();

    /**
     * Load a module and start it from its first order.
     *
     * @param module The XM module.
     */
    void Play(std::span<const std::uint8_t> module);

    /** Stop the module and silence its channels. */
    void Stop();

    /** @return Whether the device paused, and true without a device. */
    bool Pause();

    /** @return Whether the device resumed, and true without a device. */
    bool Resume();

    /**
     * Scale the music of the modules, apart from the sound effects the player mixes with it.
     *
     * @param gain The scale, 1 for the level of the console.
     */
    void SetMusicGain(float gain);

    /**
     * Render stereo 16-bit frames without a device, as the device would receive them.
     *
     * @param frames The frames to render.
     * @return The interleaved samples.
     */
    std::vector<std::int16_t> Render(std::size_t frames);

private:
    void Tick(std::vector<std::int16_t> &output);
    static void SDLCALL StreamCallback(void *userData,
                                       SDL_AudioStream *stream,
                                       int additional,
                                       int total);

    SDL_AudioStream *m_stream = nullptr;
    std::vector<std::uint8_t> m_module;
    std::uint32_t m_aiLength = 0;
    std::uint64_t m_aiRemainder = 0;
    std::vector<std::int16_t> m_pending;
    // The music gain in 8.8 fixed point.
    std::uint32_t m_musicGain = 256;
};

/**
 * Write interleaved stereo 16-bit samples as a WAV file.
 *
 * @param path The file.
 * @param samples The samples.
 * @param rate The frames a second.
 * @return Whether the file was written.
 */
bool WriteWav(const std::filesystem::path &path, std::span<const std::int16_t> samples, int rate);

} // namespace coastline
