#pragma once

#include <filesystem>

namespace coastline {

class N64Audio;

/** Where the music of a race comes from. */
enum class RaceMusicSource {
    Modules, /*!< The XM modules of the Nintendo 64 release, the default. */
    CdAudio, /*!< The FLAC files the import made of the CD audio of the PC release. */
};

/**
 * Add a user of the music of the races, as the PC release opens its music device. The first user
 * finds the tracks and opens the playback device, when the game plays music.
 *
 * @param directory The directory of the music files.
 * @param source Where the music comes from.
 * @return Whether the music can play.
 */
bool StartRaceMusic(const std::filesystem::path &directory, RaceMusicSource source);

/**
 * Drop a user of the music of the races. The last user stops the music of the CD and closes its
 * device. A shared player of the menus then plays the module of the track on, as the console
 * plays it on the screens after a race.
 */
void StopRaceMusic();

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
