#pragma once

/**
 * @file
 * The XM music player and sample mixers of the Nintendo 64 release, streamed to an SDL playback
 * device, with the context and lock that guard them against the audio thread.
 */

#include <cstdint>
#include <expected>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include <SDL3/SDL.h>

namespace coastline {

class N64Mixer;

/** Mixer channels, XM storage, and synchronisation for one SDL audio session. */
class N64MixerContext {
public:
    /**
     * Register the sole mixer context. Another context must not already be active.
     *
     * @throws std::runtime_error If mutex creation or context registration fails.
     */
    N64MixerContext();
    /** Unregister the mixer after all audio players and locks have been destroyed. */
    ~N64MixerContext();
    /** Prevent copying the registered mixer and its synchronisation resources. */
    N64MixerContext(const N64MixerContext &) = delete;
    /**
     * Prevent assignment of the registered mixer and its synchronisation resources.
     *
     * @return This object. The operator is deleted and is never called.
     */
    N64MixerContext &operator=(const N64MixerContext &) = delete;

private:
    std::unique_ptr<N64Mixer> m_mixer; // The registered mixer state, with its mutex.
};

/** The sample rate of the imported music, in stereo frames per second. */
inline constexpr int N64_AUDIO_RATE = 21998;

/**
 * The XM player and sample mixers of the Nintendo 64 release, streamed as native stereo PCM.
 * Mixer controls advance every 10 milliseconds, with lookahead for notes and sound effects.
 */
class N64Audio {
public:
    /** Initialise channels in the active N64MixerContext, without opening an audio device. */
    N64Audio();
    /** Destroy the playback stream before the mixer context is destroyed. */
    ~N64Audio();
    /** Prevent copying the playback stream and module storage. */
    N64Audio(const N64Audio &) = delete;
    /**
     * Prevent assignment of the playback stream and module storage.
     *
     * @return This object. The operator is deleted and is never called.
     */
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
     * @param module A valid imported XM module, copied into player storage before playback.
     */
    void Play(std::span<const std::uint8_t> module);

    /** Stop the module and silence its channels. */
    void Stop();

    /**
     * Mark playback paused and pause the device through UpdateVolumeProcessing().
     *
     * @return Whether the device paused, and true without a device.
     */
    bool Pause();

    /**
     * Clear the pause and resume the device unless both audio categories are silent.
     *
     * @return Whether the device resumed, and true without a device.
     */
    bool Resume();
    /**
     * Suspend the device while paused or while both enabled audio categories are silent.
     *
     * @return Whether the required device transition succeeds, or true without a transition.
     */
    bool UpdateVolumeProcessing();
    /**
     * Enable each category independently of transient fades and playback pauses.
     *
     * @param music Whether to process music while its volume is nonzero.
     * @param soundEffects Whether to process effects while their master volume is nonzero.
     */
    void SetProcessingEnabled(bool music, bool soundEffects);

    /**
     * Scale the music of the modules, apart from the sound effects the player mixes with it.
     *
     * @param gain The finite scale, clamped to 0 through 8, with 1 for the console level.
     */
    void SetMusicGain(float gain);

    /**
     * Render stereo 16-bit frames without a device, as the device would receive them. The
     * volumes of the game are published first.
     *
     * @param frames The frames to render.
     * @return The interleaved samples.
     */
    std::vector<std::int16_t> Render(std::size_t frames);

private:
    // Render exactly the requested stereo frames with the mixer lock already taken, silent while
    // both categories are off. Samples rendered past the request are retained for the next call.
    // When both categories become silent, the ring, the pending samples, and the lookahead are
    // reset.
    std::vector<std::int16_t> RenderLocked(std::size_t frames);
    // Advance the mixers by one 10-millisecond tick of the timer of the console, appending an even
    // number of stereo frames to the output. The music player advances on every second tick. The
    // first tick after a reset also mixes the lookahead.
    void Tick(std::vector<std::int16_t> &output);
    // Supply an SDL playback stream with rendered samples under the mixer lock. The user data is
    // the N64Audio that opened the stream. Nothing is rendered when the additional byte count is
    // not positive, and the total is unused.
    static void SDLCALL StreamCallback(void *userData,
                                       SDL_AudioStream *stream,
                                       int additional,
                                       int total);

    N64Mixer &m_mixer;                    // The mixer of the active N64MixerContext.
    SDL_AudioStream *m_stream = nullptr;  // The playback stream, or nullptr without a device.
    std::vector<std::uint8_t> m_module;   // The copy of the module the player reads in place.
    std::uint32_t m_outputReadOffset = 0; // The byte offset of the next output in the ring.
    // The remainder of frames per tick, in hundredths of a frame, retained for the next tick.
    std::uint64_t m_frameRemainder = 0;
    bool m_primed = false; // Whether the lookahead has been mixed since the last reset.
    // The byte offset in the ring where the sound effects mix next.
    std::uint32_t m_soundEffectMixOffset = 0;
    // Toggled by each tick that mixes music. The player advances when it is true.
    bool m_musicTick = false;
    std::vector<std::int16_t> m_pending; // Samples rendered past the last request.
    bool m_musicProcessing = true;       // Whether the last render mixed music.
    bool m_sfxProcessing = true;         // Whether the last render mixed sound effects.
    bool m_paused = false;               // Whether Pause() was called without a later Resume().
    bool m_devicePaused = true;
    bool m_musicEnabled = true;      // Whether SetProcessingEnabled() enabled music.
    bool m_sfxEnabled = true;        // Whether SetProcessingEnabled() enabled sound effects.
    std::uint32_t m_musicGain = 256; // The music gain in 8.8 fixed point.
};

/**
 * A lock of the mixers and the music player of the console against the audio thread that mixes
 * them, for the life of the object. The game thread takes it around each change of a channel. It
 * is recursive. The audio thread waits on it, and the game thread locks it for a few stores at
 * most.
 */
class N64MixerLock {
public:
    /** Acquire the active mixer context's recursive mutex on the calling thread. */
    N64MixerLock();
    /** Release the mutex on the thread that acquired this lock. */
    ~N64MixerLock();
    /** Prevent copying a mutex acquisition. */
    N64MixerLock(const N64MixerLock &) = delete;
    /**
     * Prevent assignment of a mutex acquisition.
     *
     * @return This object. The operator is deleted and is never called.
     */
    N64MixerLock &operator=(const N64MixerLock &) = delete;

private:
    SDL_Mutex *m_mutex; // The recursive mutex of the mixer context, locked by this object.
};

/**
 * Hand the volumes the game writes (g_bBgmVolume, g_bMusicVolume, g_bSfxMasterVolume, and
 * g_bSfxVolume) to the mixers. The mixers read the copy, and the game calls this once a frame.
 */
void PublishN64Volumes();

} // namespace coastline
