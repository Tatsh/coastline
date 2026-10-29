#pragma once

#include <filesystem>
#include <optional>

namespace coastline {

/** How EAR plays a sound for PlayEarSound(). */
struct EarSoundOptions {
    /** The direction of the matrix of one direction, from 0 for the centre to 16. */
    int direction = 0;
    /** Whether EAR reads the whole file before it plays, rather than streaming it. */
    bool whole = false;
    /** The WAV file the mix is written to in place of the playback device, if any. */
    std::optional<std::filesystem::path> output;
};

/**
 * Play a wave file through EAR, as the PC release mixes an event of a wave file, and return when
 * EAR reports that the event stopped. The SDL subsystem of events must be running.
 *
 * @param path The wave file.
 * @param options How EAR plays the file and where the mix goes.
 * @return Whether the sound played to its end and any WAV file was written.
 */
bool PlayEarSound(const std::filesystem::path &path, const EarSoundOptions &options);

} // namespace coastline
