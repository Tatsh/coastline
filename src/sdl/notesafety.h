#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace coastline {

/** Counts of handling, transmission, tyre, suspension, and controller-layout choices, in order. */
using NoteEquipmentCounts = std::array<std::size_t, 5>;

/**
 * Check that a saved race time is finite, non-negative, and safe to display in signed hundredths.
 *
 * @param seconds Saved time in seconds; zero represents an unset record.
 * @return Whether display conversion can represent the time.
 */
bool ValidSavedRaceTime(float seconds);

/**
 * Validate native options-note length, volume levels, units, and equipment indices.
 *
 * @param note Native Controller Pak payload.
 * @param counts Available equipment choices in the active menu.
 * @return Whether every field the options screen reads is valid.
 */
bool ValidOptionsNote(std::span<const std::uint8_t> note, const NoteEquipmentCounts &counts);

/**
 * Validate native season-note length, times, placements, and equipment indices.
 * Season, round, and unlock availability must also be checked against the active championship.
 *
 * @param note Native Controller Pak payload.
 * @param counts Available equipment choices in the active menu.
 * @return Whether numeric profile fields are safe to load and display.
 */
bool ValidSeasonNoteValues(std::span<const std::uint8_t> note, const NoteEquipmentCounts &counts);

/**
 * Validate an expanded console ghost's length, track, car, and equipment header.
 *
 * @param recording Expanded header and paired steering/action bytes.
 * @param tracks Number of available tracks.
 * @param cars Number of available cars.
 * @param counts Available equipment choices in the active menu.
 * @return Whether the recording can index menu data and unlock masks safely.
 */
bool ValidGhostRecording(std::span<const std::uint8_t> recording,
                         std::size_t tracks,
                         std::size_t cars,
                         const NoteEquipmentCounts &counts);

} // namespace coastline
