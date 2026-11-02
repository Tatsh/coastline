#pragma once

#include <filesystem>
#include <memory>

namespace coastline {

class N64Audio;
class RaceMusic;

/** Race music resources for one active audio session, destroyed before its shared menu player. */
class RaceMusicContext {
public:
    /** Register the exclusive race music context before race music API calls. */
    RaceMusicContext();
    /** Shut down active race music and unregister the context before SDL audio shutdown. */
    ~RaceMusicContext();
    /** Prevent copying registered race music resources. */
    RaceMusicContext(const RaceMusicContext &) = delete;
    /** Prevent assignment of registered race music resources. */
    RaceMusicContext &operator=(const RaceMusicContext &) = delete;

private:
    std::unique_ptr<RaceMusic> m_music;
};

/** Where the music of a race comes from. */
enum class RaceMusicSource {
    Modules, /*!< The XM modules of the Nintendo 64 release, the default. */
    CdAudio, /*!< The FLAC files the import made of the CD audio of the PC release. */
};

/** Front-end music with independent CD resources and the shared native XM player. */
class FrontEndMusic {
public:
    /**
     * Prepare front-end playback without starting music.
     *
     * @param audio The shared XM player, whose lifetime must exceed this music controller's.
     * @param directory The directory with menu.xm and converted CD tracks.
     * @param source The preferred menu music source, with XM fallback for unavailable CD audio.
     */
    FrontEndMusic(N64Audio &audio, const std::filesystem::path &directory, RaceMusicSource source);
    /** Stop shared XM playback and release independent CD resources. */
    ~FrontEndMusic();
    /** Prevent copying playback control and CD resources. */
    FrontEndMusic(const FrontEndMusic &) = delete;
    /** Prevent assignment of playback control and CD resources. */
    FrontEndMusic &operator=(const FrontEndMusic &) = delete;

    /**
     * Start menu music, or use XM with a warning when CD audio is unavailable.
     *
     * @return Whether CD playback starts or the XM module can be read and submitted.
     */
    bool Play();
    /** Stop front-end music before starting a race. */
    void Stop();
    /**
     * Apply the menu BGM volume to the CD stream.
     *
     * @param volume The console BGM volume, from 0 through 48, with 0 suspending CD processing.
     */
    void SetVolume(unsigned char volume);
    /**
     * Stop the previous source before selecting menu music again.
     *
     * @param source The replacement preference; an unchanged source does not restart playback.
     */
    void SetSource(RaceMusicSource source);

private:
    struct State;
    std::unique_ptr<State> m_state;
};

/**
 * Add a user of the music of the races, as the PC release opens its music device. The first user
 * finds the tracks and opens the playback device, when the game plays music.
 *
 * @param directory The directory of the music files.
 * @param source Where the music comes from.
 * @param musicTrack The engine track selecting XM music, or -1 for the current race track.
 * PC CD selection ignores this parameter.
 * @return Whether the music can play.
 */
bool StartRaceMusic(const std::filesystem::path &directory,
                    RaceMusicSource source,
                    int musicTrack = -1);

/**
 * Drop a user of the music of the races. The last user stops the music of the CD and closes its
 * device. A shared player of the menus then plays the module of the track on, as the console
 * plays it on the screens after a race.
 */
void StopRaceMusic();

/** Release all race music resources regardless of active users, before SDL audio shuts down. */
void ShutdownRaceMusic();

/** Play the track of the race from its start, with the device of the music resumed. */
void RestartRaceMusic();

/**
 * Play the modules of the races through the player of the music of the menus, which mixes the
 * channels of the console, instead of a player of their own. Two players would mix the same
 * channels on two threads.
 *
 * @param audio The player of the menus, with its device open, or null for a player of their own.
 */
void ShareRaceMusicPlayer(N64Audio *audio);

/**
 * Render frames of the track that plays, without a device, for tests.
 *
 * @param frames The stereo frames.
 * @return The number of samples that are not silent.
 */
std::size_t RenderRaceMusic(std::size_t frames);

} // namespace coastline
