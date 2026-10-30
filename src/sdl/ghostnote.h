#pragma once

/**
 * @file
 * The ghost car of a time attack in the TOP GEAR GHOST note of the Controller Pak, as the console
 * writes it, and the frames the race engine of the PC release plays the ghost car from.
 *
 * The console records the input of the player: a header of the track, the car, and the setup, and
 * then the steering and the actions of each step of the race, two bytes a step. It plays the ghost
 * car by driving it with the input again. The race engine of the PC release records the pose of the
 * car at each step instead, and the SDL game records both in a time attack. The note includes only
 * the input, and the frames are stored beside the pak with the input they belong to.
 */

#include <cstddef>
#include <filesystem>
#include <optional>
#include <span>
#include <vector>

#include "common/types.h"

namespace coastline {

/** The size of the note of a ghost car, 58 pages. */
inline constexpr std::size_t GHOST_NOTE_SIZE = 0x3A00;

/** The capacity of the recording of the console, its header included. */
inline constexpr std::size_t GHOST_RECORDING_CAPACITY = 0xDE5C;

/**
 * Compress a recording into the note of a ghost car, as SaveSaveLoadNote() of the console does:
 * each of the two channels of the recording (the steering and the actions) with run lengths, and
 * the length of each channel first in the byte order of the console. The rest of the note is zero.
 *
 * @param recording The recording, its header included.
 * @return The note, or nothing when the recording does not fit in it.
 */
std::optional<std::vector<u8>> PackGhostNote(std::span<const u8> recording);

/**
 * Expand the note of a ghost car into its recording.
 *
 * @param note The note.
 * @return The recording, its header included, or nothing for a note whose channel lengths pass its
 * end or whose recording is shorter than 2 bytes or longer than GHOST_RECORDING_CAPACITY.
 */
std::optional<std::vector<u8>> UnpackGhostNote(std::span<const u8> note);

/**
 * Store the frames of the race engine with the recording of the console they were recorded with.
 *
 * @param recording The recording of the console.
 * @param frames The frames, GHOST_FRAME_SIZE bytes each.
 * @return The file.
 */
std::vector<u8> PackGhostFrames(std::span<const u8> recording, std::span<const u8> frames);

/**
 * Read the frames of the race engine of a recording of the console from the file of
 * PackGhostFrames().
 *
 * @param file The file.
 * @param recording The recording of the console.
 * @return The frames, or nothing when the file is broken or was stored with another recording.
 */
std::optional<std::vector<u8>> GhostFramesFor(std::span<const u8> file,
                                              std::span<const u8> recording);

/**
 * Write the file of PackGhostFrames(), creating its directory.
 *
 * @param path The file.
 * @param recording The recording of the console.
 * @param frames The frames, GHOST_FRAME_SIZE bytes each.
 * @return Whether the file was written.
 */
bool SaveGhostFrames(const std::filesystem::path &path,
                     std::span<const u8> recording,
                     std::span<const u8> frames);

/**
 * Read the frames of a recording from the file of SaveGhostFrames().
 *
 * @param path The file.
 * @param recording The recording of the console.
 * @return The result of GhostFramesFor(), or nothing when the file cannot be read.
 */
std::optional<std::vector<u8>> LoadGhostFrames(const std::filesystem::path &path,
                                               std::span<const u8> recording);

} // namespace coastline
